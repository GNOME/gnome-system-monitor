/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#ifndef _GSM_LOAD_GRAPH_H_
#define _GSM_LOAD_GRAPH_H_

#include <glib.h>
#include <glibtop/cpu.h>

#include "legacy/gsm_color_button.h"
#include "util.h"
#include "settings-keys.h"

enum
{
    LOAD_GRAPH_CPU,
    LOAD_GRAPH_MEM,
    LOAD_GRAPH_NET
};

enum
{
    CPU_TOTAL,
    CPU_USED,
    N_CPU_STATES
};

struct LoadGraphLabels
{
    GtkLabel *cpu[GLIBTOP_NCPU];
    GtkLabel *memory;
    GtkLabel *swap;
    GtkLabel *net_in;
    GtkLabel *net_in_total;
    GtkLabel *net_out;
    GtkLabel *net_out_total;
};

struct LoadGraph
  : private procman::NonCopyable
{
    static const unsigned GRAPH_MIN_HEIGHT = 40;

    LoadGraph(guint type);
    ~LoadGraph();

    unsigned num_bars() const;
    void clear_background();
    bool is_logarithmic_scale() const;
    char* get_caption(guint index);
    float translate_to_log_partial_if_needed(float position_partial);

    double fontsize;
    double rmargin;
    /* left margin */
    double indent;

    guint n;
    gint type;
    guint speed;
    guint num_points;
    guint latest;
    guint draw_width, draw_height;
    guint render_counter;
    guint frames_per_unit;
    guint graph_dely;
    guint real_draw_height;
    double graph_delx;
    guint graph_buffer_offset;

    std::vector<GdkRGBA> colors;

    std::vector<double> data_block;
    std::vector<double*> data;

    GtkBox *main_widget;
    GtkDrawingArea *disp;

    cairo_surface_t *background;

    guint timer_index;

    gboolean draw;

    LoadGraphLabels labels;
    GsmColorButton *mem_color_picker;
    GsmColorButton *swap_color_picker;

    Glib::RefPtr<Gio::Settings> font_settings;

    /* union { */
    struct CPU
    {
        guint now; /* 0 -> current, 1 -> last
                      now ^ 1 each time */
        /* times[now], times[now ^ 1] is last */
        guint64 times[2][GLIBTOP_NCPU][N_CPU_STATES];
    } cpu;

    struct NET
    {
        guint64 last_in, last_out;
        guint64 time;
        guint64 max;
        std::vector<unsigned> values;
    } net;
    /* }; */
};

/* Force a drawing update */
void
load_graph_queue_draw (LoadGraph *g);

/* Start load graph. */
void
load_graph_start (LoadGraph *g);

/* Stop load graph. */
void
load_graph_stop (LoadGraph *g);

/* Change load graph speed and restart it if it has been previously started */
void
load_graph_change_speed (LoadGraph *g,
                         guint new_speed);

/* Change load graph data points and restart it if it has been previously started */
void
load_graph_change_num_points(LoadGraph *g,
                             guint new_num_points);

/* Clear the history data. */
void
load_graph_reset (LoadGraph *g);

LoadGraphLabels*
load_graph_get_labels (LoadGraph *g) G_GNUC_CONST;

GtkBox*
load_graph_get_widget (LoadGraph *g) G_GNUC_CONST;

GsmColorButton*
load_graph_get_mem_color_picker(LoadGraph *g) G_GNUC_CONST;

GsmColorButton*
load_graph_get_swap_color_picker(LoadGraph *g) G_GNUC_CONST;

#endif /* _GSM_LOAD_GRAPH_H_ */
