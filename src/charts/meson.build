
libgsm_charts_sources = [
  'gsm-cpu-graph.c',
  'gsm-cpu-model.c',
]

libgsm_charts = static_library('gsm_charts',
  libgsm_charts_sources,
  include_directories: rootInclude,
  dependencies: [
    glib,
    gtk3,
    librsvg,
    libdazzle,
  ],
)
