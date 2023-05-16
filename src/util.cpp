/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include <config.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <random>
#include <tuple>
#include <vector>

#include <glib/gi18n.h>
#include <glib.h>
#include <gtk/gtk.h>

#include <glibtop/proctime.h>
#include <glibtop/procstate.h>

#include "util.h"
#include "application.h"

extern "C" {
#include "legacy/e_date.h"
}


const char*
format_process_state(guint state)
{
    const char *status;

    switch (state)
    {
        case GLIBTOP_PROCESS_RUNNING:
            status = _("Running");
            break;

        case GLIBTOP_PROCESS_STOPPED:
            status = _("Stopped");
            break;

        case GLIBTOP_PROCESS_ZOMBIE:
            status = _("Zombie");
            break;

        case GLIBTOP_PROCESS_UNINTERRUPTIBLE:
            status = _("Uninterruptible");
            break;

        default:
            status = _("Sleeping");
            break;
    }

    return status;
}



static char *
mnemonic_safe_process_name(const char *process_name)
{
    const char *p;
    GString *name;

    name = g_string_new ("");

    for(p = process_name; *p; ++p)
    {
        g_string_append_c (name, *p);

        if(*p == '_')
            g_string_append_c (name, '_');
    }

    return g_string_free (name, FALSE);
}



static inline unsigned divide(unsigned *q, unsigned *r, unsigned d)
{
    *q = *r / d;
    *r = *r % d;
    return *q != 0;
}


/*
 * @param d: duration in centiseconds
 * @type d: unsigned
 */
char *
procman::format_duration_for_display(unsigned centiseconds)
{
    unsigned weeks = 0, days = 0, hours = 0, minutes = 0, seconds = 0;

    (void)(divide(&seconds, &centiseconds, 100)
           && divide(&minutes, &seconds, 60)
           && divide(&hours, &minutes, 60)
           && divide(&days, &hours, 24)
           && divide(&weeks, &days, 7));

    if (weeks)
        /* xgettext: weeks, days */
        return g_strdup_printf(_("%uw%ud"), weeks, days);

    if (days)
        /* xgettext: days, hours (0 -> 23) */
        return g_strdup_printf(_("%ud%02uh"), days, hours);

    if (hours)
        /* xgettext: hours (0 -> 23), minutes, seconds */
        return g_strdup_printf(_("%u:%02u:%02u"), hours, minutes, seconds);

    /* xgettext: minutes, seconds, centiseconds */
    return g_strdup_printf(_("%u:%02u.%02u"), minutes, seconds, centiseconds);
}



GtkLabel*
procman_make_label_for_mmaps_or_ofiles(const char *format,
                                       const char *process_name,
                                       unsigned pid)
{
    GtkLabel* label;
    char *name, *title;

    name = mnemonic_safe_process_name (process_name);
    title = g_strdup_printf(format, name, pid);
    label = GTK_LABEL (gtk_label_new_with_mnemonic (title));
    gtk_widget_set_valign (GTK_WIDGET (label), GTK_ALIGN_CENTER);
    gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_START);

    g_free (title);
    g_free (name);

    return label;
}



/**
 * procman::format_size:
 * @size:
 *
 * Formats the file size passed in @bytes in a way that is easy for
 * the user to read. Gives the size in bytes, kibibytes, mebibytes or
 * gibibytes, choosing whatever is appropriate.
 *
 * Returns: a newly allocated string with the size ready to be shown.
 **/

gchar*
procman::format_size(guint64 size, bool want_bits)
{
    if (want_bits)
        size *= 8;

    const GFormatSizeFlags flags = (want_bits ? G_FORMAT_SIZE_BITS : G_FORMAT_SIZE_IEC_UNITS);
    return g_format_size_full(size, flags);
}

gchar *
procman::get_nice_level (gint nice)
{
    if (nice < -7)
        return _("Very High");
    else if (nice < -2)
        return _("High");
    else if (nice < 3)
        return _("Normal");
    else if (nice < 7)
        return _("Low");
    else
        return _("Very Low");
}

gchar *
procman::get_nice_level_with_priority (gint nice)
{
    if (nice < -7)
        return _("Very High Priority");
    else if (nice < -2)
        return _("High Priority");
    else if (nice < 3)
        return _("Normal Priority");
    else if (nice < 7)
        return _("Low Priority");
    else
        return _("Very Low Priority");
}

gchar*
format_byte_size(guint64 size, bool want_iec_format)
{
    const GFormatSizeFlags flags = (want_iec_format ? G_FORMAT_SIZE_IEC_UNITS : G_FORMAT_SIZE_DEFAULT);
    return g_format_size_full(size, flags);
}

gboolean
load_symbols(const char *module, ...)
{
    GModule *mod;
    gboolean found_all = TRUE;
    va_list args;

    mod = g_module_open(module, static_cast<GModuleFlags>(G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL));

    if (!mod)
        return FALSE;

    procman_debug("Found %s", module);

    va_start(args, module);

    while (1) {
        const char *name;
        void **symbol;

        name = va_arg(args, char*);

        if (!name)
            break;

        symbol = va_arg(args, void**);

        if (g_module_symbol(mod, name, symbol)) {
            procman_debug("Loaded %s from %s", name, module);
        }
        else {
            procman_debug("Could not load %s from %s", name, module);
            found_all = FALSE;
            break;
        }
    }

    va_end(args);


    if (found_all)
        g_module_make_resident(mod);
    else
        g_module_close(mod);

    return found_all;
}


static gboolean
is_debug_enabled(void)
{
    static gboolean init;
    static gboolean enabled;

    if (!init) {
        enabled = g_getenv("GNOME_SYSTEM_MONITOR_DEBUG") != NULL;
        init = TRUE;
    }

    return enabled;
}


static double
get_relative_time(void)
{
    static unsigned long start_time;

    if (G_UNLIKELY(!start_time)) {
        glibtop_proc_time buf;
        glibtop_get_proc_time(&buf, getpid());
        start_time = buf.start_time;
    }

    return 1e-6 * g_get_monotonic_time () - start_time;
}

static gdouble
get_size_from_column(GtkTreeModel* model, GtkTreeIter* first,
                             const guint index)
{
    GValue value = { 0 };
    gtk_tree_model_get_value(model, first, index, &value);

    gdouble size;
    switch (G_VALUE_TYPE(&value)) {
        case G_TYPE_UINT:
            size = g_value_get_uint(&value);
            break;
        case G_TYPE_ULONG:
            size = g_value_get_ulong(&value);
            break;
        case G_TYPE_UINT64:
            size = g_value_get_uint64(&value);
            break;
        case G_TYPE_DOUBLE:
            size = g_value_get_double(&value);
            break;
        default:
            g_assert_not_reached();
    }

    g_value_unset(&value);
    return size;
}

void
procman_debug_real(const char *file, int line, const char *func,
                   const char *format, ...)
{
    va_list args;
    char *msg;

    if (G_LIKELY(!is_debug_enabled()))
        return;

    va_start(args, format);
    msg = g_strdup_vprintf(format, args);
    va_end(args);

    g_print ("[%.3f %s:%d %s] %s\n", get_relative_time(), file, line, func, msg);

    g_free(msg);
}


// h is in [0.0; 1.0] (and not [0°; 360°] )
// s is in [0.0; 1.0]
// v is in [0.0; 1.0]
// https://en.wikipedia.org/wiki/HSL_and_HSV#From_HSV
std::tuple<double, double, double> hsv_to_rgb(double h, double s, double v)
{
    const double c = v * s;
    const double hp = h * 6; // 360° / 60°
    const double x = c * (1 - std::abs(std::fmod(hp, 2.0) - 1));

    double r1 = 0 , g1 = 0, b1 = 0;

    switch (int(hp)) {
    case 0: r1 = c; g1 = x; b1 = 0; break;
    case 1: r1 = x; g1 = c; b1 = 0; break;
    case 2: r1 = 0; g1 = c; b1 = x; break;
    case 3: r1 = 0; g1 = x; b1 = c; break;
    case 4: r1 = x; g1 = 0; b1 = c; break;
    case 5: r1 = c; g1 = 0; b1 = x; break;
    }

    const double m = v - c;

    return {r1 + m, g1 + m, b1 + m};
}


std::string rgb_to_color_string(const std::tuple<double, double, double> &t)
{
    char b[14];
    auto c = [](double d) { return (unsigned short)(0xffff * d); };
    std::snprintf(b, sizeof b, "#%04x%04x%04x", c(std::get<0>(t)), c(std::get<1>(t)), c(std::get<2>(t)));
    return {b, 13};
}


namespace procman
{

    // http://martin.ankerl.com/2009/12/09/how-to-create-random-colors-programmatically/
    std::vector<std::string> generate_colors(unsigned n)
    {
        constexpr double golden_ration_conjugate = 0.618033988749894848204586834;

        std::vector<std::string> values;

        std::random_device re;
        std::uniform_real_distribution<> dist(0.0, 1.0);

        double h = dist(re);

        for (unsigned i = 0; i < n; i++) {
            h = CLAMP(h, 0.0, 1.0);
            auto rgb = hsv_to_rgb(h, 0.5, 0.95);
            values.push_back(rgb_to_color_string(rgb));
            h = std::fmod(h + golden_ration_conjugate, 1.0);
        }

        return values;
    }

    void size_cell_data_func(GtkTreeViewColumn *, GtkCellRenderer *renderer,
                             GtkTreeModel *model, GtkTreeIter *iter,
                             gpointer user_data)
    {
        const guint index = GPOINTER_TO_UINT(user_data);

        guint64 size;
        GValue value = { 0 };

        gtk_tree_model_get_value(model, iter, index, &value);

        switch (G_VALUE_TYPE(&value)) {
            case G_TYPE_ULONG:
                size = g_value_get_ulong(&value);
                break;

            case G_TYPE_UINT64:
                size = g_value_get_uint64(&value);
                break;

            default:
                g_assert_not_reached();
        }

        g_value_unset(&value);

        char *str = g_format_size_full(size, G_FORMAT_SIZE_IEC_UNITS);
        g_object_set(renderer, "text", str, NULL);
        g_free(str);
    }

    void percentage_cell_data_func(GtkTreeViewColumn *, GtkCellRenderer *renderer,
                             GtkTreeModel *model, GtkTreeIter *iter,
                             gpointer user_data)
    {
        const guint index = GPOINTER_TO_UINT(user_data);

        gdouble size;
        GValue value = { 0 };

        gtk_tree_model_get_value(model, iter, index, &value);
        size = g_value_get_double(&value);
        g_value_unset(&value);

        char *str = g_strdup_printf("%.2f", size);
        g_object_set(renderer, "text", str, NULL);
        g_free(str);
    }


    /*
      Same as above but handles size == 0 as not available
    */
    void size_na_cell_data_func(GtkTreeViewColumn *, GtkCellRenderer *renderer,
                                GtkTreeModel *model, GtkTreeIter *iter,
                                gpointer user_data)
    {
        const guint index = GPOINTER_TO_UINT(user_data);

        guint64 size;
        GValue value = { 0 };

        gtk_tree_model_get_value(model, iter, index, &value);

        switch (G_VALUE_TYPE(&value)) {
            case G_TYPE_ULONG:
                size = g_value_get_ulong(&value);
                break;

            case G_TYPE_UINT64:
                size = g_value_get_uint64(&value);
                break;

            default:
                g_assert_not_reached();
        }

        g_value_unset(&value);

        if (size == 0) {
            g_object_set (renderer,
                          "text", _("N/A"),
                          "style", PANGO_STYLE_ITALIC,
                          NULL);
        }
        else {
            char *str = format_byte_size(size, GsmApplication::get()->config.process_memory_in_iec);
            g_object_set (renderer,
                          "text", str,
                          "style", PANGO_STYLE_NORMAL,
                          NULL);
            g_free(str);
        }

    }

    void io_rate_cell_data_func(GtkTreeViewColumn *, GtkCellRenderer *renderer,
                                GtkTreeModel *model, GtkTreeIter *iter,
                                gpointer user_data)
    {
        const guint index = GPOINTER_TO_UINT(user_data);

        guint64 size;
        GValue value = { 0 };

        gtk_tree_model_get_value(model, iter, index, &value);

        switch (G_VALUE_TYPE(&value)) {
            case G_TYPE_ULONG:
                size = g_value_get_ulong(&value);
                break;

            case G_TYPE_UINT64:
                size = g_value_get_uint64(&value);
                break;

            default:
                g_assert_not_reached();
        }

        g_value_unset(&value);

        if (size == 0) {
            g_object_set (renderer,
                          "text", _("N/A"),
                          "style", PANGO_STYLE_ITALIC,
                          NULL);
        }
        else {
            g_object_set (renderer,
                          "text", procman::format_rate(size, FALSE).c_str(),
                          "style", PANGO_STYLE_NORMAL,
                          NULL);
        }

    }
    
    /*
        Cell data function to format a size value with SI units (to be used only for disk size, see bugzilla 693630)
    */
    void size_si_cell_data_func(GtkTreeViewColumn *, GtkCellRenderer *renderer,
                             GtkTreeModel *model, GtkTreeIter *iter,
                             gpointer user_data)
    {
        const guint index = GPOINTER_TO_UINT(user_data);

        guint64 size;
        GValue value = { 0 };

        gtk_tree_model_get_value(model, iter, index, &value);

        switch (G_VALUE_TYPE(&value)) {
            case G_TYPE_ULONG:
                size = g_value_get_ulong(&value);
                break;

            case G_TYPE_UINT64:
                size = g_value_get_uint64(&value);
                break;

            default:
                g_assert_not_reached();
        }

        g_value_unset(&value);

        char *str = g_format_size(size);
        g_object_set(renderer, "text", str, NULL);
        g_free(str);
    }

    void duration_cell_data_func(GtkTreeViewColumn *, GtkCellRenderer *renderer,
                                 GtkTreeModel *model, GtkTreeIter *iter,
                                 gpointer user_data)
    {
        const guint index = GPOINTER_TO_UINT(user_data);

        unsigned time;
        GValue value = { 0 };

        gtk_tree_model_get_value(model, iter, index, &value);

        switch (G_VALUE_TYPE(&value)) {
            case G_TYPE_ULONG:
                time = g_value_get_ulong(&value);
                break;

            case G_TYPE_UINT64:
                time = g_value_get_uint64(&value);
                break;

            default:
                g_assert_not_reached();
        }

        g_value_unset(&value);

        time = 100 * time / GsmApplication::get()->frequency;
        char *str = format_duration_for_display(time);
        g_object_set(renderer, "text", str, NULL);
        g_free(str);
    }


    void time_cell_data_func(GtkTreeViewColumn *, GtkCellRenderer *renderer,
                             GtkTreeModel *model, GtkTreeIter *iter,
                             gpointer user_data)
    {
        const guint index = GPOINTER_TO_UINT(user_data);

        time_t time;
        GValue value = { 0 };

        gtk_tree_model_get_value(model, iter, index, &value);

        switch (G_VALUE_TYPE(&value)) {
            case G_TYPE_ULONG:
                time = g_value_get_ulong(&value);
                break;

            default:
                g_assert_not_reached();
        }

        g_value_unset(&value);

        char *str = procman_format_date_for_display(time);
        g_object_set(renderer, "text", str, NULL);
        g_free(str);
    }

    void status_cell_data_func(GtkTreeViewColumn *, GtkCellRenderer *renderer,
                               GtkTreeModel *model, GtkTreeIter *iter,
                               gpointer user_data)
    {
        const guint index = GPOINTER_TO_UINT(user_data);

        guint state;
        GValue value = { 0 };

        gtk_tree_model_get_value(model, iter, index, &value);

        switch (G_VALUE_TYPE(&value)) {
            case G_TYPE_UINT:
                state = g_value_get_uint(&value);
                break;

            default:
                g_assert_not_reached();
        }

        g_value_unset(&value);

        const char *str = format_process_state(state);
        g_object_set(renderer, "text", str, NULL);
    }

    void priority_cell_data_func(GtkTreeViewColumn *, GtkCellRenderer *renderer,
                             GtkTreeModel *model, GtkTreeIter *iter,
                             gpointer user_data)
    {
        const guint index = GPOINTER_TO_UINT(user_data);

        GValue value = { 0 };

        gtk_tree_model_get_value(model, iter, index, &value);

        gint priority = g_value_get_int(&value);

        g_value_unset(&value);

        g_object_set(renderer, "text", procman::get_nice_level(priority), NULL);

    }

    gint priority_compare_func(GtkTreeModel* model, GtkTreeIter* first,
                            GtkTreeIter* second, gpointer user_data)
    {
        const guint index = GPOINTER_TO_UINT(user_data);
        GValue value1 = { 0 };
        GValue value2 = { 0 };
        gtk_tree_model_get_value(model, first, index, &value1);
        gtk_tree_model_get_value(model, second, index, &value2);
        gint result = g_value_get_int(&value1) - g_value_get_int(&value2);
        g_value_unset(&value1);
        g_value_unset(&value2);
        return result;
    }

    gint number_compare_func(GtkTreeModel* model, GtkTreeIter* first,
                            GtkTreeIter* second, gpointer user_data)
    {
        const guint index = GPOINTER_TO_UINT(user_data);

        gdouble size1, size2;
        size1 = get_size_from_column(model, first, index);
        size2 = get_size_from_column(model, second, index);

        if ( size2 > size1 )
            return 1;
        else if ( size2 < size1 )
            return -1;
        return 0;
    }

    template<>
    void tree_store_update<const char>(GtkTreeModel* model, GtkTreeIter* iter, int column, const char* new_value)
    {
        char* current_value;

        gtk_tree_model_get(model, iter, column, &current_value, -1);

        if (g_strcmp0(current_value, new_value) != 0)
            gtk_tree_store_set(GTK_TREE_STORE(model), iter, column, new_value, -1);

        g_free(current_value);
    }


    std::string format_rate(guint64 rate, bool want_bits)
    {
        char* bytes = procman::format_size(rate, want_bits);
        // xgettext: rate, 10MiB/s or 10Mbit/s
        std::string formatted_rate(make_string(g_strdup_printf(_("%s/s"), bytes)));
        g_free(bytes);
        return formatted_rate;
    }


    std::string format_network(guint64 rate)
    {
        char* bytes = procman::format_size(rate, GsmApplication::get()->config.network_total_in_bits);
        std::string formatted(bytes);
        g_free(bytes);
        return formatted;
    }


    std::string format_network_rate(guint64 rate)
    {
        return procman::format_rate(rate, GsmApplication::get()->config.network_in_bits);
    }

}

Glib::ustring
get_monospace_system_font_name ()
{
    return Gio::Settings::create ("org.gnome.desktop.interface")->get_string ("monospace-font-name");
}


GtkLabel *
make_tnum_label (void)
{
    PangoAttrList *attrs = make_tnum_attr_list ();
    GtkWidget *label = gtk_label_new (NULL);

    gtk_label_set_attributes (GTK_LABEL (label), attrs);

    g_clear_pointer (&attrs, pango_attr_list_unref);

    return GTK_LABEL (label);
}


PangoAttrList *
make_tnum_attr_list (void)
{
    PangoAttrList *attrs = NULL;

    attrs = pango_attr_list_new ();
    pango_attr_list_insert (attrs, pango_attr_font_features_new ("tnum=1"));

    return attrs;
}

