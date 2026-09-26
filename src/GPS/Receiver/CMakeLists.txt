# Local GPS receiver sessions and connection policy. The application supplies Facts and serial ports.
if(DEFINED QGC_GPS_COMPONENTS_ENABLED AND NOT "Receiver" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
    return()
endif()

qt_add_library(
    QGCGPSReceiver
    STATIC
    GPSProvider.cc
    GPSReceiver.cc
    GPSReceiverConnectionPolicy.cc
    GPSReceiverSession.cc
    GPSReceiverSession.h
)
add_library(QGC::GPSReceiver ALIAS QGCGPSReceiver)
set_target_properties(QGCGPSReceiver PROPERTIES AUTOMOC ON)
target_compile_features(QGCGPSReceiver PUBLIC cxx_std_20)
target_link_libraries(
    QGCGPSReceiver
    PUBLIC QGCGPSCommon QGCGPSCorrections QGCGPSPositioning QGCGPSReceiverConfig QGCReentrancy QGCGPSTransport
    PRIVATE QGCGPSCorrectionService QGCGPSProtocols QGCGPSReceiverTransports QGCGPSRTCM QGCLoggingCategory QGCTiming
)
target_sources(
    QGCGPSReceiver
    PUBLIC FILE_SET
           HEADERS
           FILES
           GPSProvider.h
           GPSReceiver.h
           GPSReceiverConnectionPolicy.h
           GPSReceiverConnectionTarget.h
)
# GPSReceiver.h declares its serial API only in builds with serial links.
if(QGC_NO_SERIAL_LINK)
    target_compile_definitions(QGCGPSReceiver PUBLIC QGC_NO_SERIAL_LINK)
endif()

if(TARGET ${CMAKE_PROJECT_NAME})
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSReceiver)
endif()
