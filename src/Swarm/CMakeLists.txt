# ============================================================================
# Swarm Manager Module
# ============================================================================

qt_add_library(SwarmModule STATIC)

# ----------------------------------------------------------------------------
# Swarm QML Module
# ----------------------------------------------------------------------------
qt_add_qml_module(SwarmModule
    URI Swarm
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        QmlControls/SwarmInterface.qml
        QmlControls/SwarmDashboard.qml
        QmlControls/SwarmFormationSelector.qml
        QmlControls/SwarmVehicleStatus.qml
        QmlControls/SwarmControlPanel.qml
        QmlControls/SwarmTelemetryWidget.qml
        QmlControls/SwarmAlertSystem.qml
        QmlControls/SwarmMiniMap.qml
    NO_PLUGIN
)

# ----------------------------------------------------------------------------
# Sources
# ----------------------------------------------------------------------------
target_sources(SwarmModule
    PRIVATE
        SwarmManager.cc
        SwarmManager.h
)

# ----------------------------------------------------------------------------
# Include Directories
# ----------------------------------------------------------------------------
target_include_directories(SwarmModule
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)

# ----------------------------------------------------------------------------
# Dependencies
# ----------------------------------------------------------------------------
target_link_libraries(SwarmModule
    PRIVATE
        QGC
        Vehicle
)