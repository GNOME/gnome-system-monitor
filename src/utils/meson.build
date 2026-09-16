libgsm_utils_sources = [
  config_h,
  'gsm-module-loader.h',
]

libgsm_utils_dependencies = [glib, gmodule]

libgsm_utils = static_library(
  'gsm-utils',
  libgsm_utils_sources,
  include_directories: rootInclude,
  dependencies: libgsm_utils_dependencies,
  gnu_symbol_visibility: 'hidden',
)

libgsm_utils_dep = declare_dependency(
  include_directories: [rootInclude, '.'],
  dependencies: libgsm_utils_dependencies,
  link_with: libgsm_utils,
)
