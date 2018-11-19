## Process this file with automake to produce Makefile.in

SUBDIRS = . contrib

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS =      dist-zip

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

# Extra files to distribute in the source tarball
EXTRA_DIST = makefile.vc CMakeLists.txt autogen.sh \
		tests/test1.sh tests/test2.sh tests/test3.sh \
		tests/stream1.out tests/stream1.out tests/stream1.out \
		web/maptools.css \
		web/codepage.html \
		web/index.html \
		web/shapelib-tools.html \
		web/shp_api.html \
		web/release.html \
		web/dbf_api.html \
		web/license.html \
		web/manifest.html \
		README.tree README.CMake

# pkg-config file
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = shapelib.pc

# Library
lib_LTLIBRARIES = libshp.la
libshp_la_includedir = $(includedir)
libshp_la_include_HEADERS = shapefil.h
libshp_la_SOURCES = shpopen.c dbfopen.c safileio.c shptree.c sbnsearch.c
libshp_la_LDFLAGS = -version-info $(SHAPELIB_SO_VERSION) $(no_undefined) $(LIBM)

# Installed executables
bin_PROGRAMS = dbfadd dbfcreate dbfdump shpadd shpcreate shpdump shprewind shptreedump shputils

dbfadd_SOURCES = dbfadd.c
dbfadd_LDADD = $(top_builddir)/libshp.la

dbfcreate_SOURCES = dbfcreate.c
dbfcreate_LDADD = $(top_builddir)/libshp.la

dbfdump_SOURCES = dbfdump.c
dbfdump_LDADD = $(top_builddir)/libshp.la

shpadd_SOURCES = shpadd.c
shpadd_LDADD = $(top_builddir)/libshp.la

shpcreate_SOURCES = shpcreate.c
shpcreate_LDADD = $(top_builddir)/libshp.la

shpdump_SOURCES = shpdump.c
shpdump_LDADD = $(top_builddir)/libshp.la

shprewind_SOURCES = shprewind.c
shprewind_LDADD = $(top_builddir)/libshp.la

shptreedump_SOURCES = shptreedump.c
shptreedump_LDADD = $(top_builddir)/libshp.la

shputils_SOURCES = shputils.c
shputils_LDADD = $(top_builddir)/libshp.la

# Non-installed executables
noinst_PROGRAMS = shptest
shptest_SOURCES = shptest.c
shptest_LDADD = $(top_builddir)/libshp.la

# Tests
TESTS_ENVIRONMENT = top_builddir=$(abs_top_builddir)
# tests/test1.sh requires ftp://gdal.velocet.ca/pub/outgoing/shape_eg_data.zip
TESTS = tests/test2.sh tests/test3.sh
