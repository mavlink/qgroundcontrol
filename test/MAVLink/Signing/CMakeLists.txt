# ============================================================================
# MAVLink Signing Unit Tests
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        MockLinkSigningTest.cc
        MockLinkSigningTest.h
        SigningControllerTest.cc
        SigningControllerTest.h
        SigningStatusTest.cc
        SigningStatusTest.h
        SigningTest.cc
        SigningTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
