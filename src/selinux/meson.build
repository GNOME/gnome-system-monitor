libgsm_selinux_sources = [
  config_h,
  'selinux.c',
  'selinux.h',
]

libgsm_selinux_dependencies = [glib, gmodule]

libgsm_selinux = static_library(
  'gsm-selinux',
  libgsm_selinux_sources,
  include_directories: rootInclude,
  dependencies: libgsm_selinux_dependencies,
  gnu_symbol_visibility: 'hidden',
)

libgsm_selinux_dep = declare_dependency(
  include_directories: [rootInclude, '.'],
  dependencies: libgsm_selinux_dependencies,
  link_with: libgsm_selinux,
)
