import QtQuick
import QtQuick.Controls
import QGroundControl
import QGroundControl.Controls

Column {
    property var setting: null
    property var fact: setting ? setting.fact : null
    property real value: fact ? fact.rawValue : 0

    property string label: ""
    property real from: 0
    property real to: 100
    property real stepSize: 1
    property int decimalPlaces: 0
    property var onValueChange: null
    property bool enabled: true

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
        stepSize: parent.stepSize
        snapMode: Slider.SnapAlways
        enabled: parent.enabled

        onValueChanged: {
            // TODO: this shouldn't be necessary -- we need updates to reflect on the sliders (when we receive fresh eeprom data)
            // if (!pressed) return  // Only update when user is dragging
            if (setting) setting.setPendingValue(value)
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
}
