libgsm_actions_sources = [
  config_h,
  'gsm_gksu.cpp',
  'gsm_gksu.h',
  'gsm_gnomesu.cpp',
  'gsm_gnomesu.h',
  'gsm_pkexec.cpp',
  'gsm_pkexec.h',
  'gsm-actions.c',
  'gsm-actions.h',
]

libgsm_actions_dependencies = [glib, gmodule, libgsm_actions_impl_dep]

libgsm_actions = static_library(
  'gsm-actions',
  libgsm_actions_sources,
  include_directories: rootInclude,
  dependencies: libgsm_actions_dependencies,
  gnu_symbol_visibility: 'hidden',
)

libgsm_actions_dep = declare_dependency(
  include_directories: [rootInclude, '.'],
  dependencies: libgsm_actions_dependencies,
  link_with: libgsm_actions,
)
