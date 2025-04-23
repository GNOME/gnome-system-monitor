# https://gitlab.gnome.org/GNOME/gnome-system-monitor/issues
project('gnome-system-monitor',
  'c', 'cpp',
  default_options : [
    'c_std=gnu2x',
    'cpp_std=gnu++20',
    'warning_level=3',
    'b_lto=true',
  ],
  version: '49.1',
  meson_version: '>=0.57.0',
)


gnome = import('gnome')
i18n  = import('i18n')

cc = meson.get_compiler('c')
cx = meson.get_compiler('cpp')

if get_option('development')
  app_id = 'org.gnome.SystemMonitor.Devel'
else
  app_id = 'org.gnome.SystemMonitor'
endif

gettext_package = meson.project_name()

conf = configuration_data()
conf.set_quoted('APP_ID', app_id)
conf.set_quoted('VERSION', meson.project_version())
conf.set_quoted('GETTEXT_PACKAGE', gettext_package)
conf.set_quoted('GNOMELOCALEDIR',
  join_paths(get_option('prefix'), get_option('localedir'))
)
conf.set_quoted('GSM_LIBEXEC_DIR',
  join_paths(get_option('prefix'), get_option('libexecdir'), meson.project_name())
)

dataconf = configuration_data()
dataconf.set('APP_ID', app_id)
dataconf.set('VERSION', meson.project_version())
dataconf.set('GETTEXT_PACKAGE', gettext_package)
dataconf.set('bindir', join_paths(get_option('prefix'),get_option('bindir')))
dataconf.set('pkglibexecdir',
  join_paths(get_option('prefix'),get_option('libexecdir'),meson.project_name())
)

prefix = get_option('prefix')
datadir = join_paths(prefix, get_option('datadir'))


################################################################################
# Dependencies

catch2     = dependency('catch2-with-main')
giomm      = dependency('giomm-2.68',  version: '>=2.68')
glib       = dependency('glib-2.0',    version: '>=2.56.0')
glibmm     = dependency('glibmm-2.68', version: '>=2.68')
gmodule    = dependency('gmodule-2.0')
gtk        = dependency('gtk4',        version: '>=4.12.0')
gtkmm      = dependency('gtkmm-4.0',   version: '>=4.0.0')
libgtop    = dependency('libgtop-2.0', version: '>=2.41.2')
libadwaita = dependency('libadwaita-1',version: '>=1.6.alpha')
librsvg    = dependency('librsvg-2.0', version: '>=2.46')

if get_option('systemd')
  libsystemd = dependency('libsystemd', version: '>=44')
else
  libsystemd = dependency('', required: false)
endif
conf.set('HAVE_SYSTEMD', libsystemd.found())


################################################################################
# Compiler flags

extra_flags = [
  '-Wcast-align',
  '-Wchar-subscripts',
  '-Winline',
  '-Wmissing-declarations',
  '-Wpointer-arith',
  '-Wsign-compare',
  '-Werror=odr',
  '-Werror=lto-type-mismatch',
  '-Werror=strict-aliasing',
]
extra_cflags = [
  '-Wmissing-prototypes',
  '-Wnested-externs',
]
extra_cxxflags = [
  # '-fvisibility=hidden',
  # '-fvisibility-inlines-hidden',
]


cflags   = extra_flags + extra_cflags
cxxflags = extra_flags + extra_cxxflags

add_project_arguments(cc.get_supported_arguments(cflags),
  language: 'c'
)
add_project_arguments(cx.get_supported_arguments(cxxflags),
  language: 'cpp'
)

conf.set('HAVE_LKSTRFTIME', cc.has_function('strftime'))
has_sysmacros = cc.has_header_symbol('sys/sysmacros.h', 'major')
conf.set('HAVE_DEV_IN_SYSMACROS', has_sysmacros)
conf.set('HAVE_DEV_IN_TYPES', cc.has_header_symbol('sys/types.h', 'major', required: not has_sysmacros))

configure_file(
  output: 'config.h',
  configuration: conf,
)


################################################################################
# Tests

python3 = find_program('python3', required: false)
git = find_program('git', required: false)
patch = find_program('patch', required: false)
uncrustify = find_program('uncrustify', required: false)

check_style_file = files('check-style.py')
check_style_prog = find_program(check_style_file)

if python3.found() and git.found() and patch.found() and uncrustify.found()
  test('code formatting',
    check_style_prog,
    args: ['--all', '--dry-run'],
    suite: 'formatting',
  )
else
  message('Code formatting test requires: python3, git, patch, uncrustify')
endif


################################################################################
# Subdirectories

rootInclude = include_directories('.')

subdir('data')
subdir('po')
subdir('scripts')
subdir('src')
subdir('help')


message('\n'.join(['',
'Configuration:',
'',
'        Source code location:   @0@'.format(meson.project_source_root()),
'        C Compiler:             @0@ @1@'.format(cc.get_id(), cc.version()),
'        C++ Compiler:           @0@ @1@'.format(cx.get_id(), cx.version()),
'        CFLAGS:                 @0@'.format(cflags),
'        CXXFLAGS:               @0@'.format(cxxflags),
'        systemd support:        @0@'.format(libsystemd.found()),
]))


# Extra scripts
gnome.post_install(
  glib_compile_schemas: true,
  gtk_update_icon_cache: true,
)
