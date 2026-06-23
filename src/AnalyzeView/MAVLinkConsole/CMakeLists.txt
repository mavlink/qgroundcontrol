# ============================================================================
# MAVLink Console Module
# MAVLink serial console controller
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        MAVLinkConsoleController.cc
        MAVLinkConsoleController.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
