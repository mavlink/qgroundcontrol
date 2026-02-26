# ============================================================================
# Vehicle Unit Tests
# Tests for vehicle connection, FTP, MAVLink commands, and link management
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        FTPManagerTest.cc
        FTPManagerTest.h
        FTPControllerTest.cc
        FTPControllerTest.h
        FirmwareUpgradeControllerTest.cc
        FirmwareUpgradeControllerTest.h
        InitialConnectTest.cc
        InitialConnectTest.h
        InitialConnectPeripheralStartupTest.cc
        InitialConnectPeripheralStartupTest.h
        MAVLinkLogManagerTest.cc
        MAVLinkLogManagerTest.h
        APMAirframeComponentControllerTest.cc
        APMAirframeComponentControllerTest.h
        RequestMessageTest.cc
        RequestMessageTest.h
        SendMavCommandWithHandlerTest.cc
        SendMavCommandWithHandlerTest.h
        SendMavCommandWithSignallingTest.cc
        SendMavCommandWithSignallingTest.h
        VehicleLinkManagerTest.cc
        VehicleLinkManagerTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# Component Information Tests
# ----------------------------------------------------------------------------
add_subdirectory(ComponentInformation)
