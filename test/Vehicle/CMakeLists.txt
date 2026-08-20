# ============================================================================
# Vehicle Unit Tests
# Tests for vehicle connection, FTP, MAVLink commands, and link management
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        APMAirframeComponentControllerTest.cc
        APMAirframeComponentControllerTest.h
        FTPManagerTest.cc
        FTPManagerTest.h
        FTPControllerTest.cc
        FTPControllerTest.h
        FirmwareUpgradeControllerTest.cc
        FirmwareUpgradeControllerTest.h
        GuidedModeROITest.cc
        GuidedModeROITest.h
        InitialConnectTest.cc
        InitialConnectTest.h
        InitialConnectPeripheralStartupTest.cc
        InitialConnectPeripheralStartupTest.h
        MAVLinkLogManagerTest.cc
        MAVLinkLogManagerTest.h
        MavCommandQueueReentrancyTest.cc
        MavCommandQueueReentrancyTest.h
        MultiVehicleManagerTest.cc
        MultiVehicleManagerTest.h
        PX4ExternalFlightModesTest.cc
        PX4ExternalFlightModesTest.h
        RemoteIDManagerTest.cc
        RemoteIDManagerTest.h
        RequestMessageTest.cc
        RequestMessageTest.h
        SendMavCommandWithHandlerTest.cc
        SendMavCommandWithHandlerTest.h
        SendMavCommandWithSignallingTest.cc
        SendMavCommandWithSignallingTest.h
        SetEstimatorOriginTest.cc
        SetEstimatorOriginTest.h
        TrajectoryPointsTest.cc
        TrajectoryPointsTest.h
        VehicleDistanceSensorFactGroupTest.cc
        VehicleDistanceSensorFactGroupTest.h
        VehicleLinkManagerTest.cc
        VehicleLinkManagerTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# Component Information Tests
# ----------------------------------------------------------------------------
add_subdirectory(ComponentInformation)

add_qgc_test(APMAirframeComponentControllerTest LABELS Integration Vehicle)
add_qgc_test(FTPControllerTest LABELS Integration Vehicle RESOURCE_LOCK TempFiles)
add_qgc_test(FTPManagerTest LABELS Integration Vehicle)
add_qgc_test(FirmwareUpgradeControllerTest LABELS Unit Vehicle)
add_qgc_test(GuidedModeROITest LABELS Integration Vehicle)
add_qgc_test(GuidedModeROIAPMTest LABELS Integration Vehicle)
add_qgc_test(InitialConnectTest LABELS Integration Vehicle)
add_qgc_test(InitialConnectPeripheralStartupTest LABELS Integration Vehicle)
add_qgc_test(MAVLinkLogManagerTest LABELS Integration Vehicle)
add_qgc_test(MavCommandQueueReentrancyTest LABELS Integration Vehicle)
add_qgc_test(MultiVehicleManagerTest LABELS Integration Vehicle)
add_qgc_test(PX4ExternalFlightModesTest LABELS Unit Vehicle)
add_qgc_test(RemoteIDManagerTest LABELS Integration Vehicle)
add_qgc_test(RequestMessageTest LABELS Integration Vehicle TIMEOUT ${QGC_TEST_TIMEOUT_EXTENDED})
add_qgc_test(SendMavCommandWithHandlerTest LABELS Integration Vehicle)
add_qgc_test(SendMavCommandWithSignallingTest LABELS Integration Vehicle)
add_qgc_test(SetEstimatorOriginTest LABELS Integration Vehicle)
add_qgc_test(TrajectoryPointsTest LABELS Unit Vehicle)
add_qgc_test(VehicleDistanceSensorFactGroupTest LABELS Integration Vehicle)
add_qgc_test(VehicleLinkManagerTest LABELS Integration Vehicle SERIAL)
