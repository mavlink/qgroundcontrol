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
