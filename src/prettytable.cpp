#include <config.h>
#include <libgnome/gnome-program.h>
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <gdk-pixbuf/gdk-pixbuf.h>


#include "prettytable.h"
#include "defaulttable.h"
#include "proctable.h"
#include "util.h"


namespace
{
  const unsigned APP_ICON_SIZE = 16;

  GdkPixbuf*
  create_scaled_icon(const char *iconpath)
  {
    GError* error = NULL;
    GdkPixbuf* scaled;

    scaled = gdk_pixbuf_new_from_file_at_scale(iconpath,
					       APP_ICON_SIZE, APP_ICON_SIZE,
					       TRUE,
					       &error);

    if (error) {
      if(!(error->domain == G_FILE_ERROR
	   && error->code == G_FILE_ERROR_NOENT))
	g_warning(error->message);

      g_error_free(error);
      return NULL;
    }

    return scaled;
  }
}









PrettyTable::PrettyTable()
{
  this->theme = gtk_icon_theme_get_default();

  this->datadir
    = make_string(gnome_program_locate_file(NULL,
					    GNOME_FILE_DOMAIN_DATADIR,
					    "pixmaps/",
					    TRUE,
					    NULL));

  this->load_default_table();


  WnckScreen* screen = wnck_screen_get_default();
  g_signal_connect(G_OBJECT(screen), "application_opened",
		   G_CALLBACK(PrettyTable::on_application_opened), this);
  g_signal_connect(G_OBJECT(screen), "application_closed",
		   G_CALLBACK(PrettyTable::on_application_closed), this);
}


PrettyTable::~PrettyTable()
{
  unref_map_values(this->apps);
  unref_map_values(this->defaults);
}


void
PrettyTable::on_application_opened(WnckScreen* screen, WnckApplication* app, gpointer data)
{
  pid_t pid = wnck_application_get_pid(app);

  if (pid == 0)
    return;

  /* don't free list, we don't own it */
  if (GList* list = wnck_application_get_windows(app))
    {
      WnckWindow* win = static_cast<WnckWindow*>(list->data);

      if (GdkPixbuf* icon = wnck_window_get_icon(win))
	{
	  if (GdkPixbuf* scaled = gdk_pixbuf_scale_simple(icon,
							  APP_ICON_SIZE,
							  APP_ICON_SIZE,
							  GDK_INTERP_HYPER))
	    static_cast<PrettyTable*>(data)->register_application(pid, scaled);
	}
    }
}



void
PrettyTable::register_application(pid_t pid, GdkPixbuf* icon)
{
  /* If process already exists then set the icon. Otherwise put into hash
  ** table to be added later */
  if (ProcInfo* info = proctable_find_process(pid, ProcData::get_instance()))
    {
      info->set_icon(icon);
      // move the ref to the map
      this->apps[pid] = icon;
    }
}



void
PrettyTable::on_application_closed(WnckScreen* screen, WnckApplication* app, gpointer data)
{
  pid_t pid = wnck_application_get_pid(app);

  if (pid == 0)
    return;

  static_cast<PrettyTable*>(data)->unregister_application(pid);
}



void
PrettyTable::unregister_application(pid_t pid)
{
  IconsForPID::iterator it(this->apps.find(pid));

  if (it != this->apps.end()) {
    g_object_unref(it->second);
    this->apps.erase(it);
  }
}



GdkPixbuf*
PrettyTable::get_icon_from_theme(pid_t pid, const gchar* command)
{
  return gtk_icon_theme_load_icon(this->theme,
				  command,
				  APP_ICON_SIZE,
				  GTK_ICON_LOOKUP_USE_BUILTIN,
				  NULL);
}



GdkPixbuf*
PrettyTable::get_icon_from_default(pid_t pid, const gchar* command)
{
  IconsForCommand::iterator it(this->defaults.find(command));

  if (it != this->defaults.end()) {
    GdkPixbuf* icon = it->second;
    g_object_ref(icon);
    return icon;
  }

  return NULL;
}



GdkPixbuf*
PrettyTable::get_icon_from_wnck(pid_t pid, const gchar* command)
{
  IconsForPID::iterator it(this->apps.find(pid));

  if (it != this->apps.end()) {
    GdkPixbuf* icon = it->second;
    g_object_ref(icon);
    return icon;
  }

  return NULL;
}



GdkPixbuf*
PrettyTable::get_icon(const gchar* command, pid_t pid)
{
  typedef GdkPixbuf* (PrettyTable::*Getter)(pid_t pid,
					    const gchar* command);

  const Getter getters[] = {
    &PrettyTable::get_icon_from_theme,
    &PrettyTable::get_icon_from_wnck,
    &PrettyTable::get_icon_from_default
  };

  for (size_t i = 0; i < G_N_ELEMENTS(getters); ++i) {
    if (GdkPixbuf* icon = (this->*getters[i])(pid, command))
      return icon;
  }

  return NULL;
}




void
PrettyTable::load_default_table()
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
	iconpath = g_build_filename(this->datadir.c_str(), icon, NULL);
	pix = create_scaled_icon(iconpath);
	g_free(iconpath);
	if (!pix)
	  continue;
	cache[icon] = pix;

      } else {
	pix = it->second;
	g_object_ref(pix);
      }

      this->defaults[command] = pix;
    }
}
