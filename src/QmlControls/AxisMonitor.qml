import QtQuick                      2.11
import QtQuick.Controls             2.4

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

Item {
    property int    axisValue:          0
    property int    deadbandValue:      0
    property bool   narrowIndicator:    false
    property color  deadbandColor:      "#8c161a"
    property bool   mapped:             false
    property bool   reversed:           false

    property color  __barColor:         qgcPal.windowShade

    // Bar
    Rectangle {
        id:                     bar
        anchors.verticalCenter: parent.verticalCenter
        width:                  parent.width
        height:                 parent.height / 2
        color:                  __barColor
    }

    // Deadband
    Rectangle {
        id:                     deadbandBar
        anchors.verticalCenter: parent.verticalCenter
        x:                      _deadbandPosition
        width:                  _deadbandWidth
        height:                 parent.height / 2
        color:                  deadbandColor
        visible:                controller.deadbandToggle

        property real _percentDeadband:     ((2 * deadbandValue) / (32768.0 * 2))
        property real _deadbandWidth:       parent.width * _percentDeadband
        property real _deadbandPosition:    (parent.width - _deadbandWidth) / 2
    }

    // Center point
    Rectangle {
        anchors.horizontalCenter:   parent.horizontalCenter
        width:                      ScreenTools.defaultFontPixelWidth / 2
        height:                     parent.height
        color:                      qgcPal.window
    }

    // Indicator
    Rectangle {
        anchors.verticalCenter: parent.verticalCenter
        width:                  parent.narrowIndicator ?  height/6 : height
        height:                 parent.height * 0.75
        x:                      (reversed ? (parent.width - _indicatorPosition) : _indicatorPosition) - (width / 2)
        radius:                 width / 2
        color:                  qgcPal.text
        visible:                mapped

        property real _percentAxisValue:    ((axisValue + 32768.0) / (32768.0 * 2))
        property real _indicatorPosition:   parent.width * _percentAxisValue
    }

    QGCLabel {
        anchors.fill:           parent
        horizontalAlignment:    Text.AlignHCenter
        verticalAlignment:      Text.AlignVCenter
        text:                   qsTr("Not Mapped")
        visible:                !mapped
    }

    ColorAnimation {
        id:         barAnimation
        target:     bar
        property:   "color"
        from:       "yellow"
        to:         __barColor
        duration:   1500
    }

    // Axis value debugger
    /*
    QGCLabel {
        anchors.fill: parent
        text: axisValue
    }
    */

}
