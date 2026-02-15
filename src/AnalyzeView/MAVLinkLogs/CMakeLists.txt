# ============================================================================
# MAVLink Logs Module
# MAVLink log list and download functionality
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        MAVLinkLogController.cc
        MAVLinkLogController.h
        MAVLinkLogEntry.cc
        MAVLinkLogEntry.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
