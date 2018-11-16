CONTRIB_CFLAGS = -I$(top_srcdir) -DDEBUG -DDEBUG2

# Extra files to distribute in the source tarball
EXTRA_DIST = makefile.vc tests/shpproj.sh doc/Shape_PointInPoly_README.txt doc/shpproj.txt doc/shpsort.txt ShapeFileII.pas

# Installed executables
bin_PROGRAMS = dbfcat dbfinfo shpcat shpdxf shpfix shpsort Shape_PointInPoly shpcentrd shpdata shpinfo shpproj shpwkb

dbfcat_SOURCES = dbfcat.c
dbfcat_CPPFLAGS = $(CONTRIB_CFLAGS)
dbfcat_LDADD = $(top_builddir)/libshp.la

dbfinfo_SOURCES = dbfinfo.c
dbfinfo_CPPFLAGS = $(CONTRIB_CFLAGS)
dbfinfo_LDADD = $(top_builddir)/libshp.la

shpcat_SOURCES = shpcat.c
shpcat_CPPFLAGS = $(CONTRIB_CFLAGS)
shpcat_LDADD = $(top_builddir)/libshp.la

shpdxf_SOURCES = shpdxf.c
shpdxf_CPPFLAGS = $(CONTRIB_CFLAGS)
shpdxf_LDADD = $(top_builddir)/libshp.la

shpfix_SOURCES = shpfix.c
shpfix_CPPFLAGS = $(CONTRIB_CFLAGS)
shpfix_LDADD = $(top_builddir)/libshp.la

shpsort_SOURCES = shpsort.c
shpsort_CPPFLAGS = $(CONTRIB_CFLAGS)
shpsort_LDADD = $(top_builddir)/libshp.la -lm

Shape_PointInPoly_SOURCES = Shape_PointInPoly.cpp
Shape_PointInPoly_CPPFLAGS = $(CONTRIB_CFLAGS)
Shape_PointInPoly_LDADD = $(top_builddir)/libshp.la

shpcentrd_SOURCES = shpcentrd.c shpgeo.c shpgeo.h
shpcentrd_CPPFLAGS = $(CONTRIB_CFLAGS) $(PROJ_CFLAGS)
shpcentrd_LDADD = $(top_builddir)/libshp.la $(PROJ_LIBS) -lm

shpdata_SOURCES = shpdata.c shpgeo.c shpgeo.h
shpdata_CPPFLAGS = $(CONTRIB_CFLAGS) $(PROJ_CFLAGS)
shpdata_LDADD = $(top_builddir)/libshp.la $(PROJ_LIBS) -lm

shpinfo_SOURCES = shpinfo.c shpgeo.c shpgeo.h
shpinfo_CPPFLAGS = $(CONTRIB_CFLAGS) $(PROJ_CFLAGS)
shpinfo_LDADD = $(top_builddir)/libshp.la $(PROJ_LIBS) -lm

shpproj_SOURCES = shpproj.c shpgeo.c shpgeo.h
shpproj_CPPFLAGS = $(CONTRIB_CFLAGS) $(PROJ_CFLAGS)
shpproj_LDADD = $(top_builddir)/libshp.la $(PROJ_LIBS) -lm

shpwkb_SOURCES = shpwkb.c shpgeo.c shpgeo.h
shpwkb_CPPFLAGS = $(CONTRIB_CFLAGS) $(PROJ_CFLAGS)
shpwkb_LDADD = $(top_builddir)/libshp.la $(PROJ_LIBS) -lm

# Tests
TESTS_ENVIRONMENT = top_builddir=$(abs_top_builddir)
TESTS = tests/shpproj.sh
