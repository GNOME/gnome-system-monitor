libgsm_actions_impl_sources = [
  config_h,
  'gsm-kill.c',
  'gsm-kill.h',
  'gsm-setaffinity.c',
  'gsm-setaffinity.h',
  'gsm-setpriority.c',
  'gsm-setpriority.h',
]

libgsm_actions_impl_dependencies = [glib, gmodule]

libgsm_actions_impl = static_library(
  'gsm-actions-impl',
  libgsm_actions_impl_sources,
  include_directories: rootInclude,
  dependencies: libgsm_actions_impl_dependencies,
  gnu_symbol_visibility: 'hidden',
)

libgsm_actions_impl_dep = declare_dependency(
  include_directories: [rootInclude, '.'],
  dependencies: libgsm_actions_impl_dependencies,
  link_with: libgsm_actions_impl,
)
