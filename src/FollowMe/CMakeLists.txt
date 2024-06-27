find_package(Qt6 REQUIRED COMPONENTS Core Positioning)

qt_add_library(FollowMe STATIC
    FollowMe.cc
    FollowMe.h
)

target_link_libraries(FollowMe
    PRIVATE
        Qt6::Positioning
        FirmwarePlugin
        PositionManager
        Settings
        Utilities
        Vehicle
    PUBLIC
        Qt6::Core
        QGC
)

target_include_directories(FollowMe PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
