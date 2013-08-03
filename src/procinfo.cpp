/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include <glib/gprintf.h>
#include <glibtop.h>
#include <glibtop/loadavg.h>
#include <glibtop/proclist.h>
#include <glibtop/procstate.h>
#include <glibtop/procmem.h>
#include <glibtop/procmap.h>
#include <glibtop/proctime.h>
#include <glibtop/procuid.h>
#include <glibtop/procargs.h>
#include <glibtop/prockernel.h>
#include <glibtop/mem.h>
#include <glibtop/swap.h>
#include <sys/stat.h>
#include <pwd.h>

#ifdef HAVE_SYSTEMD
#include <systemd/sd-login.h>
#endif

#ifdef HAVE_WNCK
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>
#endif

#include "procinfo.h"

#include "cgroups.h"
#include "selinux.h"
#include "util.h"


ProcInfo::UserMap ProcInfo::users;
ProcInfo::List ProcInfo::all;
std::map<pid_t, guint64> ProcInfo::cpu_times;

ProcInfo* ProcInfo::find(pid_t pid)
{
    Iterator it(ProcInfo::all.find(pid));
    return (it == ProcInfo::all.end() ? NULL : it->second);
}

ProcInfo::~ProcInfo()
{
    g_free(this->name);
    g_free(this->tooltip);
    g_free(this->arguments);
    g_free(this->security_context);
    g_free(this->cgroup_name);
    // The following are allocated inside of the sd_pid_get_*
    // functions using malloc(). Free with free() instead of g_free()
    // to insure proper clean up.
    free(this->unit);
    free(this->session);
    free(this->seat);
}

static void
get_process_name (ProcInfo *info,
                  const gchar *cmd, const GStrv args)
{
    if (args) {
        // look for /usr/bin/very_long_name
        // and also /usr/bin/interpreter /usr/.../very_long_name
        // which may have use prctl to alter 'cmd' name
        for (int i = 0; i != 2 && args[i]; ++i) {
            char* basename;
            basename = g_path_get_basename(args[i]);

            if (g_str_has_prefix(basename, cmd)) {
                info->name = basename;
                return;
            }

            g_free(basename);
        }
    }

    info->name = g_strdup (cmd);
}

static void
get_process_systemd_info(ProcInfo *info)
{
#ifdef HAVE_SYSTEMD
    uid_t uid;

    if (!LOGIND_RUNNING())
        return;

    free(info->unit);
    info->unit = NULL;
    sd_pid_get_unit(info->pid, &info->unit);

    free(info->session);
    info->session = NULL;
    sd_pid_get_session(info->pid, &info->session);

    free(info->seat);
    info->seat = NULL;

    if (info->session != NULL)
        sd_session_get_seat(info->session, &info->seat);

    if (sd_pid_get_owner_uid(info->pid, &uid) >= 0)
        info->owner = info->lookup_user(uid);
    else
        info->owner = "";
#endif
}

void
ProcInfo::get_writable_memory ()
{
    glibtop_proc_map buf;
    glibtop_map_entry *maps;

    maps = glibtop_get_proc_map(&buf, this->pid);

    gulong memwritable = 0;
    const unsigned number = buf.number;

    for (unsigned i = 0; i < number; ++i) {
#ifdef __linux__
        memwritable += maps[i].private_dirty;
#else
        if (maps[i].perm & GLIBTOP_MAP_PERM_WRITE)
            memwritable += maps[i].size;
#endif
    }

    this->memwritable = memwritable;

    g_free(maps);
}

static void
get_process_memory_info(ProcInfo *info)
{
    glibtop_proc_mem procmem;
#ifdef HAVE_WNCK
    WnckResourceUsage xresources;

    wnck_pid_read_resource_usage (gdk_screen_get_display (gdk_screen_get_default ()),
                                  info->pid,
                                  &xresources);

    info->memxserver = xresources.total_bytes_estimate;
#endif

    glibtop_get_proc_mem(&procmem, info->pid);

    info->vmsize    = procmem.vsize;
    info->memres    = procmem.resident;
    info->memshared = procmem.share;

    info->mem = info->memres - info->memshared;
#ifdef HAVE_WNCK
    info->mem += info->memxserver;
#endif
}

void
ProcInfo::update (ProcmanApp *app)
{
    glibtop_proc_state procstate;
    glibtop_proc_uid procuid;
    glibtop_proc_time proctime;
    glibtop_proc_kernel prockernel;

    ProcInfo *info = this;

    glibtop_get_proc_kernel(&prockernel, info->pid);
    g_strlcpy(info->wchan, prockernel.wchan, sizeof info->wchan);

    glibtop_get_proc_state (&procstate, info->pid);
    info->status = procstate.state;

    glibtop_get_proc_uid (&procuid, info->pid);
    glibtop_get_proc_time (&proctime, info->pid);

    get_process_memory_info(info);

    info->set_user(procstate.uid);

    // if the cpu time has increased reset the status to running
    // regardless of kernel state (#606579)
    guint64 difference = proctime.rtime - info->cpu_time;
    if (difference > 0) 
        info->status = GLIBTOP_PROCESS_RUNNING;
    info->pcpu = difference * 100 / app->cpu_total_time;
    info->pcpu = MIN(info->pcpu, 100);

    if (not app->config.solaris_mode)
        info->pcpu *= app->config.num_cpus;

    ProcInfo::cpu_times[info->pid] = info->cpu_time = proctime.rtime;
    info->nice = procuid.nice;
    info->ppid = procuid.ppid;

    /* get cgroup data */
    get_process_cgroup_info(info);

    get_process_systemd_info(info);
}

std::string
ProcInfo::lookup_user(guint uid)
{
    typedef std::pair<ProcInfo::UserMap::iterator, bool> Pair;
    ProcInfo::UserMap::value_type hint(uid, "");
    Pair p(ProcInfo::users.insert(hint));

    // procman_debug("User lookup for uid %u: %s", uid, (p.second ? "MISS" : "HIT"));

    if (p.second) {
        struct passwd* pwd;
        pwd = getpwuid(uid);

        if (pwd && pwd->pw_name)
            p.first->second = pwd->pw_name;
        else {
            char username[16];
            g_sprintf(username, "%u", uid);
            p.first->second = username;
        }
    }

    return p.first->second;
}

void
ProcInfo::set_user(guint uid)
{
    if (G_LIKELY(this->uid == uid))
        return;

    this->uid = uid;
    this->user = lookup_user(uid);
}

ProcInfo::ProcInfo(pid_t pid)
    : status(0),
      tooltip(NULL),
      name(NULL),
      arguments(NULL),
      security_context(NULL),
      pid(pid),
      uid(-1)
{
    ProcInfo * const info = this;
    glibtop_proc_state procstate;
    glibtop_proc_time proctime;
    glibtop_proc_args procargs;
    gchar** arguments;

    glibtop_get_proc_state (&procstate, pid);
    glibtop_get_proc_time (&proctime, pid);
    arguments = glibtop_get_proc_argv (&procargs, pid, 0);

    /* FIXME : wrong. name and arguments may change with exec* */
    get_process_name (info, procstate.cmd, static_cast<const GStrv>(arguments));

    std::string tooltip = make_string(g_strjoinv(" ", arguments));
    if (tooltip.empty())
        tooltip = procstate.cmd;

    info->tooltip = g_markup_escape_text(tooltip.c_str(), -1);

    info->arguments = g_strescape(tooltip.c_str(), "\\\"");
    g_strfreev(arguments);

    guint64 cpu_time = proctime.rtime;
    std::map<pid_t, guint64>::iterator it(ProcInfo::cpu_times.find(pid));
    if (it != ProcInfo::cpu_times.end())
    {
        if (proctime.rtime >= it->second)
            cpu_time = it->second;
    }
    info->cpu_time = cpu_time;
    info->start_time = proctime.start_time;

    get_process_selinux_context (info);
    info->cgroup_name = NULL;
    get_process_cgroup_info(info);

    info->unit = info->session = info->seat = NULL;
    get_process_systemd_info(info);
}

void
ProcInfo::set_icon(Glib::RefPtr<Gdk::Pixbuf> icon)
{
    this->pixbuf = icon;
}

