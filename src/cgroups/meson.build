libgsm_cgroups_sources = [
  config_h,
  'cgroups.c',
  'cgroups.h',
]

libgsm_cgroups_dependencies = [glib, gmodule]

libgsm_cgroups = static_library(
  'gsm-cgroups',
  libgsm_cgroups_sources,
  include_directories: rootInclude,
  dependencies: libgsm_cgroups_dependencies,
  gnu_symbol_visibility: 'hidden',
)

libgsm_cgroups_dep = declare_dependency(
  include_directories: [rootInclude, '.'],
  dependencies: libgsm_cgroups_dependencies,
  link_with: libgsm_cgroups,
)

test(
  'cgroups',
  executable(
    'test-cgroups',
    [config_h, 'test.c'],
    include_directories: [rootInclude, '.'],
    dependencies: libgsm_cgroups_dependencies,
  ),
  protocol: 'tap',
)
