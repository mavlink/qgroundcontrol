# ============================================================================
# Platform Utilities Unit Tests
# Tests for platform safety checks and environment setup
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        PlatformTest.cc
        PlatformTest.h
)

# RunGuard and SignalHandler only available on desktop platforms
if(NOT ANDROID AND NOT IOS)
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            RunGuardTest.cc
            RunGuardTest.h
            SignalHandlerTest.cc
            SignalHandlerTest.h
    )
endif()

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT ANDROID AND NOT IOS)
    add_qgc_test(PlatformTest LABELS Unit Utilities)
    add_qgc_test(RunGuardTest LABELS Unit Utilities)
    add_qgc_test(SignalHandlerTest LABELS Unit Utilities)
endif()
