#include <config.h>
#include <libgnome/gnome-program.h>
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <map>
#include <string>

using std::string;

#include "prettytable.h"
#include "defaulttable.h"
#include "proctable.h"
#include "util.h"

#define APP_ICON_SIZE 16

typedef std::map<string, GdkPixbuf*> IconsForCommand;
typedef std::map<unsigned, GdkPixbuf*> IconsForPID;

struct _PrettyTable {
  IconsForPID apps;
  IconsForCommand defaults;
  string datadir;
  GtkIconTheme* theme;
};



static void
load_default_table(PrettyTable *pretty_table);


static void
new_application (WnckScreen *screen, WnckApplication *app, gpointer data)
{
	ProcData *procdata = static_cast<ProcData*>(data);
	ProcInfo *info;
	guint pid;
	GdkPixbuf *icon = NULL, *tmp = NULL;
	GList *list = NULL;
	WnckWindow *window;

	pid = wnck_application_get_pid (app);
	if (pid == 0)
		return;

	IconsForPID::iterator it(procdata->pretty_table->apps.find(pid));

	/* Check to see if the pid has already been added */
	if (it != procdata->pretty_table->apps.end())
		return;

	/* don't free list, we don't own it */
	list = wnck_application_get_windows (app);
	if (!list)
		return;
	window = static_cast<WnckWindow*>(list->data);
	tmp = wnck_window_get_icon (window);

	if (!tmp)
		return;

	icon =  gdk_pixbuf_scale_simple (tmp, APP_ICON_SIZE, APP_ICON_SIZE, GDK_INTERP_HYPER);
	if (!icon)
		return;

	/* If process already exists then set the icon. Otherwise put into hash
	** table to be added later */
	info = proctable_find_process (pid, procdata);

	if (info) {
	/* Don't update the icon if there already is one */
		if (info->pixbuf) {
			g_object_unref(icon);
			return;
		}
		info->pixbuf = icon;
		g_object_ref(info->pixbuf);
		if (info->is_visible) {
			GtkTreeModel *model;
			model = gtk_tree_view_get_model (GTK_TREE_VIEW (procdata->tree));
			gtk_tree_store_set (GTK_TREE_STORE (model), &info->node,
					    COL_PIXBUF, info->pixbuf, -1);
		}
	}

	procdata->pretty_table->apps[pid] = icon;
}


static void
application_finished (WnckScreen *screen, WnckApplication *app, gpointer data)
{
	ProcData * const procdata = static_cast<ProcData*>(data);
	guint pid;

	pid =  wnck_application_get_pid (app);
	if (pid == 0)
		return;

	IconsForPID::iterator it(procdata->pretty_table->apps.find(pid));

	if (it != procdata->pretty_table->apps.end()) {
	  g_object_unref(it->second);
	  procdata->pretty_table->apps.erase(it);
	}
}


void pretty_table_new (ProcData *procdata)
{
	WnckScreen *screen;
	PrettyTable *pretty_table = new PrettyTable;

	pretty_table->theme = gtk_icon_theme_get_default ();




	pretty_table->datadir
	  = make_string(gnome_program_locate_file(NULL,
						  GNOME_FILE_DOMAIN_DATADIR,
						  "pixmaps/",
						  TRUE,
						  NULL));

	load_default_table(pretty_table);


	procdata->pretty_table = pretty_table;
	screen = wnck_screen_get_default();
	g_signal_connect (G_OBJECT (screen), "application_opened",
			  G_CALLBACK (new_application), procdata);
	g_signal_connect (G_OBJECT (screen), "application_closed",
			  G_CALLBACK (application_finished), procdata);
}





static GdkPixbuf *
get_icon_from_theme (PrettyTable *pretty_table,
		     guint pid,
		     const gchar *command)
{
	return gtk_icon_theme_load_icon (pretty_table->theme,
					 command,
					 APP_ICON_SIZE,
					 GTK_ICON_LOOKUP_USE_BUILTIN,
					 NULL);
}



static GdkPixbuf *
get_icon_from_default (PrettyTable *pretty_table,
		       guint pid,
		       const gchar *command)
{
  IconsForCommand::iterator it(pretty_table->defaults.find(command));

  if (it != pretty_table->defaults.end()) {

	GdkPixbuf* icon = it->second;
	g_object_ref(icon);
	return icon;
  }

  return NULL;
}



static GdkPixbuf *
get_icon_from_wnck (PrettyTable *pretty_table,
		    guint pid,
		    const gchar *command)
{
	IconsForPID::iterator it(pretty_table->apps.find(pid));

	if (it != pretty_table->apps.end()) {
	  GdkPixbuf* icon = it->second;
	  g_object_ref(icon);
	  return icon;
	}

	return NULL;
}



GdkPixbuf *pretty_table_get_icon (PrettyTable *pretty_table,
				  const gchar *command, guint pid)
{
	typedef GdkPixbuf * (*IconGetter) (PrettyTable *pretty_table,
					   guint pid,
					   const gchar *command);


	const IconGetter getters[] = {
		get_icon_from_theme,
		get_icon_from_wnck,
		get_icon_from_default
	};


	size_t i;

	for (i = 0; i < G_N_ELEMENTS (getters); ++i) {
		GdkPixbuf *icon;

		icon = (getters[i]) (pretty_table, pid, command);

		if (icon) {
#if 0
			g_print("Get icon method %lu for '%s'\n",
				(gulong)i,
				command);
#endif
			return icon;
		}
	}

	return NULL;
}



void pretty_table_free (PrettyTable *pretty_table) {
	delete pretty_table;
}







static GdkPixbuf *
create_scaled_icon(const char *iconpath)
{
	GError *error = NULL;
	GdkPixbuf *scaled;

	scaled = gdk_pixbuf_new_from_file_at_scale(iconpath,
						   APP_ICON_SIZE, APP_ICON_SIZE,
						   TRUE,
						   &error);

	if(error) {
	  if(!(error->domain == G_FILE_ERROR
	       && error->code == G_FILE_ERROR_NOENT))
		    g_warning(error->message);

	  g_error_free(error);
	  return NULL;
	}

	return scaled;
}


static void
load_default_table(PrettyTable *pretty_table)
{
  typedef std::map<string, GdkPixbuf*> IconCache;

  IconCache cache;

  for (size_t i = 0; i != G_N_ELEMENTS(default_table); ++i)
    {
      const char* command = default_table[i].command;
      const char* icon = default_table[i].icon;
      GdkPixbuf* pix;

      IconCache::iterator it(cache.find(icon));

      if (it == cache.end()) {
	char* iconpath;
	iconpath = g_build_filename(pretty_table->datadir.c_str(), icon, NULL);
	pix = create_scaled_icon(iconpath);
	g_free(iconpath);
	if (!pix)
	  continue;
	cache[icon] = pix;

      } else {
	pix = it->second;
      }

      pretty_table->defaults[command] = pix;
    }
}
