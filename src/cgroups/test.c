/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <glib.h>

#include "cgroups.c"


static void
test_cgroups_v2 (void)
{
  g_assert_cmpstr (extract_name (NULL), ==, "");
  g_assert_cmpstr (extract_name (""), ==, "");
  g_assert_cmpstr (extract_name ("0::/"), ==, "");
  g_assert_cmpstr (extract_name ("0::/\n"), ==, "");
  g_assert_cmpstr (extract_name ("0::/a\n"), ==, "/a");
}


static void
test_cgroups_v1_one_entry (void)
{
  g_assert_cmpstr (extract_name ("0:a1:/b1\n"), ==, "/b1 (a1)");
  g_assert_cmpstr (extract_name ("0:name=a2:/b2\n"), ==, "/b2 (a2)");
}


static void
test_cgroups_v1_multiple_entry (void)
{
  g_assert_cmpstr (extract_name ("1:a1:/b1\n0:a0:/b0\n"), ==, "/b0 (a0), /b1 (a1)");
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/gnome-system-monitor/cgroups/v2", test_cgroups_v2);
  g_test_add_func ("/gnome-system-monitor/cgroups/v1/one-entry",
                   test_cgroups_v1_one_entry);
  g_test_add_func ("/gnome-system-monitor/cgroups/v1/multiple-entry",
                   test_cgroups_v1_multiple_entry);

  return g_test_run ();
}
