# Note that this is NOT a relocatable package
%define ver      1.0
%define  RELEASE 1
%define  rel     %{?CUSTOM_RELEASE} %{!?CUSTOM_RELEASE:%RELEASE}
%define prefix   /usr

Summary: Simple process monitor
Name: procman
Version: %ver
Release: %rel
Copyright: GPL
Group: Applications/System
Source: http://www.personal.psu.edu/kfv101/procman/source/procman-%{ver}.tar.gz
BuildRoot: /var/tmp/procman-root
Requires: gnome-libs >= 1.2.5
Requires: gal >= 0.19
Requires: libgtop >= 1.0.6

%description
Procman is a simple process and system monitor.

%prep
%setup -q

%build
if [ ! -f configure ]; then
        CFLAGS="$RPM_OPT_FLAGS" ./autogen.sh $MYARCH_FLAGS --prefix=%prefix
else
        CFLAGS="$RPM_OPT_FLAGS" ./configure $MYARCH_FLAGS --prefix=%prefix
fi

if [ "$SMP" != "" ]; then
  make -j$SMP MAKE="make -j$SMP"
else
  make
fi


%install
rm -rf $RPM_BUILD_ROOT

make -k prefix=$RPM_BUILD_ROOT%{prefix} install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)

%{prefix}/bin/procman
%{prefix}/share/gnome/apps/System/procman.desktop
%{prefix}/share/procman/proctable.etspec
%{prefix}/share/procman/memmaps.etspec
%{prefix}/share/procman/simple.etspec
%{prefix}/share/locale/*/LC_MESSAGES/*.mo
%{prefix}/share/pixmaps/procman.png
