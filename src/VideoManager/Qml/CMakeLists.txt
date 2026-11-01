# ============================================================================
# Video QML Module
# Video components shared by the Fly View, the toolbar and the camera controls.
# ============================================================================

qt_add_library(VideoManagerModule STATIC)

qt_add_qml_module(
    VideoManagerModule
    URI QGroundControl.VideoManager
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        CameraPinchZoom.qml
        NoVideoPlaceholder.qml
        ThermalVideoOverlay.qml
        UVCCameraFormatComboBox.qml
        UVCStatsGroup.qml
        UVCVideoOutput.qml
        VideoDecodingSettingsGroup.qml
        VideoDisplaySettingsGroup.qml
        VideoGridLines.qml
        VideoStatusLabel.qml
        VideoStreamFormat.js
        VideoStreamOutput.qml
        VideoStreamSettingsGroup.qml
        VideoStreamStatsGroup.qml
        VideoView.qml
    NO_PLUGIN
)
