#include "../join.h"

#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

using namespace std::string_view_literals;

TEST_CASE("join empty range is empty", "[gnome-system-monitor]") {
  CHECK(procman::join<std::string>(std::vector<std::string_view>(), ""sv) == ""sv);
  CHECK(procman::join<std::string>(std::vector<std::string_view>(), ":)"sv) == ""sv);
}

TEST_CASE("join range of one element", "[gnome-system-monitor]") {
  CHECK(procman::join<std::string>(std::vector<std::string_view>({""sv}), ""sv) == ""sv);
  CHECK(procman::join<std::string>(std::vector<std::string_view>({""sv}), ":)"sv) == ""sv);

  CHECK(procman::join<std::string>(std::vector<std::string_view>({"a"sv}), ""sv) == "a"sv);
  CHECK(procman::join<std::string>(std::vector<std::string_view>({"a"sv}), ":)"sv) == "a"sv);
}

TEST_CASE("join range of two elements", "[gnome-system-monitor]") {
  CHECK(procman::join<std::string>(std::vector<std::string_view>({""sv, ""sv}), ""sv) == ""sv);
  CHECK(procman::join<std::string>(std::vector<std::string_view>({""sv, ""sv}), ":)"sv) == ":)"sv);

  CHECK(procman::join<std::string>(std::vector<std::string_view>({"a"sv, ""sv}), ""sv) == "a"sv);
  CHECK(procman::join<std::string>(std::vector<std::string_view>({"a"sv, ""sv}), ":)"sv) == "a:)"sv);
  CHECK(procman::join<std::string>(std::vector<std::string_view>({""sv, "b"sv}), ""sv) == "b"sv);
  CHECK(procman::join<std::string>(std::vector<std::string_view>({""sv, "b"sv}), ":)"sv) == ":)b"sv);

  CHECK(procman::join<std::string>(std::vector<std::string_view>({"a"sv, "b"sv}), ""sv) == "ab"sv);
  CHECK(procman::join<std::string>(std::vector<std::string_view>({"a"sv, "b"sv}), ":)"sv) == "a:)b"sv);
}

TEST_CASE("join range of three elements", "[gnome-system-monitor]") {
  CHECK(procman::join<std::string>(std::vector<std::string_view>({""sv, ""sv, ""sv}), ""sv) == ""sv);
  CHECK(procman::join<std::string>(std::vector<std::string_view>({""sv, ""sv, ""sv}), ":)"sv) == ":):)"sv);

  CHECK(procman::join<std::string>(std::vector<std::string_view>({"a"sv, ""sv, ""sv}), ""sv) == "a"sv);
  CHECK(procman::join<std::string>(std::vector<std::string_view>({""sv, "b"sv, ""sv}), ""sv) == "b"sv);
  CHECK(procman::join<std::string>(std::vector<std::string_view>({""sv, ""sv, "c"sv}), ""sv) == "c"sv);

  CHECK(procman::join<std::string>(std::vector<std::string_view>({"a"sv, ""sv, ""sv}), ":)"sv) == "a:):)"sv);
  CHECK(procman::join<std::string>(std::vector<std::string_view>({""sv, "b"sv, ""sv}), ":)"sv) == ":)b:)"sv);
  CHECK(procman::join<std::string>(std::vector<std::string_view>({""sv, ""sv, "c"sv}), ":)"sv) == ":):)c"sv);

  CHECK(procman::join<std::string>(std::vector<std::string_view>({"a"sv, "b"sv, "c"sv}), ":)"sv) == "a:)b:)c"sv);
}

