SUBDIRS = po src

install-data-local:
	@$(NORMAL_INSTALL)
	
EXTRA_DIST = intltool-extract.in	\
	intltool-merge.in	\
	intltool-update.in	\
	AUTHORS			\
	brightside.spec.in		\
	brightside.spec

CLEANFILES = brightside.spec

srcdist:
	cp -a . ../$(PACKAGE_TARNAME)-$(VERSION)
	make -C ../$(PACKAGE_TARNAME)-$(VERSION) distclean
	rm -r ../$(PACKAGE_TARNAME)-$(VERSION)/autom4te.cache
	tar jcvf ../$(PACKAGE_TARNAME)-$(VERSION).tar.bz2 -C .. $(PACKAGE_TARNAME)-$(VERSION)
	rm -r ../$(PACKAGE_TARNAME)-$(VERSION)

edigest: srcdist
	rm -f ../ebuild/gnome-extra/$(PACKAGE_TARNAME)/files/digest-$(PACKAGE_TARNAME)-$(VERSION)
	env DISTDIR=.. PORTDIR_OVERLAY=../ebuild GENTOO_MIRRORS="" /usr/sbin/ebuild ../ebuild/gnome-extra/$(PACKAGE_TARNAME)/$(PACKAGE_TARNAME)-$(VERSION).ebuild digest
