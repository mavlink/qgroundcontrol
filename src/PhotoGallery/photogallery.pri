# This includes the files that can be compiled hermetically (independent of
# remainder of QGC, and split out to allow unit tests) as well as those
# that cannot.

include($$PWD/photogallery_hermetic.pri)

SOURCES += \
    $$PWD/PhotoGalleryVehicleGlue.cc \

HEADERS += \
    $$PWD/PhotoGalleryVehicleGlue.h \

