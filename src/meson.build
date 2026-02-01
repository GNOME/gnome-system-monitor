subdir('action-impls')
subdir('actions')
subdir('legacy')
subdir('systemd')
subdir('selinux')

system_monitor_sources = [
  'application.cpp',
  'cgroups.cpp',
  'column-view-persister.c',
  'disk.c',
  'disks.c',
  'gsm-graph.c',
  'interface.cpp',
  'load-graph.cpp',
  'lsof.cpp',
  'mem-map.c',
  'memmaps.cpp',
  'open-file.cpp',
  'openfiles.cpp',
  'prefsdialog.cpp',
  'prettytable.cpp',
  'procactions.cpp',
  'procdialogs.cpp',
  'procinfo.cpp',
  'proclist.cpp',
  'procproperties.cpp',
  'proctable.cpp',
  'setaffinity.cpp',
  'smooth_refresh.cpp',
  'update_interval.cpp',
  'util.cpp',
]

system_monitor_headers = [
  'legacy/e_date.h',
  'legacy/gsm_color_button.h',
  'legacy/treeview.h',
  'application.h',
  'cgroups.h',
  'column-view-persister.h',
  'defaulttable.h',
  'disk.h',
  'disks.h',
  'gsm-graph.h',
  'interface.h',
  'join.h',
  'load-graph.h',
  'lsof.h',
  'mem-map.h',
  'memmaps.h',
  'open-file.h',
  'openfiles.h',
  'prefsdialog.h',
  'prettytable.h',
  'procactions.h',
  'procdialogs.h',
  'procinfo.h',
  'proclist.h',
  'procproperties.h',
  'proctable.h',
  'setaffinity.h',
  'settings-keys.h',
  'smooth_refresh.h',
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

gsm_resource = gnome.compile_resources(
  'gsm-resources',
  gresource_file,
  c_name: 'gsm',
  dependencies: metainfo_file,
  source_dir: meson.project_source_root(),
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
  gio_unix,
  gmodule,
  gtk,
  gtkmm,
  libgtop,
  libadwaita,
  libgsm_actions_dep,
  libgsm_systemd_dep,
  libgsm_selinux_dep,
]
libgsm = static_library(
  'libgsm',
  system_monitor_sources + [gsm_resource],
  include_directories: rootInclude,
  dependencies: system_monitor_dependencies,
  link_with: libgsm_legacy,
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
