# ============================================================================
# SynclairVision UI
# ============================================================================
qt_add_library(SynclairVisionUIModule STATIC)

set_source_files_properties(
    Settings/SVSettings.qml
    Settings/SVState.qml
    Resources/SVUnits.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(SynclairVisionUIModule
    URI QGroundControl.SynclairVisionUI
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        Flyview/SVFlyView.qml
        Flyview/SVFlyViewMenus.qml
        Flyview/SVFlyViewMenusList.js
        Flyview/SVFlyViewWidgetLayer.qml
        Flyview/SVRecordInfoBox.qml

        Camera/SVCameraLayer.qml
        Camera/SVCameraWidgetLayer.qml
        Camera/SVCameraLayouts.js

        Controls/SVControlPanel.qml
        Controls/SVJoystick.qml
        Controls/SVJoystickArea.qml
        Controls/SVJoystickDrag.qml
        Controls/SVJoystickButtonSegment.qml
        Controls/SVZoom.qml
        Controls/SVZoomArea.qml
        Controls/SVZoomButton.qml
        
        Menustrip/SVMenuStrip.qml
        Menustrip/SVMenuStripButton.qml

        Settings/SVSettings.qml
        Settings/SVSettingsDrawer.qml
        Settings/SVState.qml
        Resources/SVUnits.qml
        
        Settings/SVSettingsMenu.qml
        Settings/SVSettingsGeneral.js
        Settings/SVSettingsControls.js
        Settings/SVSettingsDev.js

        Resources/SVArrow.qml

    RESOURCES
        Resources/Images/no-video-camera-background.png
        Resources/Images/no_video_background.png
        
    NO_PLUGIN
)

qt_add_resources(SynclairVisionUIModule synclairvision_qmlimages
    PREFIX "/qmlimages"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons"
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/layout_single.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/layout_double.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/layout_quadruple.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/layout_double_panorama.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/layout_double+panorama.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/layout_triple+panorama.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/layout_single_panorama.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/layout_main.svg"

        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/camera_photo.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/camera_record.svg"

        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/settings_main.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/settings_main_open.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/settings_general.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/settings_controls.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/settings_dev.svg"

        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/toolbar_open.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/toolbar_closed.svg"

        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/controls_lock.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/controls_lock_closed.svg"

        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/hud_eye.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/hud_eye_closed.svg"
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
