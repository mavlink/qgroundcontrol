find_package(Qt6 REQUIRED COMPONENTS Core Qml Positioning QmlIntegration)

target_sources(QGC
    PRIVATE
        PositionManager.cpp
        PositionManager.h
        SimulatedPosition.cc
        SimulatedPosition.h
)

target_link_libraries(QGC
    PRIVATE
        Qt6::Qml
        Qt6::PositioningPrivate
    PUBLIC
        Qt6::Core
        Qt6::Positioning
        Qt6::QmlIntegration
)

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
