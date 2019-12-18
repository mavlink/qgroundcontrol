# PhotoGallery modules that can be built hermetically (i.e. those that are not
# dependent on the remainder of QGC).

QT += network qml quick

SOURCES += \
    $$PWD/AbstractPhotoTrigger.cc \
    $$PWD/AsyncDownloadPhotoTrigger.cc \
    $$PWD/ExtractJPEGMetadata.cc \
    $$PWD/JPEGSegmentParser.cc \
    $$PWD/PhotoFileStore.cc \
    $$PWD/PhotoFileStoreInterface.cc \
    $$PWD/PhotoGalleryModel.cc \
    $$PWD/PhotoGalleryView.cc \
    $$PWD/easyexif.cpp \

HEADERS += \
    $$PWD/AbstractPhotoTrigger.h \
    $$PWD/AsyncDownloadPhotoTrigger.h \
    $$PWD/ExtractJPEGMetadata.h \
    $$PWD/JPEGSegmentParser.h \
    $$PWD/PhotoFileStore.h \
    $$PWD/PhotoFileStoreInterface.h \
    $$PWD/PhotoGalleryModel.h \
    $$PWD/PhotoGalleryView.h \
    $$PWD/easyexif.h \
