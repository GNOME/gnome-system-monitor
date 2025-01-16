subdir('legacy')

system_monitor_sources = [
  'application.cpp',
  'argv.cpp',
  'cgroups.cpp',
  'disks.cpp',
  'disks-data.c',
  'gsm_gksu.cpp',
  'gsm_gnomesu.cpp',
  'gsm_pkexec.cpp',
  'gsm-graph.c',
  'interface.cpp',
  'load-graph.cpp',
  'lsof.cpp',
  'lsof-data.c',
  'memmaps.cpp',
  'memmaps-data.c',
  'openfiles.cpp',
  'openfiles-data.c',
  'prefsdialog.cpp',
  'prettytable.cpp',
  'procactions.cpp',
  'procdialogs.cpp',
  'procinfo.cpp',
  'proclist.cpp',
  'procproperties.cpp',
  'proctable.cpp',
  'selinux.cpp',
  'setaffinity.cpp',
  'smooth_refresh.cpp',
  'systemd.cpp',
  'update_interval.cpp',
  'util.cpp',
]

system_monitor_headers = [
  'legacy/e_date.h',
  'legacy/gsm_color_button.h',
  'legacy/treeview.h',
  'application.h',
  'argv.h',
  'cgroups.h',
  'defaulttable.h',
  'disks.h',
  'disks-data.h',
  'gsm_gksu.h',
  'gsm_gnomesu.h',
  'gsm_pkexec.h',
  'gsm-graph.h',
  'interface.h',
  'join.h',
  'load-graph.h',
  'lsof.h',
  'lsof-data.h',
  'memmaps.h',
  'memmaps-data.h',
  'openfiles.h',
  'openfiles-data.h',
  'prefsdialog.h',
  'prettytable.h',
  'procactions.h',
  'procdialogs.h',
  'procinfo.h',
  'proclist.h',
  'procproperties.h',
  'proctable.h',
  'selinux.h',
  'setaffinity.h',
  'settings-keys.h',
  'smooth_refresh.h',
  'systemd.h',
  'update_interval.h',
  'util.h',
]

gresourceconf = configuration_data()
gresourceconf.set('metainfo_file', '@0@.metainfo.xml'.format(app_id))

gresource_file = configure_file(
  input: 'gsm.gresource.xml.in',
  output: 'gsm.gresource.xml',
  configuration: gresourceconf,
)

gsm_resource_dir = join_paths(get_option('datadir'), meson.project_name())
gsm_resource = gnome.compile_resources(
  'gsm',
  gresource_file,
  dependencies: metainfo_file,
  gresource_bundle: true,
  source_dir: meson.project_source_root(),
  install: true,
  install_dir: gsm_resource_dir,
)

gsm_schemas = configure_file(
  input : 'org.gnome.gnome-system-monitor.gschema.xml.in',
  output: 'org.gnome.gnome-system-monitor.gschema.xml',
  configuration: dataconf,
  install: true,
  install_dir: get_option('datadir') / 'glib-2.0' / 'schemas',
)

gnome.compile_schemas(depend_files: gsm_schemas)

gsm_gsettings = gnome.mkenums('org.gnome.gnome-system-monitor.enums.xml',
  sources: system_monitor_headers,
  comments: '<!-- @comment@ -->',
  fhead:    '<schemalist>',
  vhead:    '  <@type@ id=\'org.gnome.gnome-system-monitor.@EnumName@\'>',
  vprod:    '    <value nick=\'@valuenick@\' value=\'@valuenum@\'/>',
  vtail:    '  </@type@>',
  ftail:    '</schemalist>',
  install_header: true,
  install_dir: join_paths(get_option('datadir'), 'glib-2.0', 'schemas'),
)

system_monitor_dependencies = [
  glibmm,
  giomm,
  gmodule,
  gtk,
  gtkmm,
  libgtop,
  libadwaita,
  libsystemd,
]
libgsm = static_library(
  'libgsm',
  system_monitor_sources,
  include_directories: rootInclude,
  dependencies: system_monitor_dependencies,
  link_with: libgsm_legacy,
  cpp_args: [
    '-DGSM_RESOURCE_FILE="@0@"'.format(join_paths(
      get_option('prefix'), gsm_resource_dir, 'gsm.gresource')),
  ],
)
libgsm_dep = declare_dependency(
  dependencies: system_monitor_dependencies,
  include_directories: rootInclude,
  link_with: libgsm,
)

executable(meson.project_name(),
  ['main.cpp'],
  dependencies: libgsm_dep,
  install: true,
)

unit_tests = executable('unit_tests',
  [
    'test/cgroups.cpp',
    'test/join.cpp'
  ],
  dependencies: [
    catch2,
    libgsm_dep,
  ],
)
test('unit_tests', unit_tests)

gnome.post_install(
  glib_compile_schemas: true,
)
