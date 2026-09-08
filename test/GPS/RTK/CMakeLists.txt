# RTK tests.
add_subdirectory(Driver)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSProviderTest.cc GPSProviderTest.h GPSRtkTest.cc GPSRtkTest.h)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE TcpGPSTransportTest.cc TcpGPSTransportTest.h)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE UdpGPSTransportTest.cc UdpGPSTransportTest.h)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(${CMAKE_PROJECT_NAME} PRIVATE RTKPositionSourceTest.cc RTKPositionSourceTest.h)
add_qgc_test(RTKPositionSourceTest LABELS Unit)

add_qgc_test(GPSProviderTest LABELS Unit)
add_qgc_test(TcpGPSTransportTest LABELS Unit)
add_qgc_test(UdpGPSTransportTest LABELS Unit)
add_qgc_test(GPSRtkTest LABELS Unit RESOURCE_LOCK Settings)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE RTKAutoConnectTest.cc RTKAutoConnectTest.h)
add_qgc_test(RTKAutoConnectTest LABELS Unit RESOURCE_LOCK Settings)

if(NOT QGC_NO_SERIAL_LINK)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE SerialGPSTransportTest.cc SerialGPSTransportTest.h)
    add_qgc_test(SerialGPSTransportTest LABELS Unit)
endif()
