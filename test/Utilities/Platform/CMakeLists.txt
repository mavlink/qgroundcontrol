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

# QGCKeychain tests always run — QSettings fallback is used when qtkeychain
# is unavailable (Linux), so tests exercise whichever backend is active.
target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        QGCKeychainTest.cc
        QGCKeychainTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
