#include <config.h>

#include <glib/gi18n.h>
#include <glib/gstring.h>
#include <gtk/gtk.h>

#include <libgnomevfs/gnome-vfs-utils.h>
#include <glibtop/proctime.h>
#include <unistd.h>

#include <stddef.h>

#include "util.h"


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


GtkWidget*
procman_make_label_for_mmaps_or_ofiles(const char *format,
					     const char *process_name,
					     unsigned pid)
{
	GtkWidget *label;
	char *name, *title;

	name = mnemonic_safe_process_name (process_name);
	title = g_strdup_printf(format, name, pid);
	label = gtk_label_new_with_mnemonic (title);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0f, 0.5f);

	g_free (title);
	g_free (name);

	return label;
}



#define KIBIBYTE_FACTOR (1UL << 10)
#define MEBIBYTE_FACTOR (1UL << 20)
#define GIBIBYTE_FACTOR (1UL << 30)


/**
 * SI_gnome_vfs_format_file_size_for_display:
 * @size:
 * 
 * Formats the file size passed in @bytes in a way that is easy for
 * the user to read. Gives the size in bytes, kibibytes, mebibytes or
 * gibibytes, choosing whatever is appropriate.
 * 
 * Returns: a newly allocated string with the size ready to be shown.
 **/

gchar*
SI_gnome_vfs_format_file_size_for_display (GnomeVFSFileSize size)
{
	if (size < (GnomeVFSFileSize) KIBIBYTE_FACTOR) {
		return g_strdup_printf (dngettext(GETTEXT_PACKAGE, "%u byte", "%u bytes",(guint) size), (guint) size);
	} else {
		guint factor;
		const char* format;

		if (size < (GnomeVFSFileSize) MEBIBYTE_FACTOR) {
		  factor = KIBIBYTE_FACTOR;
		  format = N_("%.1f KiB");
		} else if (size < (GnomeVFSFileSize) GIBIBYTE_FACTOR) {
		  factor = MEBIBYTE_FACTOR;
		  format = N_("%.1f MiB");
		} else {
		  factor = GIBIBYTE_FACTOR;
		  format = N_("%.1f GiB");
		}

		return g_strdup_printf(_(format), size / (double)factor);
	}
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


static double get_relative_time(void)
{
	static unsigned long start_time;
	GTimeVal tv;

	if (G_UNLIKELY(!start_time)) {
		glibtop_proc_time buf;
		glibtop_get_proc_time(&buf, getpid());
		start_time = buf.start_time;
	}

	g_get_current_time(&tv);
	return (tv.tv_sec - start_time) + 1e-6 * tv.tv_usec;
}


void
procman_debug(const char *format, ...)
{
	va_list args;
	char *msg;

	if (G_LIKELY(!is_debug_enabled()))
		return;

	va_start(args, format);
	msg = g_strdup_vprintf(format, args);
	va_end(args);

	g_debug("[%0.6f] %s", get_relative_time(), msg);

	g_free(msg);
}

