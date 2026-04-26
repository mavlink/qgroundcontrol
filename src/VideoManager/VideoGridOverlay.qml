import QtQuick

import QGroundControl

// Rule-of-thirds grid overlay for video content.
// Toggle via VideoSettings.gridLines. Hidden in fullscreen mode.
Item {
    anchors.fill: parent
    visible: QGroundControl.settingsManager.videoSettings.gridLines.rawValue
             && !QGroundControl.videoManager.fullScreen

    Rectangle {
        color:  Qt.rgba(1,1,1,0.5)
        height: parent.height
        width:  1
        x:      parent.width * 0.33
    }
    Rectangle {
        color:  Qt.rgba(1,1,1,0.5)
        height: parent.height
        width:  1
        x:      parent.width * 0.66
    }
    Rectangle {
        color:  Qt.rgba(1,1,1,0.5)
        width:  parent.width
        height: 1
        y:      parent.height * 0.33
    }
    Rectangle {
        color:  Qt.rgba(1,1,1,0.5)
        width:  parent.width
        height: 1
        y:      parent.height * 0.66
    }
}
