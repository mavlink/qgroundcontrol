# ============================================================================
# Vehicle Unit Tests
# Tests for vehicle connection, FTP, MAVLink commands, and link management
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        AutotuneStateMachineTest.cc
        AutotuneStateMachineTest.h
        InitialConnectTest.cc
        InitialConnectTest.h
        MAVLinkLogManagerTest.cc
        MAVLinkLogManagerTest.h
        RequestMessageTest.cc
        RequestMessageTest.h
        SendMavCommandWithHandlerTest.cc
        SendMavCommandWithHandlerTest.h
        SendMavCommandWithSignallingTest.cc
        SendMavCommandWithSignallingTest.h
        VehicleLinkManagerTest.cc
        VehicleLinkManagerTest.h
        VehicleTest.cc
        VehicleTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# Component Information Tests
# ----------------------------------------------------------------------------
add_subdirectory(ComponentInformation)

# ----------------------------------------------------------------------------
# Actuators Tests
# ----------------------------------------------------------------------------
add_subdirectory(Actuators)

# ----------------------------------------------------------------------------
# FTP Tests
# ----------------------------------------------------------------------------
add_subdirectory(FTP)

# ----------------------------------------------------------------------------
# Vehicle Setup Tests
# ----------------------------------------------------------------------------
add_subdirectory(VehicleSetup)
