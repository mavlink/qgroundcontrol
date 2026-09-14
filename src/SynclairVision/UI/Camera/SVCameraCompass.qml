import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

Rectangle {
    id: root

    required property real heading
    property real size: ScreenTools.defaultFontPixelHeight * 10
    property bool showBorder: true

    width: size
    height: size
    radius: width / 2
    color: qgcPalette.window
    border.color: qgcPalette.text
    border.width: showBorder ? 1 : 0

    readonly property real normalizedHeading: Number.isFinite(heading) ? (heading % 360 + 360) % 360 : 0
    readonly property real _defaultSize: ScreenTools.defaultFontPixelHeight * 10
    readonly property real _sizeRatio: size / _defaultSize
    readonly property int _fontSize: Math.max(8, ScreenTools.defaultFontPointSize * _sizeRatio)

    QGCPalette { id: qgcPalette }

    Item {
        anchors.fill: parent
        rotation: -root.normalizedHeading

        CompassDial {
            anchors.fill: parent
        }

        CompassHeadingIndicator {
            compassSize: root.size
            heading: root.normalizedHeading
            simplified: false
        }
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        y: root.size * 0.58
        width: headingLabel.implicitWidth + ScreenTools.defaultFontPixelWidth
        height: headingLabel.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.15
        radius: height / 2
        color: qgcPalette.window

        QGCLabel {
            id: headingLabel

            anchors.centerIn: parent
            text: root.normalizedHeading.toFixed(0) + "°"
            color: qgcPalette.text
            font.bold: true
            font.pointSize: root._fontSize
        }
    }
}
