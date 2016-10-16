/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include "cgroups.h"
#include "util.h"

bool
cgroups_enabled(void)
{
    static bool initialized = false;
    static bool has_cgroups;

    if (not initialized) {
        initialized = true;
        has_cgroups = Glib::file_test("/proc/cgroups", Glib::FileTest::FILE_TEST_EXISTS);
    }

    return has_cgroups;
}

static std::pair<Glib::ustring, Glib::ustring>
parse_cgroup_line(const Glib::ustring& line) {
    auto split = std::vector<Glib::ustring>(Glib::Regex::split_simple(":", line));
    if (split.size() < 3) { return std::make_pair("", ""); }
    auto cgroups = split.at(2);
    if (cgroups.empty() || cgroups == "/") { return std::make_pair("", ""); }
    auto category = split.at(1);
    if (category.find("name=") != category.npos) { return std::make_pair("", ""); }

    return std::make_pair(category, cgroups);
}

static Glib::ustring
get_process_cgroup_string(unsigned int pid) {
    if (not cgroups_enabled())
        return "";

    /* read out of /proc/pid/cgroup */
    auto path = Glib::ustring::compose("/proc/%1/cgroup", pid);
    Glib::ustring text;
    try { text = Glib::file_get_contents(path); } catch (...) { return ""; }
    auto lines = std::vector<Glib::ustring>(Glib::Regex::split_simple("\n", text));

    Glib::ustring cgroup_string;
    for (auto& line : lines) {
        auto data = parse_cgroup_line(line);
        if (data.first.empty() || data.second.empty()) { continue; }
        if (!cgroup_string.empty()) { cgroup_string += ", "; }
        cgroup_string += Glib::ustring::compose("%1 (%2)", data.second, data.first);
    }

    return cgroup_string;
}

void get_process_cgroup_info(ProcInfo& info) {
    auto cgroup_string = get_process_cgroup_string(info.pid);

    g_free(info.cgroup_name);
    info.cgroup_name = cgroup_string.empty() ? nullptr : g_strdup(cgroup_string.c_str());
}
