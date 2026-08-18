# ============================================================================
# MAVLink Bandwidth Module
# End-to-end MAVLink bandwidth test controller
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        MAVLinkBandwidthController.cc
        MAVLinkBandwidthController.h
        MAVLinkFtpBandwidthTest.cc
        MAVLinkFtpBandwidthTest.h
        MAVLinkBandwidthProtocol.cc
        MAVLinkBandwidthProtocol.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set(_mavlink_bandwidth_script "${CMAKE_SOURCE_DIR}/tools/mavlink-bandwidth/APM/scripts/mavlink_bandwidth.lua")
set_source_files_properties("${_mavlink_bandwidth_script}" PROPERTIES QT_RESOURCE_ALIAS "mavlink_bandwidth.lua")
qt_add_resources(${CMAKE_PROJECT_NAME} mavlink_bandwidth_script
    PREFIX "/mavlink-bandwidth"
    FILES "${_mavlink_bandwidth_script}"
)
