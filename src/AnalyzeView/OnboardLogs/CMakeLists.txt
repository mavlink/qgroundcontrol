# ============================================================================
# Onboard Logs Module
# Onboard log list and download functionality
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        OnboardLogController.cc
        OnboardLogController.h
        OnboardLogEntry.cc
        OnboardLogEntry.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
