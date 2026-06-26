# ============================================================================
# SynclairVision UI
# ============================================================================
qt_add_library(SynclairVisionUIModule STATIC)

qt_add_qml_module(SynclairVisionUIModule
    URI QGroundControl.SynclairVisionUI
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        SVFlyView.qml
        SVFlyViewWidgetLayer.qml

        SVCameraLayer.qml
        SVCameraWidgetLayer.qml

        DigitalJoystick.qml
        DigitalJoystickButton.qml
    NO_PLUGIN
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
