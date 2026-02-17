# ============================================================================
# DataFlash Utility
# General-purpose ArduPilot DataFlash binary log parsing utilities
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        DataFlashUtility.cc
        DataFlashUtility.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
