find_package(Qt6 REQUIRED COMPONENTS Core Positioning Test)

qt_add_library(qgcunittest
    STATIC
        MultiSignalSpy.cc
        MultiSignalSpy.h
        MultiSignalSpyV2.cc
        MultiSignalSpyV2.h
        UnitTest.cc
        UnitTest.h
)

target_link_libraries(qgcunittest
    PRIVATE
        FactSystem
        MissionManager
        Settings
        Vehicle
        ComponentInformation
        Qt6::Test
    PUBLIC
        Qt6::Core
        Qt6::Positioning
        MAVLink
        MockLink
)

target_include_directories(qgcunittest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
