# ============================================================================
# Serial Communication Tests
# Tests for serial port detection, configuration, and the serial link stack
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        AndroidSerialRxQueueTest.cc
        AndroidSerialRxQueueTest.h
        HostSerialPortTest.cc
        HostSerialPortTest.h
        MockSerialPort.cc
        MockSerialPort.h
        NmeaSerialDeviceTest.cc
        NmeaSerialDeviceTest.h
        PortRegistryTest.cc
        PortRegistryTest.h
        QGCSerialPortInfoTest.cc
        QGCSerialPortInfoTest.h
        SerialConfigurationTest.cc
        SerialConfigurationTest.h
        SerialLinkTest.cc
        SerialLinkTest.h
        SerialPortInfoCodecTest.cc
        SerialPortInfoCodecTest.h
        SerialWireContractTest.cc
        SerialWireContractTest.h
        SerialWorkerTest.cc
        SerialWorkerTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# SerialPortInfoCodecTest loads data/PortInfoGolden.json — the cross-language wire-contract fixture also
# consumed by the Java UsbPortInfoPackingTest (android/build.gradle test resources.srcDirs).
# TARGET_DIRECTORY scopes the SOURCE property to the root-defined main target (CMP0118).
set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/SerialPortInfoCodecTest.cc
    TARGET_DIRECTORY ${CMAKE_PROJECT_NAME}
    APPEND PROPERTY COMPILE_DEFINITIONS "SERIAL_TESTDATA_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/data\"")

add_qgc_test(AndroidSerialRxQueueTest LABELS Unit Comms)
add_qgc_test(HostSerialPortTest LABELS Unit Comms)
add_qgc_test(NmeaSerialDeviceTest LABELS Unit Comms)
add_qgc_test(PortRegistryTest LABELS Unit Comms)
add_qgc_test(QGCSerialPortInfoTest LABELS Unit Comms)
add_qgc_test(SerialPortInfoCodecTest LABELS Unit Comms)
add_qgc_test(SerialWireContractTest LABELS Unit Comms)
add_qgc_test(SerialConfigurationTest LABELS Unit Comms RESOURCE_LOCK Settings TempFiles)
add_qgc_test(SerialWorkerTest LABELS Unit Comms)
add_qgc_test(SerialLinkTest LABELS Integration Comms SERIAL)
