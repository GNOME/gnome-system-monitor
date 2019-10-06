/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include <config.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <glibtop.h>
#include <glibtop/cpu.h>
#include <glibtop/mem.h>
#include <glibtop/swap.h>
#include <glibtop/netload.h>
#include <glibtop/netlist.h>
#include <glibtop/sysinfo.h>

#include "application.h"
#include "load-graph.h"
#include "util.h"
#include "legacy/gsm_color_button.h"
#include <iostream>
#include <stdlib.h>

//static cairo_surface_t *surface = NULL;

struct cr_surface{
    cairo_surface_t *surface = NULL;
};

cr_surface core_surface_array[32];

//function that obtains the dimensions of core graph, count number of core graph in x and y axis
int
get_cpu_core_matrix(int ncpu,int *matrix_dim){
    double rough_x_count = sqrt(ncpu);
    int est_x_matrix = (int)rough_x_count;
    if(rough_x_count==est_x_matrix){
        matrix_dim[0]=est_x_matrix;
        matrix_dim[1]=est_x_matrix;
        return 0;
    }
    int i;
    for(i=est_x_matrix; i <= ncpu; i++)
    {
        if (ncpu%i == 0)
        {
            matrix_dim[0]=i;
            matrix_dim[1]=ncpu/i;
            break;
        }
    }
    return 0;
}


//hack while glibtop does not have this information
void 
linux_cpu_info (struct LoadData *lData)
{
    char *buffer;
    char line_buffer[200];
    const glibtop_sysinfo * sysinfo = glibtop_get_sysinfo();
    lData->ncpu = sysinfo->ncpu;
    get_cpu_core_matrix(lData->ncpu,lData->ncpu_matrix);


    FILE *cpuinfo = fopen("/proc/cpuinfo", "rb");

    while (fgets(line_buffer, 200, cpuinfo) != NULL)
    {
        if (strstr(line_buffer, "model name") != NULL)
        {
            buffer = strchr(line_buffer, ':');
            if (buffer != NULL)
            {
                buffer++; // we want to look at what's _after_ the ':' 
            }
            else
            {
                //nothing
            }

            break;
        }
    }

    fclose(cpuinfo);

    //sprintf(buffer,"%i",lData->ncpu_matrix[0]); //get total number of cpu number
    gtk_label_set_text(GTK_LABEL(lData->cpu_model), buffer);

}

gfloat f (gfloat y)
{
    return -y;//return x*sin((y*x/M_PI)+M_PI);//y*50*sin ((x/M_PI)+M_PI);
}
    int bol_first_draw = 1;


void draw_cpu_graph(struct LoadData *lData,int s){
       //
    int h;
    int j;
    int i;
    GtkWidget *widget_core = GTK_WIDGET(lData->widget_surface_array[s].da);

    cairo_t *cr_core = cairo_create(core_surface_array[s].surface);
    cairo_set_source_rgba(cr_core, 1, 1, 1, 1);
    cairo_paint(cr_core);

    float width_core = gtk_widget_get_allocated_width(widget_core);
    float height_core = gtk_widget_get_allocated_height(widget_core);

    cairo_set_line_width(cr_core, 1);
    //printf("width: %lf\n",width);
    //cairo_set_line_cap(cr_core, CAIRO_LINE_CAP_ROUND);

    cairo_save(cr_core);


    //float box_dist = ((width_core - (lData->ncpu_matrix[0]*0.5) - ((lData->ncpu_matrix[0]-1)*5)) / lData->ncpu_matrix[0]);
    float box_dist = ((width_core) / lData->ncpu_matrix[0]);
    float box_lenght = ((width_core - ((lData->ncpu_matrix[0]-1)*5)) / lData->ncpu_matrix[0]);
    //float box_height = height_core;
    i = 0;
    
    //grid boxes for each cpu core
    //start of - make this a background so this will not be redrawn each time
    cairo_set_source_rgb(cr_core, 0.152941176, 0.545098039, 0.807843137);
    cairo_set_line_width(cr_core, 1);
    for (i = 0; i < 8; i++)
    {
        cairo_rectangle(cr_core, (box_dist * i), 0, box_lenght, height_core);
    }
    cairo_stroke(cr_core);
    cairo_set_source_rgba(cr_core, 0.376470588, 0.749019608, 1, 0.5);
    cairo_set_line_width(cr_core, 0.5);
    for (i = 0; i < lData->ncpu_matrix[0]; i++)
    {
        for (j = 1; j <= 5; j++)
        {
            cairo_move_to(cr_core, ((box_lenght / 5) * j) + box_dist * i, 0);
            cairo_line_to(cr_core, ((box_lenght / 5) * j) + box_dist * i, height_core);
        }
        for (j = 1; j <= 8; j++)
        {
            cairo_move_to(cr_core, 0.5 + box_dist * i, (10 * j) + 0.5);
            cairo_line_to(cr_core, box_lenght + 0.5 + box_dist * i, (10 * j) + 0.5);
        }
    } 
    cairo_stroke(cr_core);
    //end of - make this a background so this will not be redrawn each time

    cairo_set_source_rgb(cr_core, 0.176470588, 0.654901961, 0.97254902);

    cairo_translate(cr_core, 0.5, height_core);
    /* Link each data point */
    i = 0;
    for (i = 0; i < 62; i += 1)
    {
        cairo_line_to(cr_core, i * (box_lenght) / 62, f(lData->cpu_pulse[1+(s*lData->ncpu_matrix[0])][i] * (height_core - 0.5) / 100)); //i/exp(load/100)
    }

    /* Draw the curve */
    cairo_stroke(cr_core);

    //cairo_set_source_rgb(cr_core, 0.176470588, 0.654901961, 0.97254902);
    for (h = 2; h <= 8; h++)
    {
        cairo_translate(cr_core, box_dist, 0);
        i = 0;
        for (i = 0; i < 62; i += 1)
        {

            cairo_line_to(cr_core, i * (box_lenght) / 62, f(lData->cpu_pulse[h+(s*lData->ncpu_matrix[0])][i] * (height_core - 0.5) / 100)); //i/exp(load/100)
        }

        /* Draw the curve */
        cairo_stroke(cr_core);
    }

    //cairo_clip(cr_core);
    cairo_destroy(cr_core);

    gtk_widget_queue_draw(GTK_WIDGET(lData->widget_surface_array[s].da));
}

void
draw_total_cpu_graph(struct LoadData *lData){
    GtkWidget *widget = GTK_WIDGET(lData->da);
    static int ang = 0;

    cairo_t *cr = cairo_create(lData->cpu_total_surface );
    cairo_set_source_rgba(cr, 1, 1, 1,1);
    cairo_paint(cr);

    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);
    int x = (int)(width / 2.0 * (cos(3.1416 / 180 * ang) + 1));
    int y = (int)(height / 2.0 * (sin(3.1416 / 180 * ang) + 1));
    ang = (ang + 2) % 360;

    cairo_set_line_width(cr, 7.5);
    cairo_translate(cr, (width / 2), (height / 2));
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

    double angle1 = 120 * (M_PI / 180.0);   // angles are specified
    double angle2 = 60.0 * (M_PI / 180.0);  // in radians
    double angle3 = (120.000001+(300*lData->load[0]/100))* (M_PI / 180.0); // in radians
    
    cairo_save(cr);
    cairo_set_source_rgb(cr, 0.952941176, 0.952941176, 0.952941176);
    cairo_arc(cr, 0, 0, 82.0, angle1, angle2);
    cairo_stroke(cr);

    cairo_set_source_rgb(cr, 0.176470588, 0.654901961, 0.97254902);
    cairo_arc(cr, 0, 0, 82.0, angle1, angle3);
    cairo_stroke(cr);

    cairo_set_source_rgb(cr, 0.176470588, 0.654901961, 0.97254902);
    cairo_translate(cr, 37-(width / 2), -10);
    //cairo_scale (cr, ZOOM_X, -ZOOM_Y);

    /* Determine the data points to calculate (ie. those in the clipping zone */
    cairo_set_line_width (cr, 2);

    /* Link each data point */
    int i;

    for (i = 0; i < 62; i += 1){
        //ls = ;
        cairo_line_to (cr, i*1.75, f (lData->cpu_pulse[0][i])); //i/exp(load/100)
    }
    /* Draw the curve */

    cairo_stroke (cr);

    cairo_destroy(cr);

    gtk_widget_queue_draw(GTK_WIDGET(lData->da));
}

gboolean
on_timeout(
    struct LoadData *lData)
{
    draw_total_cpu_graph(lData);
    /*if (stop_timeout)
        return FALSE;*/
    int i;
    for(i=0;i<lData->ncpu_matrix[1];i++){
        draw_cpu_graph(lData,i);
    }

    return TRUE;
}

gboolean update_load_data(struct LoadData *lData)
{
    glibtop_cpu cpu;

    glibtop_get_cpu (&cpu);
    char buffer[50];

    //double load;
    int prev;
    if(lData->current - 1<0){
        prev = 61;
    }else{
        prev = lData->current - 1;
    }
    int i;
    for(i=0;i<62;i++)
    {
        lData->cpu_pulse[0][i]=   lData->cpu_pulse[0][i+1]; //move the data 1 step to the left
    }
    lData->n_cpu_total[0][lData->current] = cpu.total;
    lData->n_cpu_usage[0][lData->current] = cpu.user + cpu.nice + cpu.sys;
    lData->load[0] = 100*(lData->n_cpu_usage[0][lData->current] - lData->n_cpu_usage[0][prev])/(lData->n_cpu_total[0][lData->current] - lData->n_cpu_total[0][prev]);

    double dm = ((lData->load[0]*lData->current)/M_PI)+M_PI;
    lData->cpu_pulse[0][61] = 16*sqrt((lData->load[0]) / 80) * ((sin(2 * dm) + sin(3 * dm)));

    //for core
    int j;
    for(i=1;i<=lData->ncpu;i++)
    {
        for(j=0;j<62;j++)
        {
           lData->cpu_pulse[i][j]=   lData->cpu_pulse[i][j+1]; //move the data 1 step to the left
        }
        lData->n_cpu_total[i][lData->current] = cpu.xcpu_total[i-1];
        lData->n_cpu_usage[i][lData->current] = cpu.xcpu_user[i-1] + cpu.xcpu_nice[i-1] + cpu.xcpu_sys[i-1];
        lData->load[i] = 100*(lData->n_cpu_usage[i][lData->current] - lData->n_cpu_usage[i][prev])/(lData->n_cpu_total[i][lData->current] - lData->n_cpu_total[i][prev]);
        lData->cpu_pulse[i][61] = lData->load[i];
    }

    lData->current++;

    sprintf(buffer, "%.1f", lData->load[0]);
    gtk_misc_set_alignment(GTK_MISC(lData->cpu_total),1,0); 
    gtk_label_set_text(GTK_LABEL(lData->cpu_total), strcat(buffer, "%"));

    if(lData->current>61){
        lData->current = 0;
    }
    on_timeout(lData);
    return TRUE;
}

static gboolean
total_cpu_configure_event(
    GtkWidget *widget,
    GdkEventConfigure *event,
    struct LoadData *lData)
{
    static int save_w = 0, save_h = 0;

    if (save_w == event->width && save_h == event->height)
        return TRUE;

    save_w = event->width;
    save_h = event->height;

    if (lData->cpu_total_surface )
        cairo_surface_destroy(lData->cpu_total_surface );

    lData->cpu_total_surface  = gdk_window_create_similar_surface(
        gtk_widget_get_window(widget), CAIRO_CONTENT_COLOR,
        save_w, save_h);

    cairo_t *cr = cairo_create(lData->cpu_total_surface);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    cairo_destroy(cr);
    
    return TRUE;
}

static gboolean
total_cpu_draw(
    GtkWidget *widget,
    cairo_t *cr,
    struct LoadData *lData)
{
    cairo_set_source_surface(cr, lData->cpu_total_surface, 0, 0);
    cairo_paint(cr);
    return TRUE;
}

//solvable create its own struct containing its surface and the widget
static gboolean
cpu_core_configure_event(
    GtkWidget *widget,
    GdkEventConfigure *event,
    struct LoadData *lData)
{
    //cairo_surface_t *surface = userdat;
    static int save_w = 0, save_h = 0;

    if (save_w == event->width && save_h == event->height)
        return TRUE;

    save_w = event->width;
    save_h = event->height;

    if (core_surface_array[lData->surface_index].surface)
        cairo_surface_destroy(core_surface_array[lData->surface_index].surface);

    core_surface_array[lData->surface_index].surface  = gdk_window_create_similar_surface(
        gtk_widget_get_window(widget), CAIRO_CONTENT_COLOR,
        save_w, save_h);

    cairo_t *cr = cairo_create(core_surface_array[lData->surface_index].surface);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    cairo_destroy(cr);
    
    return TRUE;
}

static gboolean
cpu_core_draw(
    GtkWidget *widget,
    cairo_t *cr,
    struct LoadData *lData)
{
    //    cairo_surface_t *surface = userdat;
    cairo_set_source_surface(cr, core_surface_array[lData->surface_index].surface, 0, 0);
    cairo_paint(cr);
    return TRUE;
}

void start_standard_resource (struct LoadData *lData)
{

    gtk_widget_set_size_request(lData->da, 179, 179);
    //gtk_box_pack_start(GTK_BOX(box), da, TRUE, TRUE, 0);
    //place after cpu info

    g_signal_connect(lData->da, "draw",
                     G_CALLBACK(total_cpu_draw), lData);
    g_signal_connect(lData->da, "configure-event",
                     G_CALLBACK(total_cpu_configure_event), lData);

 
    lData->current = 0;

    linux_cpu_info (lData);
    lData->widget_surface_array = (widget_array *)malloc (lData->ncpu_matrix[1] * sizeof (widget_array));
    int i;
    for(i=0;i<lData->ncpu_matrix[1];i++){
        lData->widget_surface_array[i].da = (GtkWidget *)malloc (sizeof (GtkWidget));
        lData->widget_surface_array[i].da = GTK_WIDGET (GTK_DRAWING_AREA (gtk_drawing_area_new ()));
        gtk_widget_show (lData->widget_surface_array[i].da);
        //gtk_container_add(GTK_CONTAINER(lData->box_ncore),lData->widget_surface_array[0].da);
        gtk_box_pack_start (lData->box_ncore, lData->widget_surface_array[i].da, TRUE, TRUE, 2);
        //for cpu core graph
        lData->surface_index = i;
        g_signal_connect(lData->widget_surface_array[i].da, "draw",
                        G_CALLBACK(cpu_core_draw), lData);
        g_signal_connect(lData->widget_surface_array[i].da, "configure-event",
                        G_CALLBACK(cpu_core_configure_event), lData);  
    }
    //add gtk surface programatically
    g_timeout_add(1000, (GSourceFunc)update_load_data, lData);
}

void LoadGraph::clear_background()
{
    if (background) {
        cairo_surface_destroy (background);
        background = NULL;
    }
}


unsigned LoadGraph::num_bars() const
{
    unsigned n;

    // keep 100 % num_bars == 0
    switch (static_cast<int>(draw_height / (fontsize + 14)))
    {
        case 0:
        case 1:
            n = 1;
            break;
        case 2:
        case 3:
            n = 2;
            break;
        case 4:
            n = 4;
            break;
        default:
            n = 5;
    }

    return n;
}



#define FRAME_WIDTH 4
static void draw_background(LoadGraph *graph) {
    GtkAllocation allocation;
    cairo_t *cr;
    guint i;
    unsigned num_bars;
    char *caption;
    PangoLayout* layout;
    PangoFontDescription* font_desc;
    PangoRectangle extents;
    cairo_surface_t *surface;
    GdkRGBA fg;
    GdkRGBA fg_grid;
    double const border_alpha = 0.7;
    double const grid_alpha = border_alpha / 2.0;

    num_bars = graph->num_bars();
    graph->graph_dely = (graph->draw_height - 15) / num_bars; /* round to int to avoid AA blur */
    graph->real_draw_height = graph->graph_dely * num_bars;
    graph->graph_delx = (graph->draw_width - 2.0 - graph->indent) / (LoadGraph::NUM_POINTS - 3);
    graph->graph_buffer_offset = (int) (1.5 * graph->graph_delx) + FRAME_WIDTH ;

    gtk_widget_get_allocation (GTK_WIDGET (graph->disp), &allocation);
    surface = gdk_window_create_similar_surface (gtk_widget_get_window (GTK_WIDGET (graph->disp)),
                                                           CAIRO_CONTENT_COLOR_ALPHA,
                                                           allocation.width,
                                                           allocation.height);
    cr = cairo_create (surface);

    GtkStyleContext *context = gtk_widget_get_style_context (GTK_WIDGET (GsmApplication::get()->stack));
    
    gtk_style_context_get_color (context, GTK_STATE_FLAG_NORMAL, &fg);

    cairo_paint_with_alpha (cr, 0.0);
    layout = pango_cairo_create_layout (cr);
    gtk_style_context_get (context, GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
    pango_font_description_set_size (font_desc, 0.8 * graph->fontsize * PANGO_SCALE);
    pango_layout_set_font_description (layout, font_desc);
    pango_font_description_free (font_desc);

    /* draw frame */
    cairo_translate (cr, FRAME_WIDTH, FRAME_WIDTH);

    /* Draw background rectangle */
    /* When a user uses a dark theme, the hard-coded
     * white background in GSM is a lone white on the
     * display, which makes the user unhappy. To fix
     * this, here we offer the user a chance to set
     * his favorite background color. */
    gtk_style_context_save (context);

    /* Here we specify the name of the class. Now in
     * the theme's CSS we can specify the own colors
     * for this class. */
    gtk_style_context_add_class (context, "loadgraph");

    /* And in case the user does not care, we add
     * classes that usually have a white background. */
    gtk_style_context_add_class (context, GTK_STYLE_CLASS_PAPER);
    gtk_style_context_add_class (context, GTK_STYLE_CLASS_ENTRY);

    /* And, as a bonus, the user can choose the color of the grid. */
    gtk_style_context_get_color (context, GTK_STATE_FLAG_NORMAL, &fg_grid);

    /* Why not use the new features of the
     * GTK instead of cairo_rectangle ?! :) */
    gtk_render_background (context, cr, graph->indent, 0.0,
            graph->draw_width - graph->rmargin - graph->indent,
            graph->real_draw_height);

    gtk_style_context_restore (context);

    cairo_set_line_width (cr, 1.0);
    
    for (i = 0; i <= num_bars; ++i) {
        double y;

        if (i == 0)
            y = 0.5 + graph->fontsize / 2.0;
        else if (i == num_bars)
            y = i * graph->graph_dely + 0.5;
        else
            y = i * graph->graph_dely + graph->fontsize / 2.0;

        gdk_cairo_set_source_rgba (cr, &fg);
        if (graph->type == LOAD_GRAPH_NET) {
            // operation orders matters so it's 0 if i == num_bars
            guint64 rate = graph->net.max - (i * graph->net.max / num_bars);
            const std::string captionstr(procman::format_network_rate(rate));
            caption = g_strdup(captionstr.c_str());
        } else {
            // operation orders matters so it's 0 if i == num_bars
            caption = g_strdup_printf("%d %%", 100 - i * (100 / num_bars));
        }
        pango_layout_set_alignment (layout, PANGO_ALIGN_LEFT);
        pango_layout_set_text (layout, caption, -1);
        pango_layout_get_extents (layout, NULL, &extents);
        cairo_move_to (cr, graph->draw_width - graph->indent - 23,
                       y - 1.0 * extents.height / PANGO_SCALE / 2);
        pango_cairo_show_layout (cr, layout);
        g_free(caption);

        if (i==0 || i==num_bars)
            fg_grid.alpha = border_alpha;
        else
            fg_grid.alpha = grid_alpha;

        gdk_cairo_set_source_rgba (cr, &fg_grid);
        cairo_move_to (cr, graph->indent, i * graph->graph_dely + 0.5);
        cairo_line_to (cr, graph->draw_width - graph->rmargin + 0.5 + 4, i * graph->graph_dely + 0.5);
        cairo_stroke (cr);
    }
    

    const unsigned total_seconds = graph->speed * (LoadGraph::NUM_POINTS - 2) / 1000;

    for (unsigned int i = 0; i < 7; i++) {
        double x = (i) * (graph->draw_width - graph->rmargin - graph->indent) / 6;

        if (i==0 || i==6)
            fg_grid.alpha = border_alpha;
        else
            fg_grid.alpha = grid_alpha;

        gdk_cairo_set_source_rgba (cr, &fg_grid);
        cairo_move_to (cr, (ceil(x) + 0.5) + graph->indent, 0.5);
        cairo_line_to (cr, (ceil(x) + 0.5) + graph->indent, graph->real_draw_height + 4.5);
        cairo_stroke(cr);
        unsigned seconds = total_seconds - i * total_seconds / 6;
        const char* format;
        if (i == 0)
            format = dngettext(GETTEXT_PACKAGE, "%u second", "%u seconds", seconds);
        else
            format = "%u";
        caption = g_strdup_printf(format, seconds);
        pango_layout_set_text (layout, caption, -1);
        pango_layout_get_extents (layout, NULL, &extents);
        cairo_move_to (cr,
                       (ceil(x) + 0.5 + graph->indent) - (1.0 * extents.width / PANGO_SCALE / 2),
                       graph->draw_height - 1.0 * extents.height / PANGO_SCALE);
        gdk_cairo_set_source_rgba (cr, &fg);
        pango_cairo_show_layout (cr, layout);
        g_free (caption);
    }
    g_object_unref(layout);
    cairo_stroke (cr);
    cairo_destroy (cr);
    graph->background = surface;
}

/* Redraws the backing buffer for the load graph and updates the window */
void
load_graph_queue_draw (LoadGraph *graph)
{
    /* repaint */
    gtk_widget_queue_draw (GTK_WIDGET (graph->disp));
}

static int load_graph_update (gpointer user_data); // predeclare load_graph_update so we can compile ;)

static gboolean
load_graph_configure (GtkWidget *widget,
                      GdkEventConfigure *event,
                      gpointer data_ptr)
{
    GtkAllocation allocation;
    LoadGraph * const graph = static_cast<LoadGraph*>(data_ptr);

    gtk_widget_get_allocation (widget, &allocation);
    graph->draw_width = allocation.width - 2 * FRAME_WIDTH;
    graph->draw_height = allocation.height - 2 * FRAME_WIDTH;

    graph->clear_background();

    load_graph_queue_draw (graph);

    return TRUE;
}

static void force_refresh (LoadGraph * const graph)
{
    graph->clear_background();
    load_graph_queue_draw (graph);
}

static void
load_graph_style_updated (GtkWidget *widget,
                          gpointer data_ptr)
{
    LoadGraph * const graph = static_cast<LoadGraph*>(data_ptr);
    force_refresh (graph);
}

static gboolean
load_graph_state_changed (GtkWidget *widget,
                      GtkStateFlags *flags,
                      gpointer data_ptr)
{
    LoadGraph * const graph = static_cast<LoadGraph*>(data_ptr);
    force_refresh (graph);
    return TRUE;
}

static gboolean
load_graph_draw (GtkWidget *widget,
                 cairo_t * cr,
                 gpointer data_ptr)
{
    LoadGraph * const graph = static_cast<LoadGraph*>(data_ptr);

    guint i;
    gint j;
    gdouble sample_width, x_offset;

    /* Number of pixels wide for one graph point */
    sample_width = (float)(graph->draw_width - graph->rmargin - graph->indent) / (float)LoadGraph::NUM_POINTS;
    /* General offset */
    x_offset = graph->draw_width - graph->rmargin;

    /* Subframe offset */
    x_offset += graph->rmargin - ((sample_width / graph->frames_per_unit) * graph->render_counter);

    /* draw the graph */

    if (graph->background == NULL) {
        draw_background(graph);
    }
    cairo_set_source_surface (cr, graph->background, 0, 0);
    cairo_paint (cr);

    cairo_set_line_width (cr, 1);
    cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
    cairo_rectangle (cr, graph->indent + FRAME_WIDTH + 1, FRAME_WIDTH - 1,
                     graph->draw_width - graph->rmargin - graph->indent - 1,
                     graph->real_draw_height + FRAME_WIDTH - 1);
    cairo_clip(cr);

    bool drawStacked = graph->type == LOAD_GRAPH_CPU && GsmApplication::get()->config.draw_stacked;
    bool drawSmooth = graph->type != LOAD_GRAPH_CPU || GsmApplication::get()->config.draw_smooth;
    for (j = graph->n-1; j >= 0; j--) {
        gdk_cairo_set_source_rgba (cr, &(graph->colors [j]));
        if (drawStacked) {
            cairo_move_to (cr, x_offset, graph->real_draw_height + 3.5f);
        } else {
            cairo_move_to (cr, x_offset, (1.0f - graph->data[0][j]) * graph->real_draw_height + 3.5f);
        }
        for (i = 1; i < LoadGraph::NUM_POINTS; ++i) {
            if (graph->data[i][j] == -1.0f)
                continue;
            if (drawSmooth) {
              cairo_curve_to (cr,
                              x_offset - ((i - 0.5f) * graph->graph_delx),
                              (1.0 - graph->data[i-1][j]) * graph->real_draw_height + 3.5,
                              x_offset - ((i - 0.5f) * graph->graph_delx),
                              (1.0 - graph->data[i][j]) * graph->real_draw_height + 3.5,
                              x_offset - (i * graph->graph_delx),
                              (1.0 - graph->data[i][j]) * graph->real_draw_height + 3.5);
            } else {
              cairo_line_to (cr, x_offset - (i * graph->graph_delx),
                              (1.0 - graph->data[i][j]) * graph->real_draw_height + 3.5);
            }

        }
        if (drawStacked) {
            cairo_rel_line_to (cr, 0, graph->real_draw_height + 3.5f);
            //cairo_stroke_preserve(cr);
            //cairo_close_path(cr);
            cairo_fill(cr);
        } else {
            cairo_stroke (cr);
        }
    }

    return TRUE;
}

void
load_graph_reset (LoadGraph *graph)
{
    std::fill(graph->data_block.begin(), graph->data_block.end(), -1.0);
}

static void
get_load (LoadGraph *graph)
{
    guint i;
    glibtop_cpu cpu;

    glibtop_get_cpu (&cpu);

#undef NOW
#undef LAST
#define NOW  (graph->cpu.times[graph->cpu.now])
#define LAST (graph->cpu.times[graph->cpu.now ^ 1])

    if (graph->n == 1) {
        NOW[0][CPU_TOTAL] = cpu.total;
        NOW[0][CPU_USED] = cpu.user + cpu.nice + cpu.sys;
    } else {
        for (i = 0; i < graph->n; i++) {
            NOW[i][CPU_TOTAL] = cpu.xcpu_total[i];
            NOW[i][CPU_USED] = cpu.xcpu_user[i] + cpu.xcpu_nice[i]
                + cpu.xcpu_sys[i];
        }
    }

    // on the first call, LAST is 0
    // which means data is set to the average load since boot
    // that value has no meaning, we just want all the
    // graphs to be aligned, so the CPU graph needs to start
    // immediately
    bool drawStacked = graph->type == LOAD_GRAPH_CPU && GsmApplication::get()->config.draw_stacked;

    for (i = 0; i < graph->n; i++) {
        float load;
        float total, used;
        gchar *text;

        total = NOW[i][CPU_TOTAL] - LAST[i][CPU_TOTAL];
        used  = NOW[i][CPU_USED]  - LAST[i][CPU_USED];

        load = used / MAX(total, 1.0f);
        graph->data[0][i] = load;
        if (drawStacked) {
            graph->data[0][i] /= graph->n;
            if (i > 0) {
                graph->data[0][i] += graph->data[0][i-1];
            }
        }

        /* Update label */
        text = g_strdup_printf("%.1f%%", load * 100.0f);
        gtk_label_set_text(GTK_LABEL(graph->labels.cpu[i]), text);
        g_free(text);
    }

    graph->cpu.now ^= 1;

#undef NOW
#undef LAST
}


namespace
{

    void set_memory_label_and_picker(GtkLabel* label, GsmColorButton* picker,
                                     guint64 used, guint64 total, double percent)
    {
        char* used_text;
        char* total_text;
        char* text;

        used_text = g_format_size_full(used, G_FORMAT_SIZE_IEC_UNITS);
        total_text = g_format_size_full(total, G_FORMAT_SIZE_IEC_UNITS);
        if (total == 0) {
            text = g_strdup(_("not available"));
        } else {
            // xgettext: 540MiB (53 %) of 1.0 GiB
            text = g_strdup_printf(_("%s (%.1f%%) of %s"), used_text, 100.0 * percent, total_text);
        }
        gtk_label_set_text(label, text);
        g_free(used_text);
        g_free(total_text);
        g_free(text);

        if (picker)
            gsm_color_button_set_fraction(picker, percent);
    }
}

static void
get_memory (LoadGraph *graph)
{
    float mempercent, swappercent;

    glibtop_mem mem;
    glibtop_swap swap;

    glibtop_get_mem (&mem);
    glibtop_get_swap (&swap);

    /* There's no swap on LiveCD : 0.0f is better than NaN :) */
    swappercent = (swap.total ? (float)swap.used / (float)swap.total : 0.0f);
    mempercent  = (float)mem.user  / (float)mem.total;
    set_memory_label_and_picker(GTK_LABEL(graph->labels.memory),
                                GSM_COLOR_BUTTON(graph->mem_color_picker),
                                mem.user, mem.total, mempercent);

    set_memory_label_and_picker(GTK_LABEL(graph->labels.swap),
                                GSM_COLOR_BUTTON(graph->swap_color_picker),
                                swap.used, swap.total, swappercent);
    
    gtk_widget_set_sensitive (GTK_WIDGET (graph->swap_color_picker), swap.total > 0);
    
    graph->data[0][0] = mempercent;
    graph->data[0][1] = swap.total>0 ? swappercent : -1.0;
}

/* Nice Numbers for Graph Labels after Paul Heckbert
   nicenum: find a "nice" number approximately equal to x.
   Round the number if round=1, take ceiling if round=0    */

static double
nicenum (double x, int round)
{
    int expv;				/* exponent of x */
    double f;				/* fractional part of x */
    double nf;				/* nice, rounded fraction */

    expv = floor(log10(x));
    f = x/pow(10.0, expv);		/* between 1 and 10 */
    if (round) {
        if (f < 1.5)
            nf = 1.0;
        else if (f < 3.0)
            nf = 2.0;
        else if (f < 7.0)
            nf = 5.0;
        else
            nf = 10.0;
    } else {
        if (f <= 1.0)
            nf = 1.0;
        else if (f <= 2.0)
            nf = 2.0;
        else if (f <= 5.0)
            nf = 5.0;
        else
            nf = 10.0;
    }
    return nf * pow(10.0, expv);
}

static void
net_scale (LoadGraph *graph, guint64 din, guint64 dout)
{
    graph->data[0][0] = 1.0f * din / graph->net.max;
    graph->data[0][1] = 1.0f * dout / graph->net.max;

    guint64 dmax = std::max(din, dout);
    graph->net.values[graph->net.cur] = dmax;
    graph->net.cur = (graph->net.cur + 1) % LoadGraph::NUM_POINTS;

    guint64 new_max;
    // both way, new_max is the greatest value
    if (dmax >= graph->net.max)
        new_max = dmax;
    else
        new_max = *std::max_element(&graph->net.values[0],
                                    &graph->net.values[LoadGraph::NUM_POINTS]);

    //
    // Round network maximum
    //

    const guint64 bak_max(new_max);

    if (GsmApplication::get()->config.network_in_bits) {
        // nice number is for the ticks
        unsigned ticks = graph->num_bars();

        // gets messy at low values due to division by 8
        guint64 bit_max = std::max( new_max*8, G_GUINT64_CONSTANT(10000) );

        // our tick size leads to max
        double d = nicenum(bit_max/ticks, 0);
        bit_max = ticks * d;
        new_max = bit_max / 8;

        procman_debug("bak*8 %" G_GUINT64_FORMAT ", ticks %d, d %f"
                      ", bit_max %" G_GUINT64_FORMAT ", new_max %" G_GUINT64_FORMAT,
                      bak_max*8, ticks, d, bit_max, new_max );
    } else {
        // round up to get some extra space
        // yes, it can overflow
        new_max = 1.1 * new_max;
        // make sure max is not 0 to avoid / 0
        // default to 1 KiB
        new_max = std::max(new_max, G_GUINT64_CONSTANT(1024));

        // decompose new_max = coef10 * 2**(base10 * 10)
        // where coef10 and base10 are integers and coef10 < 2**10
        //
        // e.g: ceil(100.5 KiB) = 101 KiB = 101 * 2**(1 * 10)
        //      where base10 = 1, coef10 = 101, pow2 = 16

        guint64 pow2 = std::floor(log2(new_max));
        guint64 base10 = pow2 / 10.0;
        guint64 coef10 = std::ceil(new_max / double(G_GUINT64_CONSTANT(1) << (base10 * 10)));
        g_assert(new_max <= (coef10 * (G_GUINT64_CONSTANT(1) << (base10 * 10))));

        // then decompose coef10 = x * 10**factor10
        // where factor10 is integer and x < 10
        // so we new_max has only 1 significant digit

        guint64 factor10 = std::pow(10.0, std::floor(std::log10(coef10)));
        coef10 = std::ceil(coef10 / double(factor10)) * factor10;

        new_max = coef10 * (G_GUINT64_CONSTANT(1) << guint64(base10 * 10));
        procman_debug("bak %" G_GUINT64_FORMAT " new_max %" G_GUINT64_FORMAT
                      "pow2 %" G_GUINT64_FORMAT " coef10 %" G_GUINT64_FORMAT,
                      bak_max, new_max, pow2, coef10);
    }

    if (bak_max > new_max) {
        procman_debug("overflow detected: bak=%" G_GUINT64_FORMAT
                      " new=%" G_GUINT64_FORMAT,
                      bak_max, new_max);
        new_max = bak_max;
    }

    // if max is the same or has decreased but not so much, don't
    // do anything to avoid rescaling
    if ((0.8 * graph->net.max) < new_max && new_max <= graph->net.max)
        return;

    const double scale = 1.0f * graph->net.max / new_max;

    for (size_t i = 0; i < LoadGraph::NUM_POINTS; i++) {
        if (graph->data[i][0] >= 0.0f) {
            graph->data[i][0] *= scale;
            graph->data[i][1] *= scale;
        }
    }

    procman_debug("rescale dmax = %" G_GUINT64_FORMAT
                  " max = %" G_GUINT64_FORMAT
                  " new_max = %" G_GUINT64_FORMAT,
                  dmax, graph->net.max, new_max);

    graph->net.max = new_max;

    // force the graph background to be redrawn now that scale has changed
    graph->clear_background();
}

static void
get_net (LoadGraph *graph)
{
    glibtop_netlist netlist;
    char **ifnames;
    guint32 i;
    guint64 in = 0, out = 0;
    GTimeVal time;
    guint64 din, dout;
    gboolean first = true;
    ifnames = glibtop_get_netlist(&netlist);

    for (i = 0; i < netlist.number; ++i)
    {
        glibtop_netload netload;
        glibtop_get_netload (&netload, ifnames[i]);

        if (netload.if_flags & (1 << GLIBTOP_IF_FLAGS_LOOPBACK))
            continue;

        /* Skip interfaces without any IPv4/IPv6 address (or
           those with only a LINK ipv6 addr) However we need to
           be able to exclude these while still keeping the
           value so when they get online (with NetworkManager
           for example) we don't get a suddent peak.  Once we're
           able to get this, ignoring down interfaces will be
           possible too.  */
        if (not (netload.flags & (1 << GLIBTOP_NETLOAD_ADDRESS6)
                 and netload.scope6 != GLIBTOP_IF_IN6_SCOPE_LINK)
            and not (netload.flags & (1 << GLIBTOP_NETLOAD_ADDRESS)))
            continue;

        /* Don't skip interfaces that are down (GLIBTOP_IF_FLAGS_UP)
           to avoid spikes when they are brought up */

        in  += netload.bytes_in;
        out += netload.bytes_out;
    }

    g_strfreev(ifnames);

    g_get_current_time (&time);

    if (in >= graph->net.last_in && out >= graph->net.last_out && graph->net.time.tv_sec != 0) {
        float dtime;
        dtime = time.tv_sec - graph->net.time.tv_sec +
                (double) (time.tv_usec - graph->net.time.tv_usec) / G_USEC_PER_SEC;
        din   = static_cast<guint64>((in  - graph->net.last_in)  / dtime);
        dout  = static_cast<guint64>((out - graph->net.last_out) / dtime);
    } else {
        /* Don't calc anything if new data is less than old (interface
           removed, counters reset, ...) or if it is the first time */
        din  = 0;
        dout = 0;
    }

    first = first && (graph->net.time.tv_sec==0);
    graph->net.last_in  = in;
    graph->net.last_out = out;
    graph->net.time     = time;

    if (!first)
        net_scale(graph, din, dout);

    gtk_label_set_text (GTK_LABEL (graph->labels.net_in), procman::format_network_rate(din).c_str());
    gtk_label_set_text (GTK_LABEL (graph->labels.net_in_total), procman::format_network(in).c_str());

    gtk_label_set_text (GTK_LABEL (graph->labels.net_out), procman::format_network_rate(dout).c_str());
    gtk_label_set_text (GTK_LABEL (graph->labels.net_out_total), procman::format_network(out).c_str());
}


/* Updates the load graph when the timeout expires */
static gboolean
load_graph_update (gpointer user_data)
{
    return false;
    LoadGraph * const graph = static_cast<LoadGraph*>(user_data);

    if (graph->render_counter == graph->frames_per_unit - 1) {
        std::rotate(&graph->data[0],
                    &graph->data[LoadGraph::NUM_POINTS - 1],
                    &graph->data[LoadGraph::NUM_POINTS]);

        switch (graph->type) {
            case LOAD_GRAPH_CPU:
                get_load(graph);
                break;
            case LOAD_GRAPH_MEM:
                get_memory(graph);
                break;
            case LOAD_GRAPH_NET:
                get_net(graph);
                break;
            default:
                g_assert_not_reached();
        }
    }

    if (graph->draw)
        load_graph_queue_draw (graph);

    graph->render_counter++;

    if (graph->render_counter >= graph->frames_per_unit)
        graph->render_counter = 0;

    return TRUE;
}



LoadGraph::~LoadGraph()
{
    load_graph_stop(this);

    if (timer_index)
        g_source_remove(timer_index);

    clear_background();
}



static gboolean
load_graph_destroy (GtkWidget *widget, gpointer data_ptr)
{
    LoadGraph * const graph = static_cast<LoadGraph*>(data_ptr);

    delete graph;

    return FALSE;
}


LoadGraph::LoadGraph(guint type)
    : fontsize(8.0),
      rmargin(7 * fontsize),
      indent(24.0),
      n(0),
      type(type),
      speed(0),
      draw_width(0),
      draw_height(0),
      render_counter(0),
      frames_per_unit(10), // this will be changed but needs initialising
      graph_dely(0),
      real_draw_height(0),
      graph_delx(0.0),
      graph_buffer_offset(0),
      colors(),
      data_block(),
      main_widget(NULL),
      disp(NULL),
      background(NULL),
      timer_index(0),
      draw(FALSE),
      labels(),
      mem_color_picker(NULL),
      swap_color_picker(NULL),
      cpu(),
      net()
{
    LoadGraph * const graph = this;

    // FIXME:
    // on configure, graph->frames_per_unit = graph->draw_width/(LoadGraph::NUM_POINTS);
    // knock FRAMES down to 5 until cairo gets faster

    switch (type) {
        case LOAD_GRAPH_CPU:
            memset(&cpu, 0, sizeof cpu);
            n = GsmApplication::get()->config.num_cpus;

            for(guint i = 0; i < G_N_ELEMENTS(labels.cpu); ++i)
                labels.cpu[i] = GTK_LABEL (gtk_label_new(NULL));

            break;

        case LOAD_GRAPH_MEM:
            n = 2;
            labels.memory = GTK_LABEL (gtk_label_new(NULL));
            gtk_widget_set_valign (GTK_WIDGET (labels.memory), GTK_ALIGN_CENTER);
            gtk_widget_set_halign (GTK_WIDGET (labels.memory), GTK_ALIGN_START);
            gtk_widget_show (GTK_WIDGET (labels.memory));
            labels.swap = GTK_LABEL (gtk_label_new(NULL));
            gtk_widget_set_valign (GTK_WIDGET (labels.swap), GTK_ALIGN_CENTER);
            gtk_widget_set_halign (GTK_WIDGET (labels.swap), GTK_ALIGN_START);
            gtk_widget_show (GTK_WIDGET (labels.swap));
            break;

        case LOAD_GRAPH_NET:
            memset(&net, 0, sizeof net);
            n = 2;
            net.max = 1;
            labels.net_in = GTK_LABEL (gtk_label_new(NULL));
            gtk_label_set_width_chars(labels.net_in, 10);
            gtk_widget_set_valign (GTK_WIDGET (labels.net_in), GTK_ALIGN_CENTER);
            gtk_widget_set_halign (GTK_WIDGET (labels.net_in), GTK_ALIGN_END);
            gtk_widget_show (GTK_WIDGET (labels.net_in));

            labels.net_in_total = GTK_LABEL (gtk_label_new(NULL));
            gtk_widget_set_valign (GTK_WIDGET (labels.net_in_total), GTK_ALIGN_CENTER);
            gtk_widget_set_halign (GTK_WIDGET (labels.net_in_total), GTK_ALIGN_END);
            gtk_label_set_width_chars(labels.net_in_total, 10);
            gtk_widget_show (GTK_WIDGET (labels.net_in_total));

            labels.net_out = GTK_LABEL (gtk_label_new(NULL));
            gtk_widget_set_valign (GTK_WIDGET (labels.net_out), GTK_ALIGN_CENTER);
            gtk_widget_set_halign (GTK_WIDGET (labels.net_out), GTK_ALIGN_END);
            gtk_label_set_width_chars(labels.net_out, 10);
            gtk_widget_show (GTK_WIDGET (labels.net_out));

            labels.net_out_total = GTK_LABEL (gtk_label_new(NULL));
            gtk_widget_set_valign (GTK_WIDGET (labels.net_out_total), GTK_ALIGN_CENTER);
            gtk_widget_set_halign (GTK_WIDGET (labels.net_out), GTK_ALIGN_END);
            gtk_label_set_width_chars(labels.net_out_total, 10);
            gtk_widget_show (GTK_WIDGET (labels.net_out_total));

            break;
    }

    speed  = GsmApplication::get()->config.graph_update_interval;

    colors.resize(n);

    switch (type) {
        case LOAD_GRAPH_CPU:
            memcpy(&colors[0], GsmApplication::get()->config.cpu_color,
                   n * sizeof colors[0]);
            break;
        case LOAD_GRAPH_MEM:
            colors[0] = GsmApplication::get()->config.mem_color;
            colors[1] = GsmApplication::get()->config.swap_color;
            mem_color_picker = gsm_color_button_new (&colors[0],
                                                        GSMCP_TYPE_PIE);
            swap_color_picker = gsm_color_button_new (&colors[1],
                                                         GSMCP_TYPE_PIE);
            break;
        case LOAD_GRAPH_NET:
            colors[0] = GsmApplication::get()->config.net_in_color;
            colors[1] = GsmApplication::get()->config.net_out_color;
            break;
    }

    timer_index = 0;
    render_counter = (frames_per_unit - 1);
    draw = FALSE;

    main_widget = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 6));
    gtk_widget_set_size_request(GTK_WIDGET (main_widget), -1, LoadGraph::GRAPH_MIN_HEIGHT);
    gtk_widget_show (GTK_WIDGET (main_widget));

    disp = GTK_DRAWING_AREA (gtk_drawing_area_new ());
    gtk_widget_show (GTK_WIDGET (disp));
    g_signal_connect (G_OBJECT (disp), "draw",
                      G_CALLBACK (load_graph_draw), graph);
    g_signal_connect (G_OBJECT(disp), "configure_event",
                      G_CALLBACK (load_graph_configure), graph);
    g_signal_connect (G_OBJECT(disp), "destroy",
                      G_CALLBACK (load_graph_destroy), graph);
    g_signal_connect (G_OBJECT(disp), "state-flags-changed",
                      G_CALLBACK (load_graph_state_changed), graph);
    g_signal_connect (G_OBJECT(disp), "style-updated",
                      G_CALLBACK (load_graph_style_updated), graph);

    gtk_widget_set_events (GTK_WIDGET (disp), GDK_EXPOSURE_MASK);

    gtk_box_pack_start (main_widget, GTK_WIDGET (disp), TRUE, TRUE, 0);


    /* Allocate data in a contiguous block */
    data_block = std::vector<double>(n * LoadGraph::NUM_POINTS, -1.0);

    for (guint i = 0; i < LoadGraph::NUM_POINTS; ++i)
        data[i] = &data_block[0] + i * n;

    gtk_widget_show_all (GTK_WIDGET (main_widget));
}

void
load_graph_start (LoadGraph *graph)
{
     if (!graph->timer_index) {

        load_graph_update(graph);

        graph->timer_index = g_timeout_add (graph->speed / graph->frames_per_unit,
                                            load_graph_update,
                                            graph);
    }

    graph->draw = TRUE;
}

void
load_graph_stop (LoadGraph *graph)
{
    /* don't draw anymore, but continue to poll */
    graph->draw = FALSE;
}

void
load_graph_change_speed (LoadGraph *graph,
                         guint new_speed)
{
    if (graph->speed == new_speed)
        return;

    graph->speed = new_speed;

    if (graph->timer_index) {
        g_source_remove (graph->timer_index);
        graph->timer_index = g_timeout_add (graph->speed / graph->frames_per_unit,
                                            load_graph_update,
                                            graph);
    }

    graph->clear_background();
}


LoadGraphLabels*
load_graph_get_labels (LoadGraph *graph)
{
    return &graph->labels;
}

GtkBox*
load_graph_get_widget (LoadGraph *graph)
{
    return graph->main_widget;
}

GsmColorButton*
load_graph_get_mem_color_picker(LoadGraph *graph)
{
    return graph->mem_color_picker;
}

GsmColorButton*
load_graph_get_swap_color_picker(LoadGraph *graph)
{
    return graph->swap_color_picker;
}
