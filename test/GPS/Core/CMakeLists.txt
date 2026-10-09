# ============================================================================
# GPS Core Tests
# Tests of QGCGPSCore, mirroring src/GPS/Core. Their helpers sit in each folder's Support/ and are included from here,
# such as "Protocols/Support/<header>" and "Transport/Support/<header>".
# ============================================================================

add_subdirectory(Transport/Support)
add_subdirectory(Protocols/Support)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSTestTransports QGCGPSProtocolTestSupport)
add_subdirectory(Protocols)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(Transport/Android)
endif()

set(_qgc_gps_core_suites
    GPSReceiverWorkerTest
    Common/GPSProtocolMathTest
    Common/GPSReceiverConfigTest
    NMEA/NMEASentenceTest
    RTCM/RTCMConformanceTest
    Transport/GPSCancellationTest
    Transport/GPSRecordingTransportTest
    Transport/GPSStreamTransportTest
    Transport/GPSTransportContractTest
    Transport/TCPGPSTransportTest
    Transport/UDPGPSTransportTest
    Protocols/GPSCommandChannelTest
    Protocols/GPSDecodedDataTest
    Protocols/GPSDriverTest
    Protocols/GPSGoldenTranscriptTest
    Protocols/GPSNMEAStreamTest
    Protocols/GPSProtocolFamilyContractTest
    Protocols/GPSProtocolRuntimeTest
    Protocols/GPSReceiverDetectorTest
    Protocols/GPSStreamDemuxTest
    Protocols/Ashtech/AshtechProtocolTest
    Protocols/Femto/FemtoProtocolTest
    Protocols/Passive/PassiveProtocolTest
    Protocols/Quectel/QuectelProtocolTest
    Protocols/SBF/SBFProtocolTest
    Protocols/UBX/UBXProtocolTest
    Protocols/Unicore/UnicoreProtocolTest
)
if(NOT QGC_NO_SERIAL_LINK)
    list(APPEND _qgc_gps_core_suites Transport/SerialGPSTransportTest)
endif()
qgc_add_gps_suites(Unit ${_qgc_gps_core_suites})
