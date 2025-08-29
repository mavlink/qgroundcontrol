import QtQuick
import QtQuick.Controls
import QGroundControl
import QGroundControl.Controls

Column {
    property string factName: ""
    property string label: ""
    property real from: 0
    property real to: 100
    property real stepSize: 1
    property int decimalPlaces: 0
    property bool snapToStep: false
    property var onValueChange: null
    property bool enabled: true

    property real value: 0

    spacing: ScreenTools.defaultFontPixelHeight / 4

    QGCLabel {
        text: label
        anchors.horizontalCenter: parent.horizontalCenter
    }

    Slider {
        id: slider
        width: ScreenTools.defaultFontPixelWidth * 28
        from: parent.from
        to: parent.to
        value: parent.value
        stepSize: snapToStep ? parent.stepSize : 0
        snapMode: snapToStep ? Slider.SnapAlways : Slider.NoSnap
        enabled: parent.enabled

        onValueChanged: {
            parent.value = value
            if (onValueChange) {
                onValueChange(factName, value)
            }
        }

        // Visual handle
        handle: Rectangle {
            x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            implicitWidth: ScreenTools.defaultFontPixelHeight
            implicitHeight: implicitWidth
            radius: width / 2
            color: slider.pressed ? qgcPal.buttonHighlight : qgcPal.button
            border.color: qgcPal.text
        }

        // Value display
        ToolTip {
            parent: slider.handle
            visible: slider.pressed
            text: slider.value.toFixed(decimalPlaces)
        }
    }

    // Value label
    QGCLabel {
        text: value.toFixed(decimalPlaces)
        anchors.horizontalCenter: parent.horizontalCenter
        font.pointSize: ScreenTools.smallFontPointSize
    }

    function setValue(newValue) {
        value = newValue
    }
}
