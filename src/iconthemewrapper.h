/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#ifndef _GSM_ICON_THEME_WRAPPER_H_
#define _GSM_ICON_THEME_WRAPPER_H_

#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <gtkmm/icontheme.h>
#include <gdkmm/pixbuf.h>

namespace procman
{
    class IconThemeWrapper
    {
      public:
        // returns 0 instead of raising an exception
        Glib::RefPtr<Gdk::Pixbuf>
            load_icon(const Glib::ustring& icon_name, int size, Gtk::IconLookupFlags flags) const;
        Glib::RefPtr<Gdk::Pixbuf>
            load_gicon(const Glib::RefPtr<Gio::Icon>& gicon, int size, Gtk::IconLookupFlags flags) const;

        const IconThemeWrapper* operator->() const
        { return this; }
    };
}

#endif /* _GSM_ICON_THEME_WRAPPER_H_ */
