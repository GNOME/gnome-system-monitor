#include <config.h>

#include <glib/gi18n.h>
#include <glibmm/optionentry.h>

#include "argv.h"

namespace procman
{
OptionGroup::OptionGroup()
  : Glib::OptionGroup ("", ""),
  show_system_tab (false),
  show_processes_tab (false),
  show_resources_tab (false),
  show_file_systems_tab (false),
  print_version (false)
{
  processes_tab_option.set_long_name ("show-processes-tab");
  processes_tab_option.set_short_name ('p');
  processes_tab_option.set_description (_("Show the Processes tab"));

  resources_tab_option.set_long_name ("show-resources-tab");
  resources_tab_option.set_short_name ('r');
  resources_tab_option.set_description (_("Show the Resources tab"));

  fs_tab_option.set_long_name ("show-file-systems-tab");
  fs_tab_option.set_short_name ('f');
  fs_tab_option.set_description (_("Show the File Systems tab"));

  show_version_option.set_long_name ("version");
  show_version_option.set_description (_("Show the application’s version"));

  this->add_entry (processes_tab_option, this->show_processes_tab);
  this->add_entry (resources_tab_option, this->show_resources_tab);
  this->add_entry (fs_tab_option, this->show_file_systems_tab);
  this->add_entry (show_version_option, this->print_version);
}

std::vector<Glib::OptionEntry> OptionGroup::entries() const {
  return std::vector<Glib::OptionEntry> {
    processes_tab_option,
    resources_tab_option,
    fs_tab_option,
    show_version_option
  };
}

void OptionGroup::load(const Glib::RefPtr<Glib::VariantDict> &args) {
  show_processes_tab = args->contains (processes_tab_option.get_long_name () );
  show_resources_tab = args->contains (resources_tab_option.get_long_name () );
  show_file_systems_tab = args->contains (fs_tab_option.get_long_name () );
  print_version = args->contains (show_version_option.get_long_name () );
}
}
