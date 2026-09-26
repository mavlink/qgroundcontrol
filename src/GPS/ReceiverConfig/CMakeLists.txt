if(TARGET QGCGPSReceiverConfig)
    return()
endif()

qt_add_library(QGCGPSReceiverConfig STATIC GPSBaseStationSettings.cc GPSReceiverConfig.cc GPSReceiverDescriptor.cc)
add_library(QGC::GPSReceiverConfig ALIAS QGCGPSReceiverConfig)
set_target_properties(
    QGCGPSReceiverConfig
    PROPERTIES AUTOMOC ON
               AUTOUIC OFF
               AUTORCC OFF
               POSITION_INDEPENDENT_CODE ON
)
target_compile_features(QGCGPSReceiverConfig PUBLIC cxx_std_20)
target_link_libraries(QGCGPSReceiverConfig PUBLIC Qt6::Core QGCGPSCommon)
target_sources(
    QGCGPSReceiverConfig
    PUBLIC FILE_SET
           HEADERS
           FILES
           GPSReceiverConfig.h
           GPSReceiverCapabilities.h
           GPSReceiverDescriptor.h
           GPSBaseStationConfig.h
           GPSBaseStationSettings.h
)
