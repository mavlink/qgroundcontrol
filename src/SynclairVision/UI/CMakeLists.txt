# ============================================================================
# SynclairVision UI
# ============================================================================
qt_add_library(SynclairVisionUIModule STATIC)

set_source_files_properties(SVState.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(SynclairVisionUIModule
    URI QGroundControl.SynclairVisionUI
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        SVFlyView.qml
        SVFlyViewWidgetLayer.qml
        SVState.qml
        SVCameraLayer.qml
        SVCameraWidgetLayer.qml
        SVControlPanel.qml
        SVVirtualJoystick.qml
        SVJoystickArea.qml
        SVJoystickButtonSegment.qml
        SVZoom.qml
        SVZoomArea.qml
        SVZoomButton.qml
        SVMenuStrip.qml
        SVMenuStripButton.qml
        SVCameraLayouts.js
    RESOURCES
        Images/no-video-camera-background.png
        Images/no_video_background.png
    NO_PLUGIN
)

qt_add_resources(SynclairVisionUIModule synclairvision_qmlimages
    PREFIX "/qmlimages"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/Images/Icons"
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/Icons/layout_single.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/Icons/layout_double.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/Icons/layout_quadruple.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/Icons/layout_double_panorama.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/Icons/layout_double+panorama.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/Icons/layout_triple+panorama.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/Icons/layout_single_panorama.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/Icons/layout_main.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/Icons/camera_photo.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/Icons/camera_record.svg"

        "${CMAKE_CURRENT_SOURCE_DIR}/Images/Icons/settings_main.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/Icons/settings_general.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/Icons/settings_controls.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/Icons/settings_dev.svg"


        "${CMAKE_CURRENT_SOURCE_DIR}/Images/Icons/hud_eye.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/Icons/hud_eye_closed.svg"


)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
