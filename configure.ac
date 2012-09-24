dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.63])

AC_INIT([gnome-system-monitor],
        [3.6.0],
        [http://bugzilla.gnome.org/enter_bug.cgi?product=system-monitor],
        [gnome-system-monitor])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11.1 no-dist-gzip dist-xz])
AM_SILENT_RULES([yes])

AM_MAINTAINER_MODE([enable])
GNOME_MAINTAINER_MODE_DEFINES

# Check For programs
AC_PROG_CC
AC_PROG_CXX
AC_LANG([C++])
AC_LANG_COMPILER_REQUIRE

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT

# Package dependencies
GLIB_REQUIRED=2.28.0
LIBWNCK_REQUIRED=2.91.0
LIBGTOP_REQUIRED=2.28.2
GTK_REQUIRED=3.0
GNOME_ICON_THEME_REQUIRED=2.31
GTKMM_REQUIRED=2.99
GIOMM_REQUIRED=2.27
GLIBMM_REQUIRED=2.27
LIBXML_REQUIRED=2.0
RSVG_REQUIRED=2.35
SYSTEMD_REQUIRED=38
# systemd v43 used the C++ keyword 'class' as a function parameter. This breaks the build for C++ programs.
SYSTEMD_EXCLUDED=43

PKG_CHECK_MODULES(PROCMAN, glib-2.0 >= $GLIB_REQUIRED libgtop-2.0 >= $LIBGTOP_REQUIRED libwnck-3.0 >= $LIBWNCK_REQUIRED gtk+-3.0 >= $GTK_REQUIRED gnome-icon-theme >= $GNOME_ICON_THEME_REQUIRED gtkmm-3.0 >= $GTKMM_REQUIRED libxml-2.0 >= $LIBXML_REQUIRED librsvg-2.0 >= $RSVG_REQUIRED glibmm-2.4 >= $GLIBMM_REQUIRED giomm-2.4 >= $GIOMM_REQUIRED gmodule-2.0)

have_systemd=no
AC_ARG_ENABLE(systemd, AS_HELP_STRING([--disable-systemd], [disable systemd support]),,enable_systemd=no)
if test "x$enable_systemd" != "xno"; then
        PKG_CHECK_MODULES(SYSTEMD, [ libsystemd-daemon >= $SYSTEMD_REQUIRED libsystemd-login >= $SYSTEMD_REQUIRED ],
                                   [ PKG_CHECK_MODULES(SYSTEMD_EX, [ libsystemd-login = $SYSTEMD_EXCLUDED ],
                                                                   AC_MSG_ERROR([*** systemd v43 found but it has a bug in its header files (uses C++ keyword)]), # Can't use systemd v43 
                                                                   [AC_DEFINE(HAVE_SYSTEMD, 1, [Define if systemd is available]) have_systemd=yes])],
                                   have_systemd=no)
        if test "x$have_systemd" = xno; then
                AC_MSG_ERROR([*** systemd support requested but libraries not found])
        fi
fi
AM_CONDITIONAL(HAVE_SYSTEMD, [test "$have_systemd" = "yes"])

# Compiler warnings
GNOME_COMPILE_WARNINGS([maximum])

AC_ARG_ENABLE(more-warnings,
[AS_HELP_STRING([--enable-more-warnings],[Maximum compiler warnings])],
set_more_warnings="$enableval",[
    set_more_warnings=yes
])
AC_MSG_CHECKING(for more warnings, including -Werror)
if test "$GCC" = "yes" -a "$set_more_warnings" != "no"; then
    AC_MSG_RESULT(yes)
    CFLAGS="\
    -Wall \
    -Winline \
    -Wchar-subscripts -Wmissing-declarations -Wmissing-prototypes \
    -Wnested-externs -Wpointer-arith \
    -Wcast-align -Wsign-compare \
    -std=gnu89 \
    $CFLAGS"
    CXXFLAGS="-Wall -std=c++98 $CXXFLAGS"
else
    AC_MSG_RESULT(no)
fi

dnl CXXFLAGS="-fvisibility=hidden -fvisibility-inlines-hidden $CXXFLAGS"
dnl CXXFLAGS="-fvisibility-inlines-hidden $CXXFLAGS"

GLIB_GSETTINGS

# i18n stuff
IT_PROG_INTLTOOL([0.41.0])

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.17])

GETTEXT_PACKAGE=gnome-system-monitor
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE",
                   [The gettext catalog name])

# Documentation
YELP_HELP_INIT

AC_CONFIG_FILES([
Makefile
src/Makefile
pixmaps/Makefile
po/Makefile.in
help/Makefile
gnome-system-monitor.desktop.in
])

AC_OUTPUT

echo "

Configuration:

	Source code location:	${srcdir}
	C Compiler:		${CC}
	C++ Compiler:		${CXX}
	CFLAGS:			${CFLAGS}
	CXXFLAGS:		${CXXFLAGS}
	Maintainer mode:	${USER_MAINTAINER_MODE}
	Languages:		${ALL_LINGUAS}
        systemd support:        ${have_systemd}
"
