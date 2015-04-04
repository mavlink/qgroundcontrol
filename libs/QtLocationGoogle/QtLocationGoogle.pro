TARGET       = qtgeoservices_google
CONFIG      += static
QT          += location-private positioning-private network
PLUGIN_TYPE  = geoservices

PLUGIN_CLASS_NAME = QGeoServiceProviderFactoryGoogle

load(qt_plugin)

INCLUDEPATH += $$QT.location.includes

HEADERS += \
    $$PWD/src/qgeoserviceproviderplugingoogle.h \
    $$PWD/src/qgeotiledmappingmanagerenginegoogle.h \
    $$PWD/src/qgeotilefetchergoogle.h \
    $$PWD/src/qgeomapreplygoogle.h \
    $$PWD/src/qgeocodingmanagerenginegoogle.h \
    $$PWD/src/qgeocodereplygoogle.h


SOURCES += \
    $$PWD/src/qgeoserviceproviderplugingoogle.cpp \
    $$PWD/src/qgeotiledmappingmanagerenginegoogle.cpp \
    $$PWD/src/qgeotilefetchergoogle.cpp \
    $$PWD/src/qgeomapreplygoogle.cpp \
    $$PWD/src/qgeocodingmanagerenginegoogle.cpp \
    $$PWD/src/qgeocodereplygoogle.cpp

OTHER_FILES += \
    $$PWD/google_maps_plugin.json

