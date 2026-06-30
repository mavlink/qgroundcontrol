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

        SVCameraLayouts.js
    RESOURCES
        Images/no-video-camera-background.png
        Images/no_video_background.png

    NO_PLUGIN
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
