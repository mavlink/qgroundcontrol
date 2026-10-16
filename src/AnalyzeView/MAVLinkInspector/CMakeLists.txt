# ============================================================================
# MAVLink Inspector Module
# MAVLink inspector controller and chart integration
# ============================================================================

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        MAVLinkChartController.cc
        MAVLinkChartController.h
        MAVLinkInspectorController.cc
        MAVLinkInspectorController.h
        MAVLinkMessage.cc
        MAVLinkMessage.h
        MAVLinkMessageField.cc
        MAVLinkMessageField.h
        MAVLinkSystem.cc
        MAVLinkSystem.h
)

# Shares the file-local kMinDelta with MAVLinkChartController.cc, so it cannot share a unity unit.
set_source_files_properties(
    MAVLinkMessageField.cc
    TARGET_DIRECTORY
    ${CMAKE_PROJECT_NAME}
    PROPERTIES
        SKIP_UNITY_BUILD_INCLUSION ON
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
