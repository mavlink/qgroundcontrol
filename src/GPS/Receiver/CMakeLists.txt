add_library(
    QGCGPSReceiver STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSReceiverTransportFactory.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSReceiverTransportFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSByteStream.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSByteStream.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSProvider.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSProvider.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSReceiverMailbox.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSReceiverMailbox.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSReceiverSession.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSReceiverSession.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSReceiverAttempt.h
)
add_library(QGC::GPSReceiver ALIAS QGCGPSReceiver)
set_target_properties(QGCGPSReceiver PROPERTIES AUTOMOC ON)
target_compile_features(QGCGPSReceiver PUBLIC cxx_std_20)
target_include_directories(QGCGPSReceiver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    QGCGPSReceiver
    PUBLIC QGCGPSDriver QGCGPSCore QGCGPSCorrections QGCGPSRecording QGCGPSReceiverTransports
    PRIVATE QGCLoggingCategory
)

add_library(
    QGCGPSConnections STATIC
    GPSSerialDiscovery.cc
    GPSSerialDiscovery.h
    GPSReceiverAutoConnect.cc
    GPSReceiverAutoConnect.h
    ../NMEA/NMEASourceManager.cc
    ../NMEA/NMEASourceManager.h
    ../NMEA/NMEAConnectionAttempt.cc
    ../NMEA/NMEAConnectionAttempt.h
)
add_library(QGC::GPSConnections ALIAS QGCGPSConnections)
set_target_properties(QGCGPSConnections PROPERTIES AUTOMOC ON)
target_include_directories(QGCGPSConnections PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    QGCGPSConnections
    PUBLIC QGCGPSReceiver QGCGPSNMEA Qt6::QmlIntegration
    PRIVATE QGCLoggingCategory QGCNetworkIO
)
if(NOT QGC_NO_SERIAL_LINK)
    target_sources(QGCGPSConnections PRIVATE GPSReceiverAutoConnectSerial.cc)
    if(ANDROID)
        target_link_libraries(QGCGPSConnections PRIVATE QGCAndroidSerialPort)
    else()
        target_link_libraries(QGCGPSConnections PRIVATE Qt6::SerialPort)
    endif()
endif()
