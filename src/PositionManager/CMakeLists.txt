# ============================================================================
# Position Manager Module
# Manages GPS/GNSS position sources and simulated position data
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        PositionManager.cpp
        PositionManager.h
        SimulatedPosition.cc
        SimulatedPosition.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# PositionManager QML Module
# ----------------------------------------------------------------------------
qt_add_library(PositionManagerModule STATIC)

qt_add_qml_module(PositionManagerModule
    URI QGroundControl.PositionManager
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        GCSPositionSettings.qml
    NO_PLUGIN
)
