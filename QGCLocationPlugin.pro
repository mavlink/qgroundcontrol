include(QGCCommon.pri)

TEMPLATE     = lib
TARGET       = QGeoServiceProviderFactoryQGC
CONFIG      += plugin static
QT          += location-private positioning-private network
PLUGIN_TYPE  = geoservices

DESTDIR      = $${LOCATION_PLUGIN_DESTDIR}

contains(QT_VERSION, 5.5.1) {

    message(Using Local QtLocation headers for Qt 5.5.1)

    INCLUDEPATH += \
        libs/qtlocation/include \

} else {

    message(Using Default QtLocation headers)

    INCLUDEPATH += $$QT.location.includes

}

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
