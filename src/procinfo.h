#pragma once

#include <string>

#include <gdkmm/texture.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <sys/types.h>

#include "util.h"

struct MutableProcInfo
  : private procman::NonCopyable
{
  MutableProcInfo ()
    : vmsize (0UL),
    memres (0UL),
    memshared (0UL),
    memwritable (0UL),
    mem (0UL),
    start_time (0UL),
    cpu_time (0ULL),
    disk_read_bytes_total (0ULL),
    disk_write_bytes_total (0ULL),
    disk_read_bytes_current (0ULL),
    disk_write_bytes_current (0ULL),
    status (0U),
    pcpu (0),
    nice (0)
  {
  }

  std::string user;

  std::string wchan;

  // all these members are filled with libgtop which uses
  // guint64 (to have fixed size data) but we don't need more
  // than an unsigned long (even for 32bit apps on a 64bit
  // kernel) as these data are amounts, not offsets.
  gulong vmsize;
  gulong memres;
  gulong memshared;
  gulong memwritable;
  gulong mem;

  gulong start_time;
  guint64 cpu_time;
  guint64 disk_read_bytes_total;
  guint64 disk_write_bytes_total;
  guint64 disk_read_bytes_current;
  guint64 disk_write_bytes_current;
  guint status;
  gdouble pcpu;
  gint nice;
  std::string cgroup_name;

  std::string unit;
  std::string session;
  std::string seat;

  std::string owner;
};


class ProcInfo
  : public MutableProcInfo
{
public:
  ProcInfo& operator= (const ProcInfo&) = delete;
  ProcInfo(const ProcInfo&) = delete;
  ProcInfo(pid_t pid);

  // adds one more ref to icon
  void        set_icon (Glib::RefPtr<Gdk::Texture> icon);
  void        set_user (guint uid);
  std::string lookup_user (guint uid);

  GtkTreeIter node;
  Glib::RefPtr<Gdk::Texture> icon;
  std::string tooltip;
  std::string name;
  std::string arguments;

  std::string security_context;

  const pid_t pid;
  pid_t ppid;
  guint uid;
};
