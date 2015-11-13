
QT  += location-private positioning-private network

contains(QT_VERSION, 5.5.1) {

    message(Using Local QtLocation headers for Qt 5.5.1)

    INCLUDEPATH += \
        $$PWD/qtlocation/include \

} else {

    message(Using Default QtLocation headers)

    INCLUDEPATH += $$QT.location.includes

}

HEADERS += \
    $$PWD/qgeoserviceproviderpluginqgc.h \
    $$PWD/qgeotiledmappingmanagerengineqgc.h \
    $$PWD/qgeotilefetcherqgc.h \
    $$PWD/qgeomapreplyqgc.h \
    $$PWD/qgeocodingmanagerengineqgc.h \
    $$PWD/qgeocodereplyqgc.h \
    $$PWD/OpenPilotMaps.h

SOURCES += \
    $$PWD/qgeoserviceproviderpluginqgc.cpp \
    $$PWD/qgeotiledmappingmanagerengineqgc.cpp \
    $$PWD/qgeotilefetcherqgc.cpp \
    $$PWD/qgeomapreplyqgc.cpp \
    $$PWD/qgeocodingmanagerengineqgc.cpp \
    $$PWD/qgeocodereplyqgc.cpp \
    $$PWD/OpenPilotMaps.cc

OTHER_FILES += \
    $$PWD/qgc_maps_plugin.json
