import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Viewer3D

/// Ground-scale indicator for the 3D viewer, patterned after MapScale.qml.
/// Shows the ground distance covered on screen at the ground plane through the
/// screen center (scale varies elsewhere in a perspective view).
Item {
    id: control
    width: rightEnd.x + rightEnd.width
    height: rightEnd.y + rightEnd.height

    property Viewer3DCameraController controller
    property bool autoHide: false ///< true: disappears after a timeout on scale change

    // Scene units are meters x 10
    readonly property real _sceneToMeters: 0.1
    property var _scaleLengthsMeters: [5, 10, 25, 50, 100, 150, 250, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, 500000, 1000000, 2000000]
    property var _scaleLengthsFeet: [10, 25, 50, 100, 250, 500, 1000, 2000, 3000, 4000, 5280, 5280*2, 5280*5, 5280*10, 5280*25, 5280*50, 5280*100, 5280*250, 5280*500, 5280*1000]

    function formatDistanceMeters(meters) {
        var dist = Math.round(meters)
        if (dist > 1000) {
            if (dist > 100000) {
                dist = Math.round(dist / 1000)
            } else {
                dist = Math.round(dist / 100)
                dist = dist / 10
            }
            dist = dist + qsTr(" km")
        } else {
            dist = dist + qsTr(" m")
        }
        return dist
    }

    function formatDistanceFeet(feet) {
        var dist = Math.round(feet)
        if (dist >= 5280) {
            dist = Math.round(dist / 5280)
            if (dist === 1) {
                dist += qsTr(" mile")
            } else {
                dist += qsTr(" miles")
            }
        } else {
            dist = dist + qsTr(" ft")
        }
        return dist
    }

    function _applyNiceLength(scaleLineLength, scaleLinePixelLength, scaleLengths) {
        var scaleLineRatio = 0
        var i = 0

        if (scaleLineLength === 0) {
            return 0
        }

        for (i = 0; i < scaleLengths.length - 1; i++) {
            if (scaleLineLength < ((scaleLengths[i] + scaleLengths[i + 1]) / 2)) {
                scaleLineRatio = scaleLengths[i] / scaleLineLength
                scaleLineLength = scaleLengths[i]
                break
            }
        }
        if (scaleLineRatio === 0) {
            scaleLineRatio = scaleLengths[i] / scaleLineLength
            scaleLineLength = scaleLengths[i]
        }

        centerLine.width = (scaleLinePixelLength * scaleLineRatio) - (2 * leftEnd.width)
        return scaleLineLength
    }

    function calculateScale() {
        if (!controller) {
            return
        }

        var scaleLinePixelLength = 100
        var scaleLineMeters = controller.sceneUnitsPerPixel() * scaleLinePixelLength * _sceneToMeters
        if (scaleLineMeters <= 0) {
            return
        }

        if (QGroundControl.settingsManager.unitsSettings.horizontalDistanceUnits.value === UnitsSettings.HorizontalDistanceUnitsFeet) {
            var niceFeet = _applyNiceLength(scaleLineMeters * 3.2808399, scaleLinePixelLength, _scaleLengthsFeet)
            scaleText.text = formatDistanceFeet(niceFeet)
        } else {
            var niceMeters = _applyNiceLength(scaleLineMeters, scaleLinePixelLength, _scaleLengthsMeters)
            scaleText.text = formatDistanceMeters(niceMeters)
        }
    }

    function triggerRecalc() {
        calculateScale()
        if (control.autoHide) {
            autoHideTimer.restart()
            autoHideAnimation.stop()
            control.opacity = 1
        }
    }

    Component.onCompleted: triggerRecalc()
    onControllerChanged: triggerRecalc()

    Connections {
        target: control.controller

        function onDistanceChanged() { control.triggerRecalc() }
        function onViewportSizeChanged() { control.triggerRecalc() }
        function onFieldOfViewChanged() { control.triggerRecalc() }
    }

    PropertyAnimation {
        id: autoHideAnimation
        target: control
        property: "opacity"
        from: 1
        to: 0
        duration: 500
    }

    Timer {
        id: autoHideTimer
        interval: 3000
        onTriggered: autoHideAnimation.start()
    }

    QGCPalette { id: qgcPal }

    QGCLabel {
        id: scaleText
        anchors.left: parent.left
        anchors.right: rightEnd.right
        font.bold: true
        horizontalAlignment: Text.AlignRight
        text: "0 m"
    }

    Rectangle {
        id: leftEnd
        anchors.top: scaleText.bottom
        anchors.left: parent.left
        width: 2
        height: ScreenTools.defaultFontPixelHeight
        color: qgcPal.text
    }

    Rectangle {
        id: centerLine
        anchors.bottomMargin: 2
        anchors.bottom: leftEnd.bottom
        anchors.left: leftEnd.right
        height: 2
        color: qgcPal.text
    }

    Rectangle {
        id: rightEnd
        anchors.top: leftEnd.top
        anchors.left: centerLine.right
        width: 2
        height: ScreenTools.defaultFontPixelHeight
        color: qgcPal.text
    }
}
