#include "util.h"

#include <config.h>

#include <glib/gi18n.h>
#include <stddef.h>

void _procman_array_gettext_init(const char * strings[], size_t n)
{
	size_t i;

	for(i = 0; i < n; ++i)
	{
		if(strings[i] != NULL)
			strings[i] = _(strings[i]);
	}
}
