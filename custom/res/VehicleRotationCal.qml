/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.4

import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

Rectangle {
    // Indicates whether calibration is valid for this control
    property bool calValid: false

    // Indicates whether the control is currently being calibrated
    property bool calInProgress: false

    // Text to show while calibration is in progress
    property string calInProgressText: qsTr("Hold Still")

    // Image source and size
    property string imageSource:    ""
    property real   imageWidth:     parent.width
    property real   imageHeight:    parent.height

    property var __qgcPal: QGCPalette { colorGroupEnabled: enabled }

    color:  calInProgress ? "yellow" : (calValid ? "green" : "red")

    Rectangle {
        readonly property int inset: 2

        width:  parent.width  - (inset * 2)
        height: parent.height - (inset * 2)
        color: qgcPal.windowShade
        anchors.centerIn:   parent

        Image {
            width:              imageWidth
            height:             imageHeight
            source:             imageSource
            fillMode:           Image.PreserveAspectFit
            smooth:             true
            mipmap:             true
            antialiasing:       true
            anchors.centerIn:   parent
        }

        QGCLabel {
            width:                  parent.width
            height:                 parent.height
            horizontalAlignment:    Text.AlignHCenter
            verticalAlignment:      Text.AlignBottom
            font.pointSize:         ScreenTools.mediumFontPointSize
            text:                   calInProgress ? calInProgressText : (calValid ? qsTr("Completed") : qsTr("Incomplete"))
        }
    }
}
