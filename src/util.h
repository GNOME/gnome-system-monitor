#ifndef H_GNOME_SYSTEM_MONITOR_UTIL_1123178725
#define H_GNOME_SYSTEM_MONITOR_UTIL_1123178725

#include <glib.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <stddef.h>
#include <string>
#include <functional>
#include <algorithm>

using std::string;

template<typename T>
inline int procman_cmp(T x, T y)
{
	if (x == y)
		return 0;

	if (x < y)
		return -1;

	return 1;
}

#define PROCMAN_CMP(X, Y) procman_cmp((X), (Y))
#define PROCMAN_RCMP(X, Y) procman_cmp((Y), (X));

GtkWidget*
procman_make_label_for_mmaps_or_ofiles(const char *format,
					     const char *process_name,
					     unsigned pid) G_GNUC_INTERNAL;


gchar*
SI_gnome_vfs_format_file_size_for_display (GnomeVFSFileSize size) G_GNUC_INTERNAL;


gboolean
load_symbols(const char *module, ...) G_GNUC_INTERNAL G_GNUC_NULL_TERMINATED;


void
procman_debug(const char *format, ...) G_GNUC_INTERNAL G_GNUC_PRINTF(1, 2);


inline string make_string(char *c_str)
{
	string s(c_str);
	g_free(c_str);
	return s;
}




template<typename Map>
class UnrefMapValues
  : public std::unary_function<void, Map>
{
public:
  void operator()(const typename Map::value_type &it) const
  {
    g_object_unref(it.second);
  }
};


template<typename Map>
inline void unref_map_values(Map &map)
{
  std::for_each(map.begin(), map.end(), UnrefMapValues<Map>());
}

#endif /* H_GNOME_SYSTEM_MONITOR_UTIL_1123178725 */
