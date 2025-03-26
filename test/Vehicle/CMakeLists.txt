find_package(Qt6 REQUIRED COMPONENTS Core Test)

target_sources(qgctest
    PRIVATE
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

target_link_libraries(qgctest PRIVATE Qt6::Test)

target_include_directories(qgctest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(ComponentInformation)
