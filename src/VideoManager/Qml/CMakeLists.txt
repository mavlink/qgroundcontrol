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
        UVCVideoOutput.qml
        VideoDisplaySettingsGroup.qml
        VideoStreamFormat.js
        VideoStreamOutput.qml
        VideoStreamStatsGroup.qml
        VideoView.qml
    NO_PLUGIN
)
