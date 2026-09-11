target_sources(${CMAKE_PROJECT_NAME} PRIVATE TcpGPSTransportTest.cc TcpGPSTransportTest.h)
add_qgc_test(TcpGPSTransportTest LABELS Unit)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE UdpGPSTransportTest.cc UdpGPSTransportTest.h)
add_qgc_test(UdpGPSTransportTest LABELS Unit)

if(NOT QGC_NO_SERIAL_LINK)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE SerialGPSTransportTest.cc SerialGPSTransportTest.h)
    add_qgc_test(SerialGPSTransportTest LABELS Unit)
endif()
