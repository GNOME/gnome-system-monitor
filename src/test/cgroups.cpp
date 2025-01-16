#include "../cgroups.h"

#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

using namespace std::string_view_literals;

TEST_CASE("get_process_cgroup_name cgroup-v2", "[gnome-system-monitor]") {
  CHECK(get_process_cgroup_name("0::/\n") == ""sv);
  CHECK(get_process_cgroup_name("0::/a\n") == "/a ()"sv);
}

TEST_CASE("get_process_cgroup_name cgroup-v1 one entry", "[gnome-system-monitor]") {
  CHECK(get_process_cgroup_name("0:a1:/b1\n") == "/b1 (a1)"sv);
  CHECK(get_process_cgroup_name("0:name=a2:/b2\n") == "/b2 (a2)"sv);
}

TEST_CASE("get_process_cgroup_name cgroup-v1 multiple entries", "[gnome-system-monitor]") {
  CHECK(get_process_cgroup_name("1:a1:/b1\n0:a0:/b0\n") == "/b0 (a0), /b1 (a1)"sv);
}
