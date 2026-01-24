# ============================================================================
# Communications Unit Tests
# Tests for serial port detection and communication links
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        LinkConfigurationTest.cc
        LinkConfigurationTest.h
        MockConfigurationTest.cc
        MockConfigurationTest.h
        MockLinkTest.cc
        MockLinkTest.h
        QGCSerialPortInfoTest.cc
        QGCSerialPortInfoTest.h
        TCPConfigurationTest.cc
        TCPConfigurationTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
