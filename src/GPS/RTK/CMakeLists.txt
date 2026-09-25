# Local RTK receiver sessions and connection policy. The application supplies Facts and serial ports.
if(DEFINED QGC_GPS_COMPONENTS_ENABLED AND NOT "RTK" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
    return()
endif()

qt_add_library(QGCGPSRTK STATIC GPSProvider.cc GPSRtk.cc RTKConnectionPolicy.cc)
add_library(QGC::GPSRTK ALIAS QGCGPSRTK)
set_target_properties(QGCGPSRTK PROPERTIES AUTOMOC ON)
target_compile_features(QGCGPSRTK PUBLIC cxx_std_20)
target_link_libraries(
    QGCGPSRTK
    PUBLIC QGCGPSCore QGCGPSCorrections QGCGPSPositioning QGCGPSReceiver QGCGPSTransport
    PRIVATE QGCGPSCorrectionService QGCGPSDriver QGCGPSReceiverTransports QGCGPSRTCM QGCLoggingCategory QGCTiming
)
target_sources(
    QGCGPSRTK
    PUBLIC FILE_SET
           HEADERS
           FILES
           GPSProvider.h
           GPSRtk.h
           GPSSerialPorts.h
           RTKConnectionPolicy.h
           RTKConnectionTarget.h
)
# GPSRtk.h declares its serial API only in builds with serial links.
if(QGC_NO_SERIAL_LINK)
    target_compile_definitions(QGCGPSRTK PUBLIC QGC_NO_SERIAL_LINK)
endif()

if(TARGET ${CMAKE_PROJECT_NAME})
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSRTK)
endif()
