# ============================================================================
# Platform Subsystem
# Platform abstractions, single-instance guard, and signal handling
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        Platform.cc
        Platform.h
)

# Signal handling and single-instance guard not available on mobile platforms
if(NOT ANDROID AND NOT IOS)
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            RunGuard.cc
            RunGuard.h
            SignalHandler.cc
            SignalHandler.h
    )
endif()

# iOS-specific screen management
if(IOS)
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            MobileScreenMgr.mm
            MobileScreenMgr.h
    )
endif()

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
