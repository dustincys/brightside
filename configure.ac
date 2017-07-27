AC_INIT(Brightside, 1.4.0, http://catmur.co.uk/brightside/, brightside)

AC_PREREQ(2.58)

AM_CONFIG_HEADER(config.h)
AC_CONFIG_SRCDIR(configure.ac)

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AM_MAINTAINER_MODE

AC_DEFINE(PACKAGE, AC_PACKAGE_NAME, [package name])
AC_DEFINE(VERSION, AC_PACKAGE_VERSION, [package version])
AC_DEFINE(GETTEXT_PACKAGE, "AC_PACKAGE_NAME", [foo])
GETTEXT_PACKAGE=AC_PACKAGE_NAME
AC_SUBST(GETTEXT_PACKAGE)
AM_GNU_GETTEXT([external])

AC_PROG_INTLTOOL([0.20])

PKG_CHECK_MODULES(EXTRA_GNOME, libgnomeui-2.0 libglade-2.0 libwnck-1.0 >= 2.2)
AC_SUBST(EXTRA_GNOME_CFLAGS)
AC_SUBST(EXTRA_GNOME_LIBS)

AC_PATH_PROG(GCONFTOOL, gconftool-2)
AM_GCONF_SOURCE_2

AC_ARG_ENABLE(tray-icon, AC_HELP_STRING([--enable-tray-icon],
	      [enable notification area icon [[no]]]), 
	      [enable_tray_icon=yes], [enable_tray_icon=no])
AM_CONDITIONAL(ENABLE_TRAY_ICON, test x"$enable_tray_icon" = "xyes")

dnl
dnl Test for Audio interfaces and FB-Level compatibility
dnl
have_oss=no
AC_TRY_COMPILE([
	#ifdef __NetBSD__
	#include <soundcard.h>
	#else
	#include <sys/soundcard.h>
	#endif
	],[
	int arg = SNDCTL_DSP_SETFRAGMENT;
	],[
	have_oss=yes
])
AC_MSG_CHECKING(for OSS audio support)
AC_MSG_RESULT($have_oss)
AM_CONDITIONAL(HAVE_OSS, test x"$have_oss" = "xyes")

have_alsa=no
PKG_CHECK_MODULES(ALSA, alsa >= 0.9.0,
        have_alsa=yes,
        AC_MSG_RESULT(*** All of ALSA dependent parts will be disabled ***))
AM_CONDITIONAL(HAVE_ALSA, test x"$have_alsa" = "xyes")
AC_SUBST(ALSA_LIBS)
AC_SUBST(ALSA_CFLAGS)
 
have_fb=no
AC_TRY_COMPILE([
	#include <linux/fb.h>
	#include <linux/pmu.h>
	],[
	int arg = 0;
	],[
	have_fb=yes
])
AC_MSG_CHECKING(for Brightness control support)
AC_MSG_RESULT($have_fb)
AM_CONDITIONAL(HAVE_FB, test x"$have_fb" = "xyes")

dnl Add the languages which your application supports here.
ALL_LINGUAS="am ar az be bg bn ca cs cy da de el es et eu fa fi fr ga he hi hu id is it ja ko li lt lv mk ml mn ms nl no pl pt pt_BR ro ru sl sk sq sr sr@Latn sv tr uk vi zh_CN zh_TW"
AM_GLIB_GNU_GETTEXT

AC_OUTPUT([ po/Makefile.in
Makefile
brightside.spec
src/Makefile
])

dnl Summary
echo
echo "Summary"
echo
echo "alsa          : "$have_alsa
echo "oss           : "$have_oss
echo "brightness    : "$have_fb
echo "tray icon     : "$enable_tray_icon
echo
