# PhotoGallery modules that can be built hermetically (i.e. those that are not
# dependent on the remainder of QGC).

QT += network qml quick

SOURCES += \
    $$PWD/PhotoFileStore.cc \

HEADERS += \
    $$PWD/PhotoFileStore.h \
