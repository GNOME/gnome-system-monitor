/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#ifndef _GSM_LOAD_GRAPH_H_
#define _GSM_LOAD_GRAPH_H_

#include <glib.h>
#include <glibtop/cpu.h>

#include "legacy/gsm_color_button.h"
#include "util.h"

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
    static const unsigned NUM_POINTS = 60 + 2;
    static const unsigned GRAPH_MIN_HEIGHT = 40;

    LoadGraph(guint type);
    ~LoadGraph();

    unsigned num_bars() const;
    void clear_background();

    double fontsize;
    double rmargin;
    double indent;

    guint n;
    gint type;
    guint speed;
    guint draw_width, draw_height;
    guint render_counter;
    guint frames_per_unit;
    guint graph_dely;
    guint real_draw_height;
    double graph_delx;
    guint graph_buffer_offset;

    std::vector<GdkRGBA> colors;

    std::vector<double> data_block;
    double* data[NUM_POINTS];

    GtkBox *main_widget;
    GtkDrawingArea *disp;

    cairo_surface_t *background;

    guint timer_index;

    gboolean draw;

    LoadGraphLabels labels;
    GsmColorButton *mem_color_picker;
    GsmColorButton *swap_color_picker;

    /* union { */
    struct
    {
        guint now; /* 0 -> current, 1 -> last
                      now ^ 1 each time */
        /* times[now], times[now ^ 1] is last */
        guint64 times[2][GLIBTOP_NCPU][N_CPU_STATES];
    } cpu;

    struct
    {
        guint64 last_in, last_out;
        GTimeVal time;
        guint64 max;
        unsigned values[NUM_POINTS];
        size_t cur;
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
