#include <config.h>

#include <glib/gi18n.h>
#include <glib/gstring.h>
#include <gtk/gtk.h>

#include <stddef.h>

#include "util.h"


void _procman_array_gettext_init(const char * strings[], size_t n)
{
	size_t i;

	for(i = 0; i < n; ++i)
	{
		if(strings[i] != NULL)
			strings[i] = _(strings[i]);
	}
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
