find_package(Qt6 REQUIRED COMPONENTS Core Positioning Test Widgets)

qt_add_library(AutoPilotPluginsTest
    STATIC
        RadioConfigTest.cc
        RadioConfigTest.h
)

target_link_libraries(AutoPilotPluginsTest
    PRIVATE
        Qt6::Test
        APMAutoPilotPlugin
        AutoPilotPlugins
        CommonAutoPilotPlugin
        PX4AutoPilotPlugin
        Vehicle
    PUBLIC
        Qt6::Core
        qgcunittest
)

target_include_directories(AutoPilotPluginsTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
