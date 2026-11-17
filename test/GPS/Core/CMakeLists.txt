# ============================================================================
# GPS Core Tests
# Tests of QGCGPSCore, mirroring src/GPS/Core. Their helpers sit in each folder's Support/ and are included from here,
# such as "Protocols/Support/<header>" and "Transport/Support/<header>".
# ============================================================================

add_subdirectory(Transport/Support)
add_subdirectory(Protocols/Support)
target_link_libraries(
    QGCUnitTests
    PRIVATE
        QGCGPSTestTransports
        QGCGPSProtocolTestSupport
)
add_subdirectory(Protocols)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(Transport/Android)
endif()

set(_qgc_gps_core_suites
    GPSReceiverWorkerTest
    Common/GPSProtocolMathTest
    Common/GPSReceiverConfigTest
    NMEA/NMEASatellitesTest
    NMEA/NMEASentenceTest
    RTCM/RTCMConformanceTest
    RTCM/RTCMStreamContentTest
    Transport/GPSCancellationTest
    Transport/GPSPipeTransportTest
    Transport/GPSRecordingDialogueTest
    Transport/GPSRecordingTransportTest
    Transport/GPSStreamTransportTest
    Transport/GPSTransportContractTest
    Transport/TCPGPSTransportTest
    Transport/UDPGPSTransportTest
    Protocols/GPSCaptureDecodeTest
    Protocols/GPSCommandChannelTest
    Protocols/GPSDecodedDataTest
    Protocols/GPSDriverTest
    Protocols/GPSGoldenTranscriptTest
    Protocols/GPSNMEAStreamTest
    Protocols/GPSProtocolFamilyContractTest
    Protocols/GPSProtocolRuntimeTest
    Protocols/GPSReceiverDetectorTest
    Protocols/GPSSatelliteReportTest
    Protocols/GPSStreamDemuxTest
    Protocols/Ashtech/AshtechProtocolTest
    Protocols/Femto/FemtoProtocolTest
    Protocols/Passive/PassiveProtocolTest
    Protocols/Quectel/QuectelProtocolTest
    Protocols/SBF/SBFProtocolTest
    Protocols/UBX/UBXConfigKeyTest
    Protocols/UBX/UBXProtocolTest
    Protocols/Unicore/UnicoreProtocolTest
)
if(NOT QGC_NO_SERIAL_LINK)
    list(APPEND _qgc_gps_core_suites Transport/SerialGPSTransportTest)
endif()
qgc_add_gps_suites(Unit ${_qgc_gps_core_suites})
