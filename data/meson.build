################################################################################
# Generate files

desktop_files = []

desktop_files += i18n.merge_file(
        input: configure_file(
                  input: 'org.gnome.SystemMonitor.desktop.in.in',
                 output: '@0@.desktop.in'.format(app_id),
          configuration: dataconf,
        ),
       output: '@0@.desktop'.format(app_id),
         type: 'desktop',
       po_dir: '../po',
      install: true,
  install_dir: join_paths(get_option('datadir'), 'applications')
)

desktop_files += i18n.merge_file(
        input: configure_file(
                  input: 'gnome-system-monitor-kde.desktop.in.in',
                 output: 'gnome-system-monitor-kde.desktop.in',
          configuration: dataconf,
        ),
       output: 'gnome-system-monitor-kde.desktop',
         type: 'desktop',
       po_dir: '../po',
      install: true,
  install_dir: join_paths(get_option('datadir'), 'applications')
)

metainfo_file = i18n.merge_file(
  input : configure_file(
  configuration: dataconf,
  input : 'org.gnome.SystemMonitor.metainfo.xml.in.in',
  output: '@0@.metainfo.xml.in'.format(app_id),
  ),
  output: '@0@.metainfo.xml'.format(app_id),
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

configure_file(
  input: 'org.gnome.SystemMonitor.service.in',
  output: '@0@.service'.format(app_id),
  configuration: dataconf,
  install: true,
  install_dir: join_paths(get_option('datadir'), 'dbus-1', 'services'),
)

################################################################################
# Tests

appstreamcli = find_program('appstreamcli', required: false)
if appstreamcli.found()
  test('validate_metainfo',
    appstreamcli,
    args: [ 'validate', '--no-net', '--explain', metainfo_file ],
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
