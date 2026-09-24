if(TARGET QGCGPSReceiver)
    return()
endif()

if(NOT TARGET QGCClock)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/Timing/Clock" "${CMAKE_CURRENT_BINARY_DIR}/Clock")
endif()

qt_add_library(QGCGPSReceiver STATIC GPSReceiverConfig.cc GPSReceiverDescriptor.cc GPSSatelliteObservation.cc)
add_library(QGC::GPSReceiver ALIAS QGCGPSReceiver)
set_target_properties(
    QGCGPSReceiver
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
               POSITION_INDEPENDENT_CODE ON
)
target_compile_features(QGCGPSReceiver PUBLIC cxx_std_20)
target_link_libraries(QGCGPSReceiver PUBLIC Qt6::Core QGCClock)
target_sources(
    QGCGPSReceiver
    PUBLIC FILE_SET
           HEADERS
           FILES
           GPSReceiverConfig.h
           GPSReceiverCapabilities.h
           GPSReceiverDescriptor.h
           GPSBaseStationConfig.h
           GPSEllipsoidPosition.h
           GPSType.h
           GPSDriverReports.h
           GPSConfigurationEvidence.h
           GPSSatelliteUsageReport.h
           GPSSatelliteObservation.h
           GPSSatelliteState.h
)
target_sources(
    QGCGPSReceiver
    PUBLIC FILE_SET
           constellation_types
           TYPE
           HEADERS
           BASE_DIRS
           ..
           FILES
           ../GPSConstellation.h
           ../GPSFixQuality.h
)
