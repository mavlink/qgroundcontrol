find_package(Qt6 REQUIRED COMPONENTS Core Qml)

qt_add_library(FactControls STATIC
    FactPanelController.cc
    FactPanelController.h
)

target_link_libraries(FactControls
    PRIVATE
        Qt6::Qml
        AutoPilotPlugins
        FactSystem
        QGC
        Utilities
        Vehicle
    PUBLIC
        Qt6::Core
)

target_include_directories(FactControls INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
