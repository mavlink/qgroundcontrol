include(QGCCommon.pri)

TEMPLATE     = lib
TARGET       = QGeoServiceProviderFactoryQGC
CONFIG      += plugin static
QT          += location-private positioning-private network
PLUGIN_TYPE  = geoservices

DESTDIR      = $${LOCATION_PLUGIN_DESTDIR}

INCLUDEPATH += $$QT.location.includes

HEADERS += \
    src/QtLocationPlugin/qgeoserviceproviderpluginqgc.h \
    src/QtLocationPlugin/qgeotiledmappingmanagerengineqgc.h \
    src/QtLocationPlugin/qgeotilefetcherqgc.h \
    src/QtLocationPlugin/qgeomapreplyqgc.h \
    src/QtLocationPlugin/qgeocodingmanagerengineqgc.h \
    src/QtLocationPlugin/qgeocodereplyqgc.h \
    src/QtLocationPlugin/OpenPilotMaps.h

SOURCES += \
    src/QtLocationPlugin/qgeoserviceproviderpluginqgc.cpp \
    src/QtLocationPlugin/qgeotiledmappingmanagerengineqgc.cpp \
    src/QtLocationPlugin/qgeotilefetcherqgc.cpp \
    src/QtLocationPlugin/qgeomapreplyqgc.cpp \
    src/QtLocationPlugin/qgeocodingmanagerengineqgc.cpp \
    src/QtLocationPlugin/qgeocodereplyqgc.cpp \
    src/QtLocationPlugin/OpenPilotMaps.cc

OTHER_FILES += \
    src/QtLocationPlugin/qgc_maps_plugin.json
