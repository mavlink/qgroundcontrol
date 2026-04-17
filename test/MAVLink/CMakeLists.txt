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
        MockLinkSigningTest.cc
        MockLinkSigningTest.h
        QGCMAVLinkTest.cc
        QGCMAVLinkTest.h
        SigningTest.cc
        SigningTest.h
        StatusTextHandlerTest.cc
        StatusTextHandlerTest.h
        SysStatusSensorInfoTest.cc
        SysStatusSensorInfoTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
