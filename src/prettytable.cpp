#include <config.h>
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
}









PrettyTable::PrettyTable()
{
  this->theme = gtk_icon_theme_get_default();

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
  if (ProcInfo* info = ProcInfo::find(pid))
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
PrettyTable::get_icon_from_theme(pid_t, const gchar* command)
{
  return gtk_icon_theme_load_icon(this->theme,
				  command,
				  APP_ICON_SIZE,
				  GTK_ICON_LOOKUP_USE_BUILTIN,
				  NULL);
}


bool PrettyTable::get_default_icon_name(const string &cmd, string &name)
{
  for (size_t i = 0; i != G_N_ELEMENTS(default_table); ++i) {
    if (default_table[i].command->FullMatch(cmd)) {
      name = default_table[i].icon;
      return true;
    }
  }

  return false;
}

/*
  Try to get an icon from the default_table
  If it's not in defaults, try to load it.
  If there is no default for a command, store NULL in defaults
  so we don't have to lookup again.
*/

GdkPixbuf*
PrettyTable::get_icon_from_default(pid_t, const gchar* command)
{
  GdkPixbuf* pix = NULL;
  string name;

  if (this->get_default_icon_name(command, name)) {
    IconCache::iterator it(this->defaults.find(name));

    if (it == this->defaults.end()) {
      pix = gtk_icon_theme_load_icon(this->theme,
				     name.c_str(),
				     APP_ICON_SIZE,
				     GTK_ICON_LOOKUP_USE_BUILTIN,
				     NULL);
      if (pix)
	this->defaults[name] = pix;

    } else
      pix = it->second;
  }

  if (pix)
    g_object_ref(pix);

  return pix;
}



GdkPixbuf*
PrettyTable::get_icon_from_wnck(pid_t pid, const gchar*)
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
PrettyTable::get_icon_from_name(pid_t, const gchar* command)
{
  return gtk_icon_theme_load_icon(this->theme,
				  command,
				  APP_ICON_SIZE,
				  GTK_ICON_LOOKUP_USE_BUILTIN,
				  NULL);
}


GdkPixbuf*
PrettyTable::get_icon_dummy(pid_t, const gchar*)
{
  return gtk_icon_theme_load_icon(this->theme,
				  "applications-other",
				  APP_ICON_SIZE,
				  GTK_ICON_LOOKUP_USE_BUILTIN,
				  NULL);
}


GdkPixbuf*
PrettyTable::get_icon(const gchar* command, pid_t pid)
{
  typedef GdkPixbuf* (PrettyTable::*Getter)(pid_t pid,
					    const gchar* command);

  const Getter getters[] = {
    &PrettyTable::get_icon_from_wnck,
    &PrettyTable::get_icon_from_theme,
    &PrettyTable::get_icon_from_default,
    &PrettyTable::get_icon_from_name,
    &PrettyTable::get_icon_dummy,
  };

  for (size_t i = 0; i < G_N_ELEMENTS(getters); ++i) {
    if (GdkPixbuf* icon = (this->*getters[i])(pid, command))
      return icon;
  }

  return NULL;
}

