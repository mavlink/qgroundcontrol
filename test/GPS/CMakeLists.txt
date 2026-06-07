# ============================================================================
# GPS Unit Tests
# Tests for GPS/GNSS positioning and RTK corrections
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        GpsTestHelpers.h
        NTRIPManagerTest.cc
        NTRIPManagerTest.h
        NTRIPSourceTableTest.cc
        NTRIPSourceTableTest.h
        NTRIPHttpTransportTest.cc
        NTRIPHttpTransportTest.h
        RTCMParserTest.cc
        RTCMParserTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(RTCMParserTest LABELS Unit)
add_qgc_test(NTRIPManagerTest LABELS Unit)
add_qgc_test(NTRIPHttpTransportTest LABELS Unit)
add_qgc_test(NTRIPSourceTableTest LABELS Unit)
