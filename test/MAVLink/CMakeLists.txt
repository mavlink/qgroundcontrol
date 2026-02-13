# ============================================================================
# MAVLink Unit Tests
# Tests for MAVLink protocol, status text handling, and message signing
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        MAVLinkBenchmark.cc
        MAVLinkBenchmark.h
        SigningTest.cc
        SigningTest.h
        StatusTextHandlerTest.cc
        StatusTextHandlerTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
