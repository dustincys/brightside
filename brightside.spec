Summary:	Brightside Screen Corners and Edges Daemon
Name:           brightside
Version:        1.4.0
Release:        1

Group:          System Environment/Daemons 
License:      	GPL
URL:            http://catmur.co.uk/brightside/
Source:         %{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-root

Requires:	libgnomeui >= 2.0.0
Requires:	libglade2 >= 2.0.0
BuildRequires:  libgnomeui-devel >= 2.0.0
BuildRequires:  libglade2-devel >= 2.0.0
BuildRequires:  libwnck-devel >= 2.1.5

%description
Brightside is a small Gnome tool to enable edge flipping between workspaces and
actions to take place when the mouse rests in the corner of the screen: for 
instance, muting the volume or launching the screensaver to lock the screen.
%prep
%setup -q

%build
if [ ! -f configure ]; then
  CFLAGS="$RPM_OPT_FLAGS" ./autogen.sh --prefix=/usr --enable-shared
else
  CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=/usr --enable-shared
fi

export GCONF_DISABLE_MAKEFILE_SCHEMA_INSTALL=1
make

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%makeinstall

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%post
export GCONF_CONFIG_SOURCE=`gconftool-2 --get-default-source`
gconftool-2 --makefile-install-rule %{_sysconfdir}/gconf/schemas/brightside.schemas > /dev/null

%postun
/sbin/ldconfig

%files
%defattr(644,root,root,755)
%attr(755,root,root) %{_bindir}/*
%{_datadir}/pixmaps/*
%{_datadir}/brightside/*
%{_datadir}/control-center-2.0/capplets/*
%{_datadir}/locale/*/*/*
%{_sysconfdir}/gconf/schemas/brightside.schemas

%changelog
* Thu Oct 31 2002 Thomas Vander Stichele <thomas at apestaart dot org>
- fix requires and buildrequires

* Mon Sep 23 2002 Bastien Nocera <hadess@hadess.net>
- More hacking

* Mon Sep 23 2002 Christian Fredrik Kalager Schaller <Uraeus@linuxrising.org>
- First attempt at SPEC
