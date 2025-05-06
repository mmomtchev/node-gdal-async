AUTOMAKE_OPTIONS = color-tests
AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include
TESTS = check_code_options check_buffer_sizes check_long_fs \
check_seeking check_rsi_block_access szcomp.sh sampledata.sh
TEST_EXTENSIONS = .sh
CLEANFILES = test.dat test.rz
check_LTLIBRARIES = libcheck_aec.la
libcheck_aec_la_SOURCES = check_aec.c check_aec.h
check_PROGRAMS = check_code_options check_buffer_sizes check_long_fs \
check_szcomp check_seeking check_rsi_block_access

check_code_options_SOURCES = check_code_options.c check_aec.h \
$(top_builddir)/include/libaec.h

check_buffer_sizes_SOURCES = check_buffer_sizes.c check_aec.h \
$(top_builddir)/include/libaec.h

check_long_fs_SOURCES = check_long_fs.c check_aec.h \
$(top_builddir)/include/libaec.h

check_seeking_SOURCES = check_seeking.c check_aec.h \
$(top_builddir)/include/libaec.h

check_rsi_block_access_SOURCES = check_rsi_block_access.c check_aec.h \
$(top_builddir)/include/libaec.h $(top_builddir)/src/decode.h

check_szcomp_SOURCES = check_szcomp.c $(top_srcdir)/include/szlib.h

LDADD = libcheck_aec.la $(top_builddir)/src/libaec.la
check_szcomp_LDADD = $(top_builddir)/src/libsz.la

EXTRA_DIST = sampledata.sh szcomp.sh CMakeLists.txt

szcomp.log: sampledata.log
