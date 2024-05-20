/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 2.15
import QtQuick.Layouts  1.15
import QtQuick.Dialogs  1.3

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controllers   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.ScreenTools   1.0

Rectangle {
    id:                 metDataWindow
    height:             700
    width:              900
    color:              qgcPal.window
    radius:             ScreenTools.defaultFontPixelWidth / 2

    property real _toolsMargin:           ScreenTools.defaultFontPixelWidth
    property real _fontSize:              ScreenTools.defaultFontPointSize

    // mouse area to prevent clicks from going through to the map
    MouseArea {
        anchors.fill: parent
        onClicked: {}
    }

    MetFlightDataRecorder {
        id:             metFlightData
        width:          parent.width
        anchors.top:    parent.top
        anchors.bottom: valueAreaBackground.top
        anchors.bottomMargin: _toolsMargin

    }

    Rectangle {
        id: valueAreaBackground
        color: qgcPal.window

        anchors.bottom: parent.bottom
        anchors.bottomMargin: _toolsMargin
        height: childrenRect.height
        width: childrenRect.width + _toolsMargin

        MetFactValueGrid {
            id:                     valueArea
            defaultSettingsGroup:   metDataDefaultSettingsGroup
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.leftMargin: _toolsMargin
        }
    }

    QGCButton {
        id:                             goToFileButton
        anchors.right:                  parent.right
        anchors.bottom:                 parent.bottom
        anchors.margins:                _toolsMargin
        text:                           qsTr("Go to File")
        onClicked:                      metFlightData.goToFile()  
    }
}
