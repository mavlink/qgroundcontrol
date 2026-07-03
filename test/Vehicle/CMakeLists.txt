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
        RequestMessageTest.cc
        RequestMessageTest.h
        SendMavCommandWithHandlerTest.cc
        SendMavCommandWithHandlerTest.h
        SendMavCommandWithSignallingTest.cc
        SendMavCommandWithSignallingTest.h
        SetEstimatorOriginTest.cc
        SetEstimatorOriginTest.h
        VehicleLinkManagerTest.cc
        VehicleLinkManagerTest.h
)

if(NOT QGC_DISABLE_APM_PLUGIN)
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            APMAirframeComponentControllerTest.cc
            APMAirframeComponentControllerTest.h
    )
endif()

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# Component Information Tests
# ----------------------------------------------------------------------------
add_subdirectory(ComponentInformation)

add_qgc_test(APMAirframeComponentControllerTest LABELS Integration Vehicle)
add_qgc_test(FTPControllerTest LABELS Integration Vehicle RESOURCE_LOCK TempFiles)
add_qgc_test(FTPManagerTest LABELS Integration Vehicle)
add_qgc_test(FirmwareUpgradeControllerTest LABELS Unit Vehicle)
add_qgc_test(InitialConnectTest LABELS Integration Vehicle)
add_qgc_test(InitialConnectPeripheralStartupTest LABELS Integration Vehicle)
add_qgc_test(MAVLinkLogManagerTest LABELS Integration Vehicle)
add_qgc_test(RequestMessageTest LABELS Integration Vehicle TIMEOUT ${QGC_TEST_TIMEOUT_EXTENDED})
add_qgc_test(SendMavCommandWithHandlerTest LABELS Integration Vehicle)
add_qgc_test(SendMavCommandWithSignallingTest LABELS Integration Vehicle)
add_qgc_test(SetEstimatorOriginTest LABELS Integration Vehicle)
add_qgc_test(VehicleLinkManagerTest LABELS Integration Vehicle SERIAL)
