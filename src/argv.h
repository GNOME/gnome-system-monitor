#ifndef H_PROCMAN_ARGV_1205873424
#define H_PROCMAN_ARGV_1205873424

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
    };
}

#endif // H_PROCMAN_ARGV_1205873424
