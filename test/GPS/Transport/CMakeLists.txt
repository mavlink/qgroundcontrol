target_sources(${CMAKE_PROJECT_NAME} PRIVATE TCPGPSTransportTest.cc TCPGPSTransportTest.h)
add_qgc_test(TCPGPSTransportTest LABELS Unit)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE UDPGPSTransportTest.cc UDPGPSTransportTest.h)
add_qgc_test(UDPGPSTransportTest LABELS Unit)

if(NOT QGC_NO_SERIAL_LINK)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE SerialGPSTransportTest.cc SerialGPSTransportTest.h)
    add_qgc_test(SerialGPSTransportTest LABELS Unit)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_subdirectory(Android)
endif()
