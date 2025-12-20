import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

/// Displays the value of a single channel as an indicator within a value range bar
Item {
    enum Mode {
        RawValue, // Display raw channel value
        MappedValue // Channel goes through a mapping process and can be reversed
    }

    required property int channelValueMin
    required property int channelValueMax
    required property int mode ///< Mode of display using Mode enum values
    property bool channelMapped: false
    property int channelValue: _valueBarRange / 2 + _valueBarMin

    id: control
    implicitHeight: ScreenTools.defaultFontPixelHeight

    readonly property real _valueRangeBarOvershootPercent: 0.2 // Pct overshoot on each end of display so indicator doesn't go right to edge
    readonly property int _halfRange: (channelValueMax - channelValueMin) / 2
    readonly property int _valueBarMin: channelValueMin - (_halfRange * _valueRangeBarOvershootPercent)
    readonly property int _valueBarMax: channelValueMax + (_halfRange * _valueRangeBarOvershootPercent)
    readonly property int _valueBarRange: _valueBarMax - _valueBarMin

    Component.onCompleted: {
        if (mode === RemoteControlChannelValueDisplay.RawValue) {
            if (channelMapped) {
                console.warn("RemoteControlChannelValueDisplay: channelMapped should not be true in Raw Value mode")
                channelMapped = false
            }
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: control.enabled }

    Rectangle {
        id: fullValueRangeBar
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width
        height: parent.height / 2
        color: qgcPal.windowShade
    }

    Rectangle {
        id: centerPointDisplay
        anchors.horizontalCenter: parent.horizontalCenter
        width: ScreenTools.defaultFontPixelWidth / 2
        height: parent.height
        color: qgcPal.window
    }

    QGCLabel {
        id: notMappedLabel
        anchors.centerIn: parent
        text: qsTr("Not Mapped")
        visible: control.mode === RemoteControlChannelValueDisplay.MappedValue && !control.channelMapped
    }

    // Indicator
    Rectangle {
        anchors.verticalCenter: parent.verticalCenter
        width: parent.height * 0.75
        height: width
        x: {
            const clampedValue = Math.max(control._valueBarMin, Math.min(control._valueBarMax, control.channelValue))
            const normalized = (clampedValue - control._valueBarMin) / control._valueBarRange
            return (normalized * parent.width) - (width / 2)
        }
        radius: width / 2
        color: qgcPal.text
        visible: control.mode === RemoteControlChannelValueDisplay.RawValue || control.channelMapped
    }
}
