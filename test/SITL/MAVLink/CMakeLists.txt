# ============================================================================
# SITL MAVLink Protocol Tests
# ============================================================================
# Tests that validate QGC's MAVLink protocol implementation against a real
# autopilot. These are flight-stack-agnostic — they run against PX4 SITL
# but assert MAVLink-level behavior that any conformant autopilot should exhibit.

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        tst_MAVLinkHeartbeat.h
        tst_MAVLinkHeartbeat.cc
        tst_MAVLinkParamSync.h
        tst_MAVLinkParamSync.cc
        tst_MAVLinkMission.h
        tst_MAVLinkMission.cc
        tst_MAVLinkCommand.h
        tst_MAVLinkCommand.cc
        tst_MAVLinkStandardModes.h
        tst_MAVLinkStandardModes.cc
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_resources(${CMAKE_PROJECT_NAME} "sitl_mavlink_test_resources"
    PREFIX "/test/SITL/MAVLink"
    FILES
        resources/simple_square.plan
)

add_qgc_test(SITLHeartbeatTest      LABELS SITL MAVLinkProtocol RESOURCE_LOCK SITLContainer)
add_qgc_test(SITLParamSyncTest      LABELS SITL MAVLinkProtocol RESOURCE_LOCK SITLContainer)
add_qgc_test(SITLMissionTest        LABELS SITL MAVLinkProtocol RESOURCE_LOCK SITLContainer)
add_qgc_test(SITLCommandTest        LABELS SITL MAVLinkProtocol RESOURCE_LOCK SITLContainer)
add_qgc_test(SITLStandardModesTest  LABELS SITL MAVLinkProtocol RESOURCE_LOCK SITLContainer)
