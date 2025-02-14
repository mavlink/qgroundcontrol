add_subdirectory(ComponentInformation)

find_package(Qt6 REQUIRED COMPONENTS Core Test)

qt_add_library(VehicleTest
    STATIC
        FTPManagerTest.cc
        FTPManagerTest.h
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
)

target_link_libraries(VehicleTest
    PRIVATE
        Qt6::Test
        Comms
        Vehicle
        VehicleComponentsTest
    PUBLIC
        qgcunittest
)

target_include_directories(VehicleTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
