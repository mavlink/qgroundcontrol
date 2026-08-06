# ============================================================================
# GPS Unit Tests
# Tests for GPS/GNSS positioning and RTK corrections
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        GpsTestHelpers.h
        MockNTRIPTransport.h
        NTRIPManagerTest.cc
        NTRIPManagerTest.h
        NTRIPSourceTableTest.cc
        NTRIPSourceTableTest.h
        NTRIPHttpTransportTest.cc
        NTRIPHttpTransportTest.h
        NTRIPConnectionStatsTest.cc
        NTRIPConnectionStatsTest.h
        NTRIPGgaProviderTest.cc
        NTRIPGgaProviderTest.h
        NTRIPSourceTableControllerTest.cc
        NTRIPSourceTableControllerTest.h
        UdpForwarderTest.cc
        UdpForwarderTest.h
        RTCMParserTest.cc
        RTCMParserTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(RTCMParserTest LABELS Unit)
add_qgc_test(NTRIPManagerTest LABELS Unit)
add_qgc_test(NTRIPHttpTransportTest LABELS Unit)
add_qgc_test(NTRIPSourceTableTest LABELS Unit)
add_qgc_test(NTRIPConnectionStatsTest LABELS Unit)
add_qgc_test(NTRIPGgaProviderTest LABELS Unit)
add_qgc_test(NTRIPSourceTableControllerTest LABELS Unit)
add_qgc_test(UdpForwarderTest LABELS Unit)

# GPSDriver facade test exercises the px4 bridge — only built when the Driver layer is.
if(NOT QGC_NO_SERIAL_LINK)
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            GPSDriverTest.cc
            GPSDriverTest.h
            GPSRtkTest.cc
            GPSRtkTest.h
            SerialGPSTransportTest.cc
            SerialGPSTransportTest.h
    )
    add_qgc_test(GPSDriverTest LABELS Unit)
    add_qgc_test(GPSRtkTest LABELS Unit)
    add_qgc_test(SerialGPSTransportTest LABELS Unit)
endif()
