# Note that this is NOT a relocatable package

%define libgnomeui_version 2.2.0
%define libgtop2_version 2.9.5
%define libwnck_version 2.5.0
%define pango_version 1.2.0
%define gtk2_version 2.5.0
%define desktop_file_utils_version 0.2.90

Summary: Simple process monitor
Name: gnome-system-monitor
Version: 2.9.92
Release: 1
License: GPL
Group: Applications/System
Source: http://download.gnome.org/GNOME/pre-gnome2/sources/gnome-system-monitor/gnome-system-monitor-%{version}.tar.bz2
BuildRoot: %{_tmppath}/%{name}-root
Requires: libgnomeui >= %{libgnomeui_version}
Requires: libgtop2 >= %{libgtop2_version}
Requires: libwnck >= %{libwnck_version}
BuildRequires: libgnomeui-devel >= %{libgnomeui_version}
BuildRequires: libgtop2-devel >= %{libgtop2_version}
BuildRequires: libwnck-devel >= %{libwnck_version}
BuildRequires: pango-devel >= %{pango_version}
BuildRequires: gtk2-devel >= %{gtk2_version}
BuildRequires: desktop-file-utils >= %{desktop_file_utils_version}
BuildRequires: startup-notification-devel
BuildRequires: libtool autoconf automake16
BuildRequires: intltool scrollkeeper gettext

Obsoletes: gtop

%description
gnome-system-monitor is a simple process and system monitor.

%prep
%setup -q -n gnome-system-monitor-%{version}

%build

autoconf
%configure

make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT

export GCONF_DISABLE_MAKEFILE_SCHEMA_INSTALL=1
%makeinstall
unset GCONF_DISABLE_MAKEFILE_SCHEMA_INSTALL

desktop-file-install --vendor gnome --delete-original       \
  --dir $RPM_BUILD_ROOT%{_datadir}/applications             \
  --add-only-show-in GNOME                                  \
  --add-category X-Red-Hat-Base                             \
  $RPM_BUILD_ROOT%{_datadir}/applications/*

rm -rf $RPM_BUILD_ROOT/var/scrollkeeper
%find_lang %{name}

%clean
rm -rf $RPM_BUILD_ROOT

%post
export GCONF_CONFIG_SOURCE=`gconftool-2 --get-default-source`
gconftool-2 --makefile-install-rule %{_sysconfdir}/gconf/schemas/gnome-system-monitor.schemas > /dev/null

%files -f %{name}.lang
%defattr(-, root, root,-)

%{_bindir}/gnome-system-monitor
%{_datadir}/applications
%{_datadir}/gnome
%{_datadir}/applications
%{_sysconfdir}/gconf/schemas/*
%{_datadir}/omf

%changelog
* Fri Feb 25 2005 Benoît Dejean <TazForEver@dlfp.org>
- Steal Fedora .spec
- Update dependencies
- Remove libgnomesu patch

* Wed Feb  9 2005 Matthias Clasen <mclasen@redhat.com> 2.9.91-1
- Update to 2.9.91

* Wed Feb  2 2005 Matthias Clasen <mclasen@redhat.com> 2.9.90-1
- Update to 2.9.90
- Remove libgnomesu dependency

* Sun Jan 30 2005 Florian La Roche <laroche@redhat.com>
- rebuild against new libs

* Mon Dec 20 2004 Christopher Aillon <caillon@redhat.com> 2.8.1-1
- Update to 2.8.1

* Wed Nov 03 2004 Christopher Aillon <caillon@redhat.com> 2.8.0-1
- Update to 2.8.0

* Wed Sep 01 2004 Florian La Roche <Florian.LaRoche@redhat.de>
- rebuild

* Fri Aug 06 2004 Christopher Aillon <caillon@redhat.com> 2.7.0-1
- Update to 2.7.0

* Tue Jun 15 2004 Elliot Lee <sopwith@redhat.com>
- rebuilt

* Mon May 03 2004 Than Ngo <than@redhat.com> 2.6.0-3
- cleanup GNOME/KDE menu, only show in GNOME

* Tue Apr 13 2004 Warren Togami <wtogami@redhat.com> 2.6.0-2
- #110918 BR intltool scrollkeeper gettext

* Fri Apr  2 2004 Alex Larsson <alexl@redhat.com> 2.6.0-1
- update to 2.6.0

* Thu Feb 26 2004 Alexander Larsson <alexl@redhat.com> 2.5.3-1
- update to 2.5.3

* Fri Feb 13 2004 Elliot Lee <sopwith@redhat.com>
- rebuilt

* Tue Jan 27 2004 Alexander Larsson <alexl@redhat.com> 2.5.2-1
- update to 2.5.2

* Fri Oct  3 2003 Alexander Larsson <alexl@redhat.com> 2.4.0-1
- 2.4.0

* Tue Aug 19 2003 Alexander Larsson <alexl@redhat.com> 2.3.0-1
- update for gnome 2.3

* Mon Jul 28 2003 Havoc Pennington <hp@redhat.com> 2.0.5-3
- rebuild, require newer libgtop2

* Fri Jul 18 2003  <timp@redhat.com> 2.0.5-2
- rebuild against newer libgtop

* Tue Jul  8 2003 Havoc Pennington <hp@redhat.com> 2.0.5-1
- 2.0.5

* Wed Jun 04 2003 Elliot Lee <sopwith@redhat.com>
- rebuilt

* Fri Feb 14 2003 Havoc Pennington <hp@redhat.com> 2.0.4-2
- don't buildreq xft

* Wed Feb  5 2003 Havoc Pennington <hp@redhat.com> 2.0.4-1
- 2.0.4

* Wed Jan 22 2003 Tim Powers <timp@redhat.com>
- rebuilt

* Tue Dec  3 2002 Tim Powers <timp@redhat.com> 2.0.3-1
- updated to 2.0.3

* Fri Jun 21 2002 Tim Powers <timp@redhat.com>
- automated rebuild

* Sun Jun 16 2002 Havoc Pennington <hp@redhat.com>
- 2.0.0
- add a bunch of build requires
- put omf in file list
- fix location of desktop file
- use desktop-file-install

* Fri Jun 07 2002 Havoc Pennington <hp@redhat.com>
- rebuild in different environment

* Wed Jun  5 2002 Havoc Pennington <hp@redhat.com>
- build with new libs

* Sun May 26 2002 Tim Powers <timp@redhat.com>
- automated rebuild

* Tue May 21 2002 Havoc Pennington <hp@redhat.com>
- rebuild in different environment

* Tue May 21 2002 Havoc Pennington <hp@redhat.com>
- obsolete gtop

* Tue May 21 2002 Havoc Pennington <hp@redhat.com>
- 1.1.7

* Fri May  3 2002 Havoc Pennington <hp@redhat.com>
- rebuild with new libs

* Thu Apr 18 2002 Havoc Pennington <hp@redhat.com>
- 1.1.6

* Wed Jan 30 2002 Owen Taylor <otaylor@redhat.com>
- Version 1.1.3 (should rename package to gnome-system-monitor as upstream)

* Thu Jan 10 2002 Havoc Pennington <hp@pobox.com>
- make spec "Red Hat style"
- add GConf stuff
- langify

