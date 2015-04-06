TARGET       = qtgeoservices_qgc
CONFIG      += static
QT          += location-private positioning-private network
PLUGIN_TYPE  = geoservices

PLUGIN_CLASS_NAME = QGeoServiceProviderFactoryQGC

load(qt_plugin)

INCLUDEPATH += $$QT.location.includes

HEADERS += \
    $$PWD/src/qgeoserviceproviderpluginqgc.h \
    $$PWD/src/qgeotiledmappingmanagerengineqgc.h \
    $$PWD/src/qgeotilefetcherqgc.h \
    $$PWD/src/qgeomapreplyqgc.h \
    $$PWD/src/qgeocodingmanagerengineqgc.h \
    $$PWD/src/qgeocodereplyqgc.h \
    $$PWD/src/OpenPilotMaps.h

SOURCES += \
    $$PWD/src/qgeoserviceproviderpluginqgc.cpp \
    $$PWD/src/qgeotiledmappingmanagerengineqgc.cpp \
    $$PWD/src/qgeotilefetcherqgc.cpp \
    $$PWD/src/qgeomapreplyqgc.cpp \
    $$PWD/src/qgeocodingmanagerengineqgc.cpp \
    $$PWD/src/qgeocodereplyqgc.cpp \
    $$PWD/src/OpenPilotMaps.cc

OTHER_FILES += \
    $$PWD/qgc_maps_plugin.json

