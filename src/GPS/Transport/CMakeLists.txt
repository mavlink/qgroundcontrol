add_library(QGCGPSTransport STATIC GPSTransport.cc)
add_library(QGC::GPSTransport ALIAS QGCGPSTransport)
target_compile_features(QGCGPSTransport PUBLIC cxx_std_20)
target_link_libraries(
    QGCGPSTransport
    PUBLIC Qt6::Core
    PRIVATE QGCLoggingCategory Qt6::Network
)
target_sources(QGCGPSTransport PUBLIC FILE_SET HEADERS FILES GPSTransport.h)

if(NOT DEFINED QGC_GPS_COMPONENTS_ENABLED OR "ReceiverTransports" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
    add_library(QGCGPSReceiverTransports STATIC TCPGPSTransport.cc UDPGPSTransport.cc)
    add_library(QGC::GPSReceiverTransports ALIAS QGCGPSReceiverTransports)
    target_compile_features(QGCGPSReceiverTransports PUBLIC cxx_std_20)
    target_link_libraries(
        QGCGPSReceiverTransports
        PUBLIC QGCGPSTransport Qt6::Network
        PRIVATE QGCLoggingCategory
    )
    target_sources(QGCGPSReceiverTransports PUBLIC FILE_SET HEADERS FILES TCPGPSTransport.h UDPGPSTransport.h)
    if(NOT QGC_NO_SERIAL_LINK)
        target_sources(QGCGPSReceiverTransports PRIVATE SerialGPSTransport.cc)
        target_sources(QGCGPSReceiverTransports PUBLIC FILE_SET HEADERS FILES SerialGPSTransport.h)
        if(ANDROID)
            target_link_libraries(QGCGPSReceiverTransports PRIVATE QGCAndroidSerialPort)
        else()
            target_link_libraries(QGCGPSReceiverTransports PRIVATE Qt6::SerialPort)
        endif()
    else()
        target_compile_definitions(QGCGPSReceiverTransports PUBLIC QGC_NO_SERIAL_LINK)
    endif()
endif()
