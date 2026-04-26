import QtQuick

import QGroundControl
import QGroundControl.Controls

// Placeholder shown when no video stream is active.
// Displays "WAITING FOR VIDEO" or "VIDEO DISABLED" over a background image.
Image {
    property bool useSmallFont: true

    source:   "/res/NoVideoBackground.jpg"
    fillMode: Image.PreserveAspectCrop

    Rectangle {
        anchors.centerIn: parent
        width:            label.contentWidth + ScreenTools.defaultFontPixelHeight
        height:           label.contentHeight + ScreenTools.defaultFontPixelHeight
        radius:           ScreenTools.defaultFontPixelWidth / 2
        color:            "black"
        opacity:          0.5
    }

    QGCLabel {
        id:               label
        text:             QGroundControl.settingsManager.videoSettings.streamEnabled.rawValue
                              ? qsTr("WAITING FOR VIDEO") : qsTr("VIDEO DISABLED")
        font.bold:        true
        color:            "white"
        font.pointSize:   useSmallFont ? ScreenTools.smallFontPointSize : ScreenTools.largeFontPointSize
        anchors.centerIn: parent
    }
}
