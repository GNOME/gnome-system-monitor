/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <glib.h>
#include <gmodule.h>

G_BEGIN_DECLS


typedef enum {
  GSM_MODULE_INIT_FAILED = 0,
  GSM_MODULE_INIT_SUCCESS,
} GsmModuleInitResult;


#define GSM_DEFINE_MODULE_LOADER(ns, name, file)                             \
  static inline GsmModuleInitResult                                          \
  ns##_##name##_init_module (GModule *module);                               \
                                                                             \
  static inline GModule *                                                    \
  ns##_##name##_load_module (void)                                           \
  {                                                                          \
    g_autoptr (GError) error = NULL;                                         \
    GModule *module =                                                        \
      g_module_open_full (file,                                              \
                          G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL,          \
                          &error);                                           \
                                                                             \
    if (error) {                                                             \
      g_debug ("Could not load " file ": %s", error->message);               \
      goto fail;                                                             \
    }                                                                        \
                                                                             \
    if (!module) {                                                           \
      g_debug ("Could not load " file);                                      \
      goto fail;                                                             \
    }                                                                        \
                                                                             \
    if (ns##_##name##_init_module (module) != GSM_MODULE_INIT_SUCCESS) {     \
      g_debug ("Failed to load " file);                                      \
      goto fail;                                                             \
    }                                                                        \
                                                                             \
    return g_steal_pointer (&module);                                        \
                                                                             \
  fail:                                                                      \
    g_clear_pointer (&module, g_module_close);                               \
                                                                             \
    return NULL;                                                             \
                                                                             \
  }                                                                          \
                                                                             \
  static gboolean                                                            \
  ns##_##name##_get_or_load_module ()                                        \
  {                                                                          \
    static GModule *module = NULL;                                           \
    enum {                                                                   \
      UNINIT = 0,                                                            \
      AVAILABLE,                                                             \
      NOT_AVAILABLE,                                                         \
    };                                                                       \
    static size_t status = UNINIT;                                           \
                                                                             \
    if (g_once_init_enter (&status)) {                                       \
      module = ns##_##name##_load_module ();                                 \
                                                                             \
      g_once_init_leave (&status, module ? AVAILABLE : NOT_AVAILABLE);       \
    }                                                                        \
                                                                             \
    return status == AVAILABLE;                                              \
  }

#define GSM_MODULE_REQUIRE_SYMBOL(module, file, symbol)                      \
  if (!g_module_symbol (module, #symbol, (gpointer *) &symbol)) {            \
    g_debug ("Could not load " #symbol " from " file);                       \
    return GSM_MODULE_INIT_FAILED;                                           \
  } else {                                                                   \
    g_debug ("Loaded " #symbol " from " file);                               \
  }

G_END_DECLS
