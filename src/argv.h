#ifndef _GSM_ARGV_H_
#define _GSM_ARGV_H_

#include <glibmm/optiongroup.h>

namespace procman
{
class OptionGroup
  : public Glib::OptionGroup
{
public:
OptionGroup();

bool show_system_tab;
bool show_processes_tab;
bool show_resources_tab;
bool show_file_systems_tab;
bool print_version;
};
}

#endif /* _GSM_ARGV_H_ */
