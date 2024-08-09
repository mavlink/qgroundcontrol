find_package(Qt6 REQUIRED COMPONENTS Core Qml)

qt_add_library(TerminateButton STATIC
    TerminateButton.cc
    TerminateButton.h
)


target_link_libraries(TerminateButton
    PRIVATE
        Qt6::Qml
        FirmwarePlugin
        Settings
        Utilities
        Vehicle
    PUBLIC
        Qt6::Core
        Comms
        QGC
        QmlControls
)

target_include_directories(TerminateButton PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
