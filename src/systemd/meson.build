if not libsystemd.found()
  libgsm_systemd_dep = declare_dependency()
  subdir_done()
endif

libgsm_systemd_sources = [
  config_h,
  'systemd.c',
  'systemd.h',
]

libgsm_systemd_dependencies = [glib, gmodule, libsystemd]

libgsm_systemd = static_library(
  'gsm-systemd',
  libgsm_systemd_sources,
  include_directories: rootInclude,
  dependencies: libgsm_systemd_dependencies,
  gnu_symbol_visibility: 'hidden',
)

libgsm_systemd_dep = declare_dependency(
  include_directories: [rootInclude, '.'],
  dependencies: libgsm_systemd_dependencies,
  link_with: libgsm_systemd,
)
