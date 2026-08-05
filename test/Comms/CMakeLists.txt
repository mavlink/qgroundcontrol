# ============================================================================
# Communications Unit Tests
# Tests for serial port detection and communication links
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        LinkConfigurationTest.cc
        LinkConfigurationTest.h
        LinkManagerTest.cc
        LinkManagerTest.h
        LogReplayLinkControllerTest.cc
        LogReplayLinkControllerTest.h
        LogReplayLinkTest.cc
        LogReplayLinkTest.h
        MAVLinkV1TrafficTest.cc
        MAVLinkV1TrafficTest.h
        QGCSerialPortInfoTest.cc
        QGCSerialPortInfoTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(Bluetooth)

add_qgc_test(LinkConfigurationTest LABELS Unit Comms RESOURCE_LOCK Settings TempFiles)
add_qgc_test(LinkManagerTest LABELS Integration Comms SERIAL)
add_qgc_test(LogReplayLinkControllerTest LABELS Unit Comms)
add_qgc_test(LogReplayLinkTest LABELS Unit Comms)
add_qgc_test(MAVLinkV1TrafficTest LABELS Integration Comms)
add_qgc_test(QGCSerialPortInfoTest LABELS Unit Comms)
