message(STATUS "ATLAS: personalizacao carregada")

# ============================================================================
# BRANDING
# ============================================================================

set(ATLAS_BRAND_RESOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/res/atlas_logo_white.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/icons/map.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/icons/telemetry.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/icons/attitude.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/icons/gimbal.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/icons/events.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/icons/system.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/icons/gimbal_control.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/icons/zoom_plus.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/icons/zoom_minus.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/icons/recenter.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/icons/photo.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/icons/fullscreen.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/icons/video.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/icons/ai.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/icons/settings.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/icons/tracking.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/icons/track_click.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/icons/roi.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/icons/auto.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/icons/clear.svg"
)

set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/res/atlas_logo_white.svg"
    PROPERTIES
    QT_RESOURCE_ALIAS "atlas_logo_white.svg"
)

set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/res/icons/map.svg" PROPERTIES QT_RESOURCE_ALIAS "icons/map.svg")
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/res/icons/telemetry.svg" PROPERTIES QT_RESOURCE_ALIAS "icons/telemetry.svg")
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/res/icons/attitude.svg" PROPERTIES QT_RESOURCE_ALIAS "icons/attitude.svg")
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/res/icons/gimbal.svg" PROPERTIES QT_RESOURCE_ALIAS "icons/gimbal.svg")
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/res/icons/events.svg" PROPERTIES QT_RESOURCE_ALIAS "icons/events.svg")
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/res/icons/system.svg" PROPERTIES QT_RESOURCE_ALIAS "icons/system.svg")
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/res/icons/gimbal_control.svg" PROPERTIES QT_RESOURCE_ALIAS "icons/gimbal_control.svg")
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/res/icons/zoom_plus.svg" PROPERTIES QT_RESOURCE_ALIAS "icons/zoom_plus.svg")
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/res/icons/zoom_minus.svg" PROPERTIES QT_RESOURCE_ALIAS "icons/zoom_minus.svg")
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/res/icons/recenter.svg" PROPERTIES QT_RESOURCE_ALIAS "icons/recenter.svg")
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/res/icons/photo.svg" PROPERTIES QT_RESOURCE_ALIAS "icons/photo.svg")
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/res/icons/fullscreen.svg" PROPERTIES QT_RESOURCE_ALIAS "icons/fullscreen.svg")
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/res/icons/video.svg" PROPERTIES QT_RESOURCE_ALIAS "icons/video.svg")
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/res/icons/ai.svg" PROPERTIES QT_RESOURCE_ALIAS "icons/ai.svg")
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/res/icons/settings.svg" PROPERTIES QT_RESOURCE_ALIAS "icons/settings.svg")
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/res/icons/tracking.svg" PROPERTIES QT_RESOURCE_ALIAS "icons/tracking.svg")
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/res/icons/track_click.svg" PROPERTIES QT_RESOURCE_ALIAS "icons/track_click.svg")
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/res/icons/roi.svg" PROPERTIES QT_RESOURCE_ALIAS "icons/roi.svg")
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/res/icons/auto.svg" PROPERTIES QT_RESOURCE_ALIAS "icons/auto.svg")
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/res/icons/clear.svg" PROPERTIES QT_RESOURCE_ALIAS "icons/clear.svg")

qt_add_resources(
    ${CMAKE_PROJECT_NAME}
    atlas_brand
    PREFIX "/atlas"
    FILES ${ATLAS_BRAND_RESOURCES}
)

# ============================================================================
# QML OVERRIDES
#
# Mantemos toda a UI ATLAS dentro de custom/atlas. O interceptor do
# AtlasCustomPlugin redireciona qrc:/qml/... para /Custom/qml/... quando existe
# um override correspondente.
# ============================================================================

set(ATLAS_QML_OVERRIDES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FlyView.qml"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FlyViewCustomLayer.qml"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/CustomMapItems.qml"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FlyViewToolBar.qml"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FlyViewToolBarIndicators.qml"
)

set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FlyView.qml"
    PROPERTIES
    QT_RESOURCE_ALIAS "QGroundControl/FlyView/FlyView.qml"
)

set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FlyViewCustomLayer.qml"
    PROPERTIES
    QT_RESOURCE_ALIAS "QGroundControl/FlyView/FlyViewCustomLayer.qml"
)

set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/CustomMapItems.qml"
    PROPERTIES
    QT_RESOURCE_ALIAS "QGroundControl/FlightMap/MapItems/CustomMapItems.qml"
)

set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FlyViewToolBar.qml"
    PROPERTIES
    QT_RESOURCE_ALIAS "QGroundControl/Toolbar/FlyViewToolBar.qml"
)

set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FlyViewToolBarIndicators.qml"
    PROPERTIES
    QT_RESOURCE_ALIAS "QGroundControl/Toolbar/FlyViewToolBarIndicators.qml"
)

qt_add_resources(
    ${CMAKE_PROJECT_NAME}
    atlas_qml_overrides
    PREFIX "/Custom/qml"
    FILES ${ATLAS_QML_OVERRIDES}
)

# ============================================================================
# ATLAS C++ SOURCES
# ============================================================================

set(CUSTOM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AtlasCustomPlugin.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AtlasCustomPlugin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AtlasJetsonClient.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AtlasJetsonClient.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AtlasSiyiClient.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AtlasSiyiClient.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AtlasPayloadRouter.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AtlasPayloadRouter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AtlasScreenRecorder.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AtlasScreenRecorder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AtlasKmlManager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AtlasKmlManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AtlasKmlWallGeometry.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AtlasKmlWallGeometry.h
    PARENT_SCOPE
)

set(CUSTOM_INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    PARENT_SCOPE
)

# ============================================================================
# QGC CUSTOM PLUGIN
# ============================================================================

set(CUSTOM_DEFINITIONS
    QGC_CUSTOM_BUILD
    CUSTOMHEADER="AtlasCustomPlugin.h"
    CUSTOMCLASS=AtlasCustomPlugin
    PARENT_SCOPE
)
