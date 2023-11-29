system_monitor_ng_sources = [
  'application.c',
]

system_monitor_ng_headers = [
  'application.h',
]

gsm_resource_dir = join_paths(get_option('datadir'), meson.project_name())
gsm_resource = gnome.compile_resources(
  'gsm-ng',
  'gsm-ng.gresource.xml',
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
  sources: system_monitor_ng_headers,
  comments: '<!-- @comment@ -->',
  fhead:    '<schemalist>',
  vhead:    '  <@type@ id=\'org.gnome.gnome-system-monitor.@EnumName@\'>',
  vprod:    '    <value nick=\'@valuenick@\' value=\'@valuenum@\'/>',
  vtail:    '  </@type@>',
  ftail:    '</schemalist>',
  install_header: true,
  install_dir: join_paths(get_option('datadir'), 'glib-2.0', 'schemas'),
)


executable('gsm-ng',
  system_monitor_ng_sources,
  include_directories: rootInclude,
  dependencies: [
    gtk,
    libgtop,
    libadwaita,
    libsystemd,
  ],
  cpp_args: [
    '-DGSM_RESOURCE_FILE="@0@"'.format(join_paths(
      get_option('prefix'), gsm_resource_dir, 'gsm-ng.gresource')),
  ],
  install: true,
)

gnome.post_install(
  glib_compile_schemas: true,
)
