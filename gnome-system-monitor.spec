# Note that this is NOT a relocatable package

Summary: Simple process monitor
Name: gnome-system-monitor
Version: 1.1.2
Release: 1
Copyright: GPL
Group: Applications/System
Source: http://www.personal.psu.edu/kfv101/procman/source/procman-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-root
Requires: libgnomeui >= 1.106.0
Requires: libgtop >= 1.90.0
Requires: libwnck >= 0.1

%description
Procman is a simple process and system monitor.

%prep
%setup -q

%build
%configure

make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT

export GCONF_DISABLE_MAKEFILE_SCHEMA_INSTALL=1
%makeinstall
unset GCONF_DISABLE_MAKEFILE_SCHEMA_INSTALL

%find_lang %{name}

%clean
rm -rf $RPM_BUILD_ROOT

%post
export GCONF_CONFIG_SOURCE=`gconftool-2 --get-default-source`
gconftool-2 --makefile-install-rule %{_sysconfdir}/gconf/schemas/gnome-system-monitor.schemas > /dev/null

%files -f %{name}.lang
%defattr(-, root, root)

%{_bindir}/gnome-system-monitor
%{_datadir}/applications/gnome-system-monitor.schemas
%{_sysconfdir}/gconf/schemas/*

%changelog

* Thu Jan 10 2002 Havoc Pennington <hp@pobox.com>
- make spec "Red Hat style"
- add GConf stuff
- langify

