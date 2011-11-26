#include <config.h>

#include <glib/gi18n.h>
#include <glibmm/optionentry.h>


#include "argv.h"

namespace procman
{
    OptionGroup::OptionGroup()
        : Glib::OptionGroup("", ""),
          show_system_tab(false),
          show_processes_tab(false),
          show_resources_tab(false),
          show_file_systems_tab(false)
    {
        Glib::OptionEntry sys_tab;
        sys_tab.set_long_name("show-system-tab");
        sys_tab.set_short_name('s');
        sys_tab.set_description(_("Show the System tab"));

        Glib::OptionEntry proc_tab;
        proc_tab.set_long_name("show-processes-tab");
        proc_tab.set_short_name('p');
        proc_tab.set_description(_("Show the Processes tab"));

        Glib::OptionEntry res_tab;
        res_tab.set_long_name("show-resources-tab");
        res_tab.set_short_name('r');
        res_tab.set_description(_("Show the Resources tab"));

        Glib::OptionEntry fs_tab;
        fs_tab.set_long_name("show-file-systems-tab");
        fs_tab.set_short_name('f');
        fs_tab.set_description(_("Show the File Systems tab"));

        this->add_entry(sys_tab, this->show_system_tab);
        this->add_entry(proc_tab, this->show_processes_tab);
        this->add_entry(res_tab, this->show_resources_tab);
        this->add_entry(fs_tab, this->show_file_systems_tab);
    }
}

