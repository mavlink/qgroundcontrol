# ============================================================================
# Bluetooth Tests
# Hardware-backed and unit tests for Bluetooth configuration/workers
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        BluetoothConfigurationTest.cc
        BluetoothConfigurationTest.h
        BluetoothEmulatedAdapterTest.cc
        BluetoothEmulatedAdapterTest.h
        BluetoothLiveAdapterTest.cc
        BluetoothLiveAdapterTest.h
        BluetoothWorkerTest.cc
        BluetoothWorkerTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
