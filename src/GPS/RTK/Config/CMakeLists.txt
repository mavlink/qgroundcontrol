if(TARGET QGCGPSReceiverConfig)
    return()
endif()

add_library(QGCGPSReceiverConfig STATIC ../GPSReceiverConfig.cc)
add_library(QGC::GPSReceiverConfig ALIAS QGCGPSReceiverConfig)
set_target_properties(
    QGCGPSReceiverConfig
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
               POSITION_INDEPENDENT_CODE ON
)
target_compile_features(QGCGPSReceiverConfig PUBLIC cxx_std_20)
target_sources(
    QGCGPSReceiverConfig
    PUBLIC FILE_SET
           HEADERS
           BASE_DIRS
           ..
           FILES
           ../GPSReceiverConfig.h
           ../GPSReceiverCapabilities.h
           ../GPSBaseStationConfig.h
           ../GPSType.h
)
