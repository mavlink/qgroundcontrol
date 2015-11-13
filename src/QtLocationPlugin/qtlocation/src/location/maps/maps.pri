
INCLUDEPATH += maps

QT += gui quick

PUBLIC_HEADERS += \
                    maps/qgeocodereply.h \
                    maps/qgeocodingmanagerengine.h \
                    maps/qgeocodingmanager.h \
                    maps/qgeomaneuver.h \
                    maps/qgeoroute.h \
                    maps/qgeoroutereply.h \
                    maps/qgeorouterequest.h \
                    maps/qgeoroutesegment.h \
                    maps/qgeoroutingmanagerengine.h \
                    maps/qgeoroutingmanager.h \
                    maps/qgeoserviceproviderfactory.h \
                    maps/qgeoserviceprovider.h

PRIVATE_HEADERS += \
                    maps/qgeocameracapabilities_p.h \
                    maps/qgeocameradata_p.h \
                    maps/qgeocameratiles_p.h \
                    maps/qgeocodereply_p.h \
                    maps/qgeocodingmanagerengine_p.h \
                    maps/qgeocodingmanager_p.h \
                    maps/qgeomaneuver_p.h \
                    maps/qgeomapcontroller_p.h \
                    maps/qgeomapscene_p.h \
                    maps/qgeotilerequestmanager_p.h \
                    maps/qgeomap_p.h \
                    maps/qgeomap_p_p.h \
                    maps/qgeotiledmap_p.h \
                    maps/qgeotiledmap_p_p.h \
                    maps/qgeotilefetcher_p.h \
                    maps/qgeotilefetcher_p_p.h \
                    maps/qgeomappingmanager_p.h \
                    maps/qgeomappingmanager_p_p.h \
                    maps/qgeomappingmanagerengine_p.h \
                    maps/qgeomappingmanagerengine_p_p.h \
                    maps/qgeotiledmappingmanagerengine_p.h \
                    maps/qgeotiledmappingmanagerengine_p_p.h \
                    maps/qgeomaptype_p.h \
                    maps/qgeomaptype_p_p.h \
                    maps/qgeoroute_p.h \
                    maps/qgeoroutereply_p.h \
                    maps/qgeorouterequest_p.h \
                    maps/qgeoroutesegment_p.h \
                    maps/qgeoroutingmanagerengine_p.h \
                    maps/qgeoroutingmanager_p.h \
                    maps/qgeoserviceprovider_p.h \
                    maps/qgeotilecache_p.h \
                    maps/qgeotiledmapreply_p.h \
                    maps/qgeotiledmapreply_p_p.h \
                    maps/qgeotilespec_p.h \
                    maps/qgeotilespec_p_p.h \
                    maps/qcache3q_p.h

SOURCES += \
            maps/qgeocameracapabilities.cpp \
            maps/qgeocameradata.cpp \
            maps/qgeocameratiles.cpp \
            maps/qgeocodereply.cpp \
            maps/qgeocodingmanager.cpp \
            maps/qgeocodingmanagerengine.cpp \
            maps/qgeomaneuver.cpp \
            maps/qgeomapcontroller.cpp \
            maps/qgeomapscene.cpp \
            maps/qgeotilerequestmanager.cpp \
            maps/qgeomap.cpp \
            maps/qgeomappingmanager.cpp \
            maps/qgeomappingmanagerengine.cpp \
            maps/qgeotiledmappingmanagerengine.cpp \
            maps/qgeotilefetcher.cpp \
            maps/qgeomaptype.cpp \
            maps/qgeoroute.cpp \
            maps/qgeoroutereply.cpp \
            maps/qgeorouterequest.cpp \
            maps/qgeoroutesegment.cpp \
            maps/qgeoroutingmanager.cpp \
            maps/qgeoroutingmanagerengine.cpp \
            maps/qgeoserviceprovider.cpp \
            maps/qgeoserviceproviderfactory.cpp \
            maps/qgeotilecache.cpp \
            maps/qgeotiledmapreply.cpp \
            maps/qgeotilespec.cpp \
            maps/qgeotiledmap.cpp


