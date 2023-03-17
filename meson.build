# https://gitlab.gnome.org/GNOME/gnome-system-monitor/issues
project('gnome-system-monitor',
  'c', 'cpp',
  default_options : [
    'c_std=c11',
    'cpp_std=c++11',
    'warning_level=3',
    'libhandy:vapi=false',
    'libhandy:introspection=disabled',
    'libhandy:tests=false',
  ],
  version: '44.0',
  meson_version: '>=0.57.0',
)


gnome = import('gnome')
i18n  = import('i18n')

cc = meson.get_compiler('c')
cx = meson.get_compiler('cpp')

gettext_package = meson.project_name()

conf = configuration_data()
conf.set_quoted('VERSION', meson.project_version())
conf.set_quoted('GETTEXT_PACKAGE', gettext_package)
conf.set_quoted('GNOMELOCALEDIR',
  join_paths(get_option('prefix'), get_option('localedir'))
)
conf.set_quoted('GSM_LIBEXEC_DIR',
  join_paths(get_option('prefix'), get_option('libexecdir'), meson.project_name())
)

dataconf = configuration_data()
dataconf.set('VERSION', meson.project_version())
dataconf.set('GETTEXT_PACKAGE', gettext_package)
dataconf.set('pkglibexecdir',
  join_paths(get_option('prefix'),get_option('libexecdir'),meson.project_name())
)

prefix = get_option('prefix')
datadir = join_paths(prefix, get_option('datadir'))

################################################################################
# Dependencies

giomm   = dependency('giomm-2.4',   version: '>=2.46')
glib    = dependency('glib-2.0',    version: '>=2.56.0')
glibmm  = dependency('glibmm-2.4',  version: '>=2.46')
gmodule = dependency('gmodule-2.0')
gtk3    = dependency('gtk+-3.0',    version: '>=3.22.0')
gtkmm   = dependency('gtkmm-3.0',   version: '>=3.3.18')
atkmm   = dependency('atkmm-1.6',   version: '>=2.28')
libgtop = dependency('libgtop-2.0', version: '>=2.37.0')
libhandy = dependency('libhandy-1', version: '>= 1.5.0', fallback: ['libhandy', 'libhandy_dep'])
librsvg = dependency('librsvg-2.0', version: '>=2.35')
libxml  = dependency('libxml-2.0',  version: '>=2.0')

if get_option('wnck')
  libwnck = dependency('libwnck-3.0', version: '>=2.91.0')
else
  libwnck = dependency('', required: false)
endif
conf.set('HAVE_WNCK', libwnck.found())

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

conf.set('CTIME_R_THREE_ARGS', cc.compiles('''
#include <time.h>
int main() {
  char *buf;
  time_t date;
  ctime_r (&date, buf, 100);
}
'''))


configure_file(
  output: 'config.h',
  configuration: conf,
)

################################################################################
# Generate files

desktop_files = []
foreach file : [
    'gnome-system-monitor.desktop',
    'gnome-system-monitor-kde.desktop',
  ]
  desktop_files += i18n.merge_file(
    input : file + '.in',
    output: file,
    po_dir: 'po',
    type: 'desktop',
    install: true,
    install_dir: join_paths(get_option('datadir'), 'applications'),
  )
endforeach

appdata_file = i18n.merge_file(
  input : 'gnome-system-monitor.appdata.xml.in',
  output: 'gnome-system-monitor.appdata.xml',
  po_dir: 'po',
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
  po_dir: 'po',
  data_dirs: 'po',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'polkit-1', 'actions'),
)


appstream_util = find_program('appstream-util', required: false)
if appstream_util.found()
  test('validate_appdata',
    appstream_util,
    args: [ 'validate-relax', '--nonet', appdata_file ],
  )
endif

desktop_validate = find_program('desktop-file-validate', required: false)
if desktop_validate.found()
  test('validate_desktop',
    desktop_validate,
    args: [ desktop_files ],
  )
endif

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
'        wnck support:           @0@ @1@'.format(libwnck.found(),
      libwnck.found() ? '(this will likely make system-monitor segfault)' : ''),
]))

# Extra scripts
gnome.post_install(
  glib_compile_schemas: true,
  gtk_update_icon_cache: true,
)
