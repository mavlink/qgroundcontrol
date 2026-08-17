# ============================================================================
# MAVLink Bandwidth Module
# End-to-end MAVLink bandwidth test controller
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        MAVLinkBandwidthController.cc
        MAVLinkBandwidthController.h
        MAVLinkBandwidthProtocol.cc
        MAVLinkBandwidthProtocol.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
