
libgsm_legacy_sources = [
  'e_date.c',
  'gsm_color_button.c',
  'treeview.c',
]

libgsm_legacy = static_library('gsm_legacy',
  libgsm_legacy_sources,
  include_directories: rootInclude,
  dependencies: [
    glib,
    gtk,
    librsvg,
  ],
)
