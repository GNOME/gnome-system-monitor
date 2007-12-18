#include <config.h>

#include <gtkmm.h>

#include "iconthemewrapper.h"


Glib::RefPtr<Gdk::Pixbuf>
procman::IconThemeWrapper::load_icon(const Glib::ustring& icon_name,
				     int size, Gtk::IconLookupFlags flags) const
{
  try
    {
      return Gtk::IconTheme::get_default()->load_icon(icon_name, size, flags);
    }
  catch (Gtk::IconThemeError &error)
    {
      if (error.code() != Gtk::IconThemeError::ICON_THEME_NOT_FOUND)
	throw;
      return Glib::RefPtr<Gdk::Pixbuf>();
    }
}

