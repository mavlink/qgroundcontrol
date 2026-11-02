import QtQuick

import QGroundControl
import QGroundControl.Controls

import "VideoStreamFormat.js" as VideoStreamFormat

// The health of the USB camera on display, from its monitor; shown only while the camera is measured.
QGCGroupBox {
    id: root

    required property UVCStreamMonitor monitor

    readonly property var  _stats:      monitor.stats
    readonly property size _videoSize:  monitor.videoSize
    readonly property real _cameraFps:  monitor.cameraFrameRate
    readonly property string _fpsText:  qsTr("%1 fps").arg(_stats.fps.toFixed(1))

    heading: qsTr("USB Camera")
    visible: _stats.valid

    LabelledLabel {
        label:      qsTr("Camera")
        labelText:  root.monitor.cameraName
        visible:    labelText !== ""
    }

    LabelledLabel {
        label:      qsTr("Frame rate")
        labelText:  root._cameraFps > 0 ? qsTr("%1 (camera %2 fps)").arg(root._fpsText).arg(root._cameraFps.toFixed(0))
                                        : root._fpsText
    }

    LabelledLabel {
        label:      qsTr("Video")
        labelText:  qsTr("%1 × %2 %3").arg(root._videoSize.width).arg(root._videoSize.height).arg(root._stats.codec)
        visible:    root._videoSize.width > 0
    }

    LabelledLabel {
        label:      qsTr("Display path")
        labelText:  root._stats.gpuPath
    }

    LabelledLabel {
        label:      qsTr("Uptime")
        labelText:  VideoStreamFormat.durationText(root._stats.uptimeMs)
    }
}
