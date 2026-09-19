add_library(QGCGPSAshtech STATIC AshtechConfiguration.cc AshtechDecoder.cc AshtechPrivate.h GPSDriverAshtech.cc
                                 GPSDriverAshtech.h
)
set_target_properties(
    QGCGPSAshtech
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
)
target_link_libraries(QGCGPSAshtech PUBLIC QGCGPSNativeCommon)
add_library(QGC::GPSAshtech ALIAS QGCGPSAshtech)
