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

    property real   _toolsMargin:           ScreenTools.defaultFontPixelWidth * 0.75
    property real _fontSize:              ScreenTools.defaultFontPointSize

    MetFlightDataRecorder {
        id:             metFlightData
        width:          parent.width
        height:         250
        anchors.top:    parent.top
    }

    // // divider line
    // Rectangle {
    //     width: parent.width - 2 * _toolsMargin
    //     height: 1
    //     color: qgcPal.text
    //     anchors.top: metFlightData.top
    //     anchors.topMargin: _toolsMargin
    //     anchors.left: parent.left
    //     anchors.leftMargin: _toolsMargin
    // }

    Rectangle {
        id: valueAreaBackground
        anchors.bottom: parent.bottom
        anchors.leftMargin: _toolsMargin
        anchors.topMargin: _toolsMargin
        color: qgcPal.window
        width: parent.width / 2
        height: 250
        MetFactValueGrid {
            id:                     valueArea
            defaultSettingsGroup:   metDataDefaultSettingsGroup
            anchors.fill: parent
        }
    }

    Rectangle {
        id:                                 goToFileBackground
        anchors.bottom:                     parent.bottom
        anchors.bottomMargin:               _toolsMargin
        anchors.right:                      parent.right
        anchors.rightMargin:                _toolsMargin

        width:                              parent.width / 2
        height:                             valueAreaBackground.height

        color:                              qgcPal.window

        QGCButton {
            id:                             goToFileButton
            width:                          80 * 1.25
            height:                         80
            anchors.centerIn:               parent

            contentItem: Item {
                id:                         _content
                anchors.fill:               goToFileButton

                QGCColoredImage {
                    id:                     icon
                    source:                  "/InstrumentValueIcons/document.svg"
                    height:                 goToFileLabel.height
                    width:                  height
                    color:                  goToFileLabel.color
                    fillMode:               Image.PreserveAspectFit
                    sourceSize.height:      height
                    anchors.top:            parent.top
                    anchors.topMargin:      _toolsMargin
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                QGCLabel {
                    id:                         goToFileLabel
                    font.pointSize:             _fontSize
                    anchors.horizontalCenter:   parent.horizontalCenter
                    anchors.bottom:             parent.bottom
                    text:                       qsTr("Go to File")
                    color:                      qgcPal.text
                }
            }
            MouseArea {
                anchors.fill:   parent
                onClicked:      metFlightData.goToFile()  
            }
            background: Item {
                anchors.fill: parent
            }
        }
    }
}
