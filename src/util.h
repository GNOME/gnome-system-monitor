#ifndef _GSM_UTIL_H_
#define _GSM_UTIL_H_

#include <gtkmm.h>
#include <string>
#include <vector>

using std::string;

GtkLabel*
procman_make_label_for_mmaps_or_ofiles (const char *format,
                                        const char *process_name,
                                        unsigned    pid);

gboolean
load_symbols (const char *module,
              ...) G_GNUC_NULL_TERMINATED;

const char*
format_process_state (guint state);

gchar*
format_byte_size (guint64 size,
                  bool    want_iec_format);

void
procman_debug_real (const char *file,
                    int         line,
                    const char *func,
                    const char *format,
                    ...) G_GNUC_PRINTF (4, 5);

#define procman_debug(FMT, ...) procman_debug_real (__FILE__, __LINE__, __func__, FMT, ## __VA_ARGS__)

GtkLabel *                         init_tnum_label (gint     char_width,
                                                    GtkAlign halign);
GtkLabel *                         make_tnum_label (void);
std::tuple<double, double, double> hsv_to_rgb (double h,
                                               double s,
                                               double v);
std::string                        rgb_to_color_string (const std::tuple<double, double, double> &t);

inline string
make_string (char *c_str)
{
  if (!c_str)
    {
      procman_debug ("NULL string");
      return string ();
    }

  string s (c_str);

  g_free (c_str);
  return s;
}


namespace procman
{
// create a list of n color strings
std::vector<std::string> generate_colors (unsigned n);

char *                   format_duration_for_display (unsigned centiseconds);

char * io_rate_cell_data_func (guint64 size);

char * size_na_cell_data_func (guint64 size);

char * duration_cell_data_func (guint64 duration);

void priority_cell_data_func (GtkTreeViewColumn *col,
                              GtkCellRenderer   *renderer,
                              GtkTreeModel      *model,
                              GtkTreeIter       *iter,
                              gpointer           user_data);

gint priority_compare_func (GtkTreeModel*model,
                            GtkTreeIter *first,
                            GtkTreeIter *second,
                            gpointer     user_data);

gint number_compare_func (GtkTreeModel*model,
                          GtkTreeIter *first,
                          GtkTreeIter *second,
                          gpointer     user_data);


template<typename T>
void
poison (T &  t,
        char c)
{
  memset (&t, c, sizeof t);
}



//
// Stuff to update a tree_store in a smart way
//

template<typename T>
void
list_store_update (GListModel  *model,
                   guint        position,
                   const char  *column,
                   const T&     new_value)
{
  T current_value;
  GObject *object;

  object = g_list_model_get_object (model, position);
  g_object_get (object, column, &current_value, NULL);

  if (current_value != new_value)
    g_object_set (object, column, new_value, NULL);
}

// undefined
// catch every thing about pointers
// just to make sure i'm not doing anything wrong
template<typename T>
void list_store_update (GListModel *model,
                        guint       position,
                        const char *column,
                        T          *new_value);

// specialized versions for strings
template<>
void list_store_update<const char>(GListModel *model,
                                   guint       position,
                                   const char *column,
                                   const char *new_value);

template<>
inline void
list_store_update<char>(GListModel *model,
                        guint       position,
                        const char *column,
                        char       *new_value)
{
  list_store_update<const char>(model, position, column, new_value);
}

gchar *     format_size (guint64 size,
                         bool    want_bits = false);

gchar *     get_nice_level (gint nice);

gchar *     get_nice_level_with_priority (gint nice);

std::string format_volume (guint64 size,
                           bool    want_bits = false);
std::string format_rate (guint64 rate,
                         bool    want_bits = false);

std::string format_network (guint64 size);
std::string format_network_rate (guint64 rate);

struct NonCopyable
{
  NonCopyable() = default;
  NonCopyable(const NonCopyable&) = delete;
  NonCopyable& operator=(const NonCopyable&) = delete;
};


// join the elements of c with sep
template<typename C, typename S>
auto
join (const C&c,
      const S&sep) -> decltype(c[0] + sep)
{
  decltype(c[0] + sep) r;
  bool first = true;

  for (const auto&e : c)
    {
      if (!first)
        r += sep;
      first = false;
      r += e;
    }

  return r;
}
}

#endif /* _GSM_UTIL_H_ */
