# ============================================================================
# QGroundControl Custom Plugin Example
# Template for building custom QGroundControl plugins
# ============================================================================

message(STATUS "QGC: Adding Custom Plugin")

# ============================================================================
# Custom Android Package Template
# ============================================================================

if(ANDROID)
    set(CUSTOM_ANDROID_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    if(IS_DIRECTORY "${CUSTOM_ANDROID_DIR}")
        file(GLOB_RECURSE CUSTOM_ANDROID_FILES
            CONFIGURE_DEPENDS
            LIST_DIRECTORIES FALSE
            "${CUSTOM_ANDROID_DIR}/*"
        )
        if(CUSTOM_ANDROID_FILES)
            message(STATUS "QGC: Custom Android package template found. Overlaying custom files...")
            set(DEFAULT_ANDROID_DIR "${CMAKE_SOURCE_DIR}/android")
            set(FINAL_ANDROID_DIR "${CMAKE_BINARY_DIR}/${QGC_CUSTOM_DIR}/android")

            file(COPY "${DEFAULT_ANDROID_DIR}/" DESTINATION "${FINAL_ANDROID_DIR}")
            file(COPY "${CUSTOM_ANDROID_DIR}/" DESTINATION "${FINAL_ANDROID_DIR}")

            set(QGC_ANDROID_PACKAGE_SOURCE_DIR "${FINAL_ANDROID_DIR}")
            set(QGC_ANDROID_PACKAGE_SOURCE_DIR "${FINAL_ANDROID_DIR}" PARENT_SCOPE)
            message(STATUS "QGC: Android package template path will be set to: ${QGC_ANDROID_PACKAGE_SOURCE_DIR}")
        else()
            message(STATUS "QGC: Custom Android package template empty. Using default.")
        endif()
    else()
        message(STATUS "QGC: No custom Android package template found. Using default.")
    endif()
endif()

# ============================================================================
# Custom Resources and Import Paths
# ============================================================================

set(CUSTOM_IMAGE_RESOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/res/Images/altitude.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/Images/attitude_crosshair.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/Images/attitude_dial.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/Images/attitude_pointer.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/Images/chronometer.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/Images/compass_needle.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/Images/compass_pointer.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/Images/distance.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/Images/dronecode-black.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/Images/dronecode-white.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/Images/horizontal_speed.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/Images/microSD.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/Images/odometer.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/Images/vertical_speed.svg"
)
qgc_set_qt_resource_alias(${CUSTOM_IMAGE_RESOURCES})
qt_add_resources(${CMAKE_PROJECT_NAME} custom_images
    PREFIX "/custom/img"
    FILES ${CUSTOM_IMAGE_RESOURCES}
)

set(CUSTOM_VEHICLE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/res/Images/CustomVehicleIcon.svg")
set_source_files_properties("${CUSTOM_VEHICLE_ICON}" PROPERTIES QT_RESOURCE_ALIAS "PaperPlane.svg")
qt_add_resources(${CMAKE_PROJECT_NAME} custom_vehicle_icon
    PREFIX "/Custom/qmlimages"
    FILES "${CUSTOM_VEHICLE_ICON}"
)

set(CUSTOM_QML_OVERRIDES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/CustomGuidedActionsController.qml"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FlyViewCustomLayer.qml"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FlyViewToolStripActionList.qml"
)
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/src/CustomGuidedActionsController.qml"
    PROPERTIES QT_RESOURCE_ALIAS "QGroundControl/FlyView/CustomGuidedActionsController.qml"
)
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/src/FlyViewCustomLayer.qml"
    PROPERTIES QT_RESOURCE_ALIAS "QGroundControl/FlyView/FlyViewCustomLayer.qml"
)
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/src/FlyViewToolStripActionList.qml"
    PROPERTIES QT_RESOURCE_ALIAS "QGroundControl/FlyView/FlyViewToolStripActionList.qml"
)
qt_add_resources(${CMAKE_PROJECT_NAME} custom_qml_overrides
    PREFIX "/Custom/qml"
    FILES ${CUSTOM_QML_OVERRIDES}
)

set(CUSTOM_BRAND_RESOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/res/Images/QGCLogoFull.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/icons/custom_qgroundcontrol.ico"
)
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/res/Images/QGCLogoFull.svg"
    PROPERTIES QT_RESOURCE_ALIAS "QGCLogoFull.svg"
)
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/res/icons/custom_qgroundcontrol.ico"
    PROPERTIES QT_RESOURCE_ALIAS "qgroundcontrol.ico"
)
qt_add_resources(${CMAKE_PROJECT_NAME} custom_brand
    PREFIX "/Custom/res"
    FILES ${CUSTOM_BRAND_RESOURCES}
)

set(CUSTOM_SHARED_GEAR "${CMAKE_SOURCE_DIR}/resources/gear-white.svg")
set_source_files_properties("${CUSTOM_SHARED_GEAR}" PROPERTIES QT_RESOURCE_ALIAS "gear-white.svg")
qt_add_resources(${CMAKE_PROJECT_NAME} custom_shared_resources
    PREFIX "/res"
    FILES "${CUSTOM_SHARED_GEAR}"
)

set(CUSTOM_JSON_RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/res/json/PerimeterScan.SettingsGroup.json")
qgc_set_qt_resource_alias("${CUSTOM_JSON_RESOURCE}")
qt_add_resources(${CMAKE_PROJECT_NAME} custom_json
    PREFIX "/json"
    FILES "${CUSTOM_JSON_RESOURCE}"
)

list(APPEND QML_IMPORT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/res")
set(QML_IMPORT_PATH "${QML_IMPORT_PATH}" CACHE STRING "Extra QML import paths" FORCE)

# ============================================================================
# Custom QML Module
# ============================================================================

qt_add_library(CustomModule STATIC)
qgc_configure_target(CustomModule)

set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/res/Custom/Widgets/CustomArtificialHorizon.qml PROPERTIES QT_RESOURCE_ALIAS CustomArtificialHorizon.qml)
qgc_set_qt_resource_alias(
    ${CMAKE_CURRENT_SOURCE_DIR}/res/Custom/Widgets/CustomAttitudeWidget.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/res/Custom/Widgets/CustomIconButton.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/res/Custom/Widgets/CustomOnOffSwitch.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/res/Custom/Widgets/CustomQuickButton.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/res/Custom/Widgets/CustomSignalStrength.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/res/Custom/Widgets/CustomToolBarButton.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/res/Custom/Widgets/CustomVehicleButton.qml
)

qt_add_qml_module(CustomModule
    URI Custom.Widgets
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/res/Custom/Widgets/CustomArtificialHorizon.qml
        ${CMAKE_CURRENT_SOURCE_DIR}/res/Custom/Widgets/CustomAttitudeWidget.qml
        ${CMAKE_CURRENT_SOURCE_DIR}/res/Custom/Widgets/CustomIconButton.qml
        ${CMAKE_CURRENT_SOURCE_DIR}/res/Custom/Widgets/CustomOnOffSwitch.qml
        ${CMAKE_CURRENT_SOURCE_DIR}/res/Custom/Widgets/CustomQuickButton.qml
        ${CMAKE_CURRENT_SOURCE_DIR}/res/Custom/Widgets/CustomSignalStrength.qml
        ${CMAKE_CURRENT_SOURCE_DIR}/res/Custom/Widgets/CustomToolBarButton.qml
        ${CMAKE_CURRENT_SOURCE_DIR}/res/Custom/Widgets/CustomVehicleButton.qml
    NO_PLUGIN
)

# ============================================================================
# Custom Plan Module (PerimeterScan map visual + editor)
# ============================================================================

qt_add_library(CustomPlanModule STATIC)
qgc_configure_target(CustomPlanModule)

qgc_set_qt_resource_alias(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PlanView/PerimeterScanMapVisual.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PlanView/PerimeterScanEditor.qml
)

qt_add_qml_module(CustomPlanModule
    URI Custom.Plan
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlanView/PerimeterScanMapVisual.qml
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlanView/PerimeterScanEditor.qml
    NO_PLUGIN
)

# ============================================================================
# Custom Plugin Sources
# ============================================================================

set(CUSTOM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CustomPlugin.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CustomPlugin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AutoPilotPlugin/CustomAutoPilotPlugin.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AutoPilotPlugin/CustomAutoPilotPlugin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FirmwarePlugin/CustomFirmwarePlugin.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FirmwarePlugin/CustomFirmwarePlugin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FirmwarePlugin/CustomFirmwarePluginFactory.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FirmwarePlugin/CustomFirmwarePluginFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MissionManager/PerimeterScanComplexItem.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MissionManager/PerimeterScanComplexItem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MissionManager/PerimeterScanPlanCreator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MissionManager/PerimeterScanPlanCreator.h
    PARENT_SCOPE
)

# ============================================================================
# Custom Build Configuration
# ============================================================================

set(CUSTOM_LIBRARIES
    CustomModule
    CustomPlanModule
    PARENT_SCOPE
)

set(CUSTOM_INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AutoPilotPlugin
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FirmwarePlugin
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MissionManager
    PARENT_SCOPE
)

set(CUSTOM_DEFINITIONS
    QGC_CUSTOM_BUILD
    CUSTOMHEADER="CustomPlugin.h"
    CUSTOMCLASS=CustomPlugin
    PARENT_SCOPE
)

set(CUSTOM_QT_COMPONENTS
    Core
    PARENT_SCOPE
)
