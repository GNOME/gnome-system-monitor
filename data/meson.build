################################################################################
# Generate files

desktop_files = []
foreach file : [
    'org.gnome.SystemMonitor.desktop',
    'gnome-system-monitor-kde.desktop',
  ]
  desktop_files += i18n.merge_file(
    input : file + '.in',
    output: file,
    po_dir: '../po',
    type: 'desktop',
    install: true,
    install_dir: join_paths(get_option('datadir'), 'applications'),
  )
endforeach

appdata_file = i18n.merge_file(
  input : 'gnome-system-monitor.appdata.xml.in',
  output: 'gnome-system-monitor.appdata.xml',
  po_dir: '../po',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'metainfo'),
)

i18n.merge_file(
  input : configure_file(
  configuration: dataconf,
  input : 'org.gnome.gnome-system-monitor.policy.in.in',
  output: 'org.gnome.gnome-system-monitor.policy.in'
  ),
  output: 'org.gnome.gnome-system-monitor.policy',
  po_dir: '../po',
  data_dirs: '../po',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'polkit-1', 'actions'),
)


################################################################################
# Tests

appstreamcli = find_program('appstreamcli', required: false)
if appstreamcli.found()
  test('validate_appdata',
    appstreamcli,
    args: [ 'validate', '--no-net', '--explain', appdata_file ],
  )
endif

desktop_validate = find_program('desktop-file-validate', required: false)
if desktop_validate.found()
  test('validate_desktop',
    desktop_validate,
    args: [ desktop_files ],
  )
endif


subdir('icons')
