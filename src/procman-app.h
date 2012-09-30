#ifndef _PROCMAN_PROCMAN_APP_H_
#define _PROCMAN_PROCMAN_APP_H_

#include <gtkmm.h>

#include "procman.h"

class ProcmanApp : public Gtk::Application
{
private:
    ProcData *procdata;

    void on_preferences_activate(const Glib::VariantBase&);
    void on_lsof_activate(const Glib::VariantBase&);
    void on_help_activate(const Glib::VariantBase&);
    void on_quit_activate(const Glib::VariantBase&);
protected:
    ProcmanApp();
public:
    static Glib::RefPtr<ProcmanApp> create ();
protected:
    virtual void on_activate();
    virtual int on_command_line(const Glib::RefPtr<Gio::ApplicationCommandLine>& command_line);
    virtual void on_startup();
    virtual void on_shutdown();
};

#endif  /* _PROCMAN_PROCMAN_APP_H_ */
