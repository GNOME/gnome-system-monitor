/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#ifndef _GSM_PROCINFO_H_
#define _GSM_PROCINFO_H_

#include <config.h>

#include <gtkmm.h>

#include <application.h>

struct MutableProcInfo
{
MutableProcInfo()
: status(0)
    { }

    std::string user;

    gchar wchan[40];

    // all these members are filled with libgtop which uses
    // guint64 (to have fixed size data) but we don't need more
    // than an unsigned long (even for 32bit apps on a 64bit
    // kernel) as these data are amounts, not offsets.
    gulong vmsize;
    gulong memres;
    gulong memshared;
    gulong memwritable;
    gulong mem;

#ifdef HAVE_WNCK
    // wnck gives an unsigned long
    gulong memxserver;
#endif

    gulong start_time;
    guint64 cpu_time;
    guint status;
    guint pcpu;
    gint nice;
    gchar *cgroup_name;

    gchar *unit;
    gchar *session;
    gchar *seat;

    std::string owner;
};


class ProcInfo
: public MutableProcInfo
{
    /* undefined */ ProcInfo& operator=(const ProcInfo&);
    /* undefined */ ProcInfo(const ProcInfo&);

    typedef std::map<guint, std::string> UserMap;
    /* cached username */
    static UserMap users;

  public:

    // TODO: use a set instead
    // sorted by pid. The map has a nice property : it is sorted
    // by pid so this helps a lot when looking for the parent node
    // as ppid is nearly always < pid.
    typedef std::map<pid_t, ProcInfo*> List;
    typedef List::iterator Iterator;

    static List all;

    static ProcInfo* find(pid_t pid);
    static Iterator begin() { return ProcInfo::all.begin(); }
    static Iterator end() { return ProcInfo::all.end(); }


    ProcInfo(pid_t pid);
    ~ProcInfo();
    // adds one more ref to icon
    void set_icon(Glib::RefPtr<Gdk::Pixbuf> icon);
    void set_user(guint uid);
    std::string lookup_user(guint uid);

    GtkTreeIter     node;
    Glib::RefPtr<Gdk::Pixbuf> pixbuf;
    gchar           *tooltip;
    gchar           *name;
    gchar           *arguments;

    gchar           *security_context;

    const guint     pid;
    guint           ppid;
    guint           uid;

// private:
    // tracks cpu time per process keeps growing because if a
    // ProcInfo is deleted this does not mean that the process is
    // not going to be recreated on the next update.  For example,
    // if dependencies + (My or Active), the proclist is cleared
    // on each update.  This is a workaround
    static std::map<pid_t, guint64> cpu_times;
};

void update_info (GsmApplication *app, ProcInfo *info);
void get_process_memory_writable (ProcInfo *info);

#endif /* _GSM_PROCINFO_H_ */
