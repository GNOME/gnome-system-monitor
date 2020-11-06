commands = [
  'renice',
  'kill',
  'taskset',
]

foreach command : commands
  executable('gsm-' + command,
    'gsm_execute_helper.c',
    dependencies: [
      glib,
    ],
    c_args: '-DCOMMAND="@0@"'.format(command),
    install: true,
    install_dir: join_paths(get_option('libexecdir'), meson.project_name())
  )
endforeach
