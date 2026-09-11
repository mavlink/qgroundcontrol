add_library(QGCGPSTransport STATIC ${CMAKE_CURRENT_SOURCE_DIR}/GPSTransport.cc
                                   ${CMAKE_CURRENT_SOURCE_DIR}/GPSTransport.h
)
add_library(QGC::GPSTransport ALIAS QGCGPSTransport)
set_target_properties(QGCGPSTransport PROPERTIES AUTOMOC ON)
target_compile_features(QGCGPSTransport PUBLIC cxx_std_20)
target_include_directories(QGCGPSTransport PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    QGCGPSTransport
    PUBLIC QGCGPSContracts Qt6::Core
    PRIVATE QGCLoggingCategory
)

add_library(
    QGCGPSReceiverTransports STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSSocketWait.cc ${CMAKE_CURRENT_SOURCE_DIR}/GPSSocketWait.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TcpGPSTransport.cc ${CMAKE_CURRENT_SOURCE_DIR}/TcpGPSTransport.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UdpGPSTransport.cc ${CMAKE_CURRENT_SOURCE_DIR}/UdpGPSTransport.h
)
add_library(QGC::GPSReceiverTransports ALIAS QGCGPSReceiverTransports)
set_target_properties(QGCGPSReceiverTransports PROPERTIES AUTOMOC ON)
target_compile_features(QGCGPSReceiverTransports PUBLIC cxx_std_20)
target_include_directories(QGCGPSReceiverTransports PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    QGCGPSReceiverTransports
    PUBLIC QGCGPSTransport Qt6::Network
    PRIVATE QGCNetworkIO QGCLoggingCategory
)

if(NOT QGC_NO_SERIAL_LINK)
    target_sources(QGCGPSReceiverTransports PRIVATE SerialGPSTransport.cc SerialGPSTransport.h)
    if(ANDROID)
        target_link_libraries(QGCGPSReceiverTransports PRIVATE QGCAndroidSerialPort)
    else()
        target_link_libraries(QGCGPSReceiverTransports PRIVATE Qt6::SerialPort)
    endif()
else()
    target_compile_definitions(QGCGPSReceiverTransports PUBLIC QGC_NO_SERIAL_LINK)
endif()
