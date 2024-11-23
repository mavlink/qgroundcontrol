find_package(Qt6 REQUIRED COMPONENTS Core Qml Positioning QmlIntegration)

qt_add_library(PositionManager STATIC
    PositionManager.cpp
    PositionManager.h
    SimulatedPosition.cc
    SimulatedPosition.h
)

target_link_libraries(PositionManager
    PRIVATE
        Qt6::Qml
        Qt6::PositioningPrivate
        API
        QGC
        Utilities
        Vehicle
    PUBLIC
        Qt6::Core
        Qt6::Positioning
        Qt6::QmlIntegration
)

target_include_directories(PositionManager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
