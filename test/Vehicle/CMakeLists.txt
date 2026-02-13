# ============================================================================
# Vehicle Unit Tests
# Tests for vehicle connection, FTP, MAVLink commands, and link management
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        FTPManagerTest.cc
        FTPManagerTest.h
        InitialConnectTest.cc
        InitialConnectTest.h
        MAVLinkLogManagerTest.cc
        MAVLinkLogManagerTest.h
        PX4ManifestTest.cc
        PX4ManifestTest.h
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
# PX4 Manifest Test Data
# ----------------------------------------------------------------------------
qt_add_resources(${CMAKE_PROJECT_NAME} "VehicleTest_PX4Manifest_res"
    PREFIX "/Vehicle"
    FILES
        px4_manifest_v2.json
)

# ----------------------------------------------------------------------------
# Component Information Tests
# ----------------------------------------------------------------------------
add_subdirectory(ComponentInformation)
