add_subdirectory(Support)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSTestTransports)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSStreamTransportTest.cc)
add_qgc_test(GPSStreamTransportTest LABELS Unit GPS)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSTransportContractTest.cc)
add_qgc_test(GPSTransportContractTest LABELS Unit GPS)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSRecordingTransportTest.cc MemoryGPSTransport.h)
add_qgc_test(GPSRecordingTransportTest LABELS Unit GPS)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ReplayGPSTransportTest.cc)
add_qgc_test(ReplayGPSTransportTest LABELS Unit GPS)
# The round trip replays the synthetic protocol corpus. Real receiver captures contain a location; never add one.
set_property(
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/ReplayGPSTransportTest.cc" TARGET_DIRECTORY ${CMAKE_PROJECT_NAME}
    APPEND
    PROPERTY COMPILE_DEFINITIONS GPS_PROTOCOL_CORPUS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../Protocols/corpus"
             GPS_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../Protocols/fixtures"
)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE TCPGPSTransportTest.cc TCPGPSTransportTest.h)
add_qgc_test(TCPGPSTransportTest LABELS Unit GPS)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE UDPGPSTransportTest.cc UDPGPSTransportTest.h)
add_qgc_test(UDPGPSTransportTest LABELS Unit GPS)
if(NOT QGC_NO_SERIAL_LINK)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE SerialGPSTransportTest.cc SerialGPSTransportTest.h)
    add_qgc_test(SerialGPSTransportTest LABELS Unit GPS)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(Android)
endif()
