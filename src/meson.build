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
  'main.cpp',
  'setaffinity.cpp',
  'memmaps.cpp',
  'memmaps-info.c',
  'openfiles.cpp',
  'openfiles-data.c',
  'prefsdialog.cpp',
  'prettytable.cpp',
  'procactions.cpp',
  'procdialogs.cpp',
  'procproperties.cpp',
  'proctable.cpp',
  'selinux.cpp',
  'smooth_refresh.cpp',
  'systemd.cpp',
  'util.cpp',
]

system_monitor_headers = [
  'prettytable.h',
  'procdialogs.h',
  'cgroups.h',
  'application.h',
  'smooth_refresh.h',
  'gsm_gnomesu.h',
  'openfiles.h',
  'openfiles-data.h',
  'procproperties.h',
  'lsof.h',
  'lsof-data.h',
  'proctable.h',
  'settings-keys.h',
  'setaffinity.h',
  'memmaps.h',
  'memmaps-info.h',
  'procactions.h',
  'systemd.h',
  'argv.h',
  'prefsdialog.h',
  'selinux.h',
  'util.h',
  'gsm_gksu.h',
  'interface.h',
  'load-graph.h',
  'disks.h',
  'disks-data.h',
  'gsm_pkexec.h',
  'defaulttable.h',
  'gsm-graph.h',
  'legacy/treeview.h',
  'legacy/e_date.h',
  'legacy/gsm_color_button.h',
]

gsm_resource_dir = join_paths(get_option('datadir'), meson.project_name())
gsm_resource = gnome.compile_resources(
  'gsm',
  'gsm.gresource.xml',
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


executable(meson.project_name(),
  system_monitor_sources,
  include_directories: rootInclude,
  dependencies: [
    glibmm,
    giomm,
    gmodule,
    gtk,
    gtkmm,
    libgtop,
    libadwaita,
    libsystemd,
  ],
  link_with: libgsm_legacy,
  cpp_args: [
    '-DGSM_RESOURCE_FILE="@0@"'.format(join_paths(
      get_option('prefix'), gsm_resource_dir, 'gsm.gresource')),
  ],
  install: true,
)

gnome.post_install(
  glib_compile_schemas: true,
)