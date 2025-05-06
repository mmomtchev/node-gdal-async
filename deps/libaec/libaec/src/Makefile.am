AUTOMAKE_OPTIONS = no-installman
AM_CFLAGS = $(CFLAG_VISIBILITY)
AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include
lib_LTLIBRARIES = libaec.la libsz.la
libaec_la_SOURCES = encode.c encode_accessors.c decode.c vector.c\
encode.h encode_accessors.h decode.h vector.h
libaec_la_CPPFLAGS = $(AM_CPPFLAGS) -DLIBAEC_BUILD
libaec_la_LDFLAGS = -version-info 1:3:1 -no-undefined

libsz_la_SOURCES = sz_compat.c
libsz_la_LIBADD = libaec.la
libsz_la_CPPFLAGS = $(AM_CPPFLAGS) -DLIBAEC_BUILD
libsz_la_LDFLAGS = -version-info 2:1:0 -no-undefined

include_HEADERS = $(top_builddir)/include/libaec.h $(top_srcdir)/include/szlib.h

noinst_PROGRAMS = utime graec
utime_SOURCES = utime.c
graec_LDADD = libaec.la
graec_SOURCES = graec.c
dist_man_MANS = graec.1

EXTRA_DIST = CMakeLists.txt benc.sh bdec.sh
CLEANFILES = bench.dat bench.rz

bench-local: all benc bdec
benc-local: all
	$(srcdir)/benc.sh $(top_srcdir)/data/typical.rz
bdec-local: all
	top_srcdir=$(top_srcdir) $(srcdir)/bdec.sh
