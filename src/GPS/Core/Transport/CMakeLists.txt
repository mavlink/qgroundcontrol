# ============================================================================
# GPS Core Transport
# The transport interface the driver reads and writes, the serial, TCP and UDP receiver transports, the pipe to a
# link another thread's event loop serves, with its transport and the base of that link's owner, and the transport
# that records a session.
# ============================================================================

target_sources(
    QGCGPSCore
    PRIVATE
        GPSCancellation.cc
        GPSCancellation.h
        GPSDeviceTransport.cc
        GPSDeviceTransport.h
        GPSPipe.cc
        GPSPipe.h
        GPSPipeLink.cc
        GPSPipeLink.h
        GPSPipeTransport.cc
        GPSPipeTransport.h
        GPSRecordingTransport.cc
        GPSRecordingTransport.h
        GPSTransport.cc
        GPSTransport.h
        TCPGPSTransport.cc
        TCPGPSTransport.h
        UDPGPSTransport.cc
        UDPGPSTransport.h
)
target_include_directories(QGCGPSCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(QGC_NO_SERIAL_LINK)
    target_compile_definitions(QGCGPSCore PUBLIC QGC_NO_SERIAL_LINK)
else()
    target_sources(
        QGCGPSCore
        PRIVATE
            SerialGPSTransport.cc
            SerialGPSTransport.h
    )
    if(ANDROID)
        target_link_libraries(QGCGPSCore PRIVATE QGCAndroidSerialPort)
    else()
        target_link_libraries(QGCGPSCore PRIVATE Qt6::SerialPort)
    endif()
endif()
