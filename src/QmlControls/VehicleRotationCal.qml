import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.Palette 1.0

Rectangle {
    property var __qgcPal: QGCPalette { colorGroupEnabled: enabled }

    // Indicates whether calibration is valid for this control
    property bool calValid: false

    // Indicates whether the control is currently being calibrated
    property bool calInProgress: false

    // Image source
    property var imageSource: ""

    width:  200
    height: 200
    color:  calInProgress ? "yellow" : (calValid ? "green" : "red")

    Rectangle {
        readonly property int inset: 5
        property string calText: calInProgress ? "Hold Still" : (calValid ? "Completed" : "Incomplete")

        x:      inset
        y:      inset
        width:  parent.width - (inset * 2)
        height: parent.height - (inset * 2)
        color: qgcPal.windowShade

        Image {
            width:      parent.width
            height:     parent.height
            source:     imageSource
            fillMode:   Image.PreserveAspectFit
            smooth: true
        }

        Label {
            width:                  parent.width
            height:                 parent.height
            horizontalAlignment:    Text.AlignHCenter
            verticalAlignment:      Text.AlignBottom
            font.pointSize:         25
            font.bold:              true
            color:                  "black"

            text: parent.calText
        }
        Label {
            width:                  parent.width
            height:                 parent.height
            horizontalAlignment:    Text.AlignHCenter
            verticalAlignment:      Text.AlignBottom
            font.pointSize:         25
            color:                  calInProgress ? "yellow" : "white"

            text: parent.calText
        }
    }
}
