AUTOMAKE_OPTIONS = dist-bzip2

bin_PROGRAMS = wakkabox

imgdata = \
	bigblock.bmp \
	medblockv.bmp \
	medblockh.bmp \
	smallblock.bmp

confdir = $(sysconfdir)/wakkabox
conf_DATA = startconfig

AM_CFLAGS = \
	-DDATADIR=\"$(pkgdatadir)\" \
	-DCONFDIR=\"$(confdir)\"

wakkabox_SOURCES = \
	wakkabox.c

pkgdata_DATA = \
	$(imgdata)


EXTRA_DIST = \
	$(conf_DATA) \
	$(pkgdata_DATA)	\
	wakkabox-1.1.0.ebuild

