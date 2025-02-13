find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(Gimbal STATIC
    Gimbal.cc
    Gimbal.h
    GimbalController.cc
    GimbalController.h
)

target_link_libraries(Gimbal
    PRIVATE
        Comms
        Settings
        Utilities
        Vehicle
    PUBLIC
        Qt6::Core
        FactSystem
        MAVLink
        QmlControls
)

target_include_directories(Gimbal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
