# ============================================================================
# MAVLink Unit Tests
# Tests for MAVLink protocol, status text handling, and message signing
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        HealthAndArmingCheckReportTest.cc
        HealthAndArmingCheckReportTest.h
        ImageProtocolManagerTest.cc
        ImageProtocolManagerTest.h
        MAVLinkStreamConfigTest.cc
        MAVLinkStreamConfigTest.h
        QGCMAVLinkTest.cc
        QGCMAVLinkTest.h
        StatusTextHandlerTest.cc
        StatusTextHandlerTest.h
        SysStatusSensorInfoTest.cc
        SysStatusSensorInfoTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(Signing)

add_qgc_test(HealthAndArmingCheckReportTest LABELS Unit MAVLink)
add_qgc_test(ImageProtocolManagerTest LABELS Unit MAVLink)
add_qgc_test(MAVLinkStreamConfigTest LABELS Unit MAVLink)
add_qgc_test(QGCMAVLinkTest LABELS Unit MAVLink)
add_qgc_test(StatusTextHandlerTest LABELS Unit MAVLink)
add_qgc_test(SysStatusSensorInfoTest LABELS Unit MAVLink)
