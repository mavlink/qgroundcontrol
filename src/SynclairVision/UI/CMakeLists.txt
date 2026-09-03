# ============================================================================
# SynclairVision UI
# ============================================================================
qt_add_library(SynclairVisionUIModule STATIC)

set_source_files_properties(
    Settings/SVSettings.qml
    Settings/SVState.qml
    Flyview/SVNotificationManager.qml
    Resources/SVUnits.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(SynclairVisionUIModule
    URI QGroundControl.SynclairVisionUI
    VERSION 1.0
    RESOURCE_PREFIX /qml
    SOURCES
        ../Digiview/DigiviewProtocolEnums.h
    QML_FILES
        Flyview/SVFlyView.qml
        Flyview/SVFlyViewMenus.qml
        Flyview/SVFlyViewMenusList.js
        Flyview/SVFlyViewWidgetLayer.qml
        Flyview/SVRecordInfoBox.qml
        Flyview/SVFlyViewStats.qml
        Flyview/SVShortcutHandler.qml
        Flyview/SVWelcomePrompt.qml
        Flyview/SVNotificationManager.qml
        Flyview/SVNotificationBox.qml
        Flyview/SVFlyViewDetectionOverlay.qml
        Flyview/SVFlyViewDetectionButton.qml

        Camera/SVCameraLayer.qml
        Camera/SVCameraWidgetLayer.qml
        Camera/SVCameraLayerOverlays.qml
        Camera/SVCameraDetectionFlag.qml

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
        Settings/SVSettingsDefinitions.js

        Resources/SVArrow.qml
        Resources/SVBackground.qml
        Resources/SVBorder.qml
        Resources/SVTooltip.qml
        Resources/SVLine.qml


    RESOURCES
        Resources/Images/halftone_texture_2.png
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

        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/external_link.svg"


        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/camera_photo_.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/camera_record.svg"

        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/settings_main.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/settings_main_open.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/settings_general.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/settings_network.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/settings_controls.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/settings_dev.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/settings_shortcut.svg"

        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/overlay_main.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/overlay_grid.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/overlay_cross.svg"

        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/tracking_main.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/tracking_cursor.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/tracking_manual.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/tracking_single.svg"

        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/network_disconnected.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/network_connected.svg"







        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/toolbar_open.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/toolbar_closed.svg"

        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/controls_lock.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/controls_lock_closed.svg"

        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/hud_eye.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/hud_eye_closed.svg"

        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/layout_ai.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Icons/layout_ai_bold.svg"


)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(SynclairVisionUIModule PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Digiview
    ${CMAKE_SOURCE_DIR}/message-definitions
)
