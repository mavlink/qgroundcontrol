/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                      2.11
import QtQuick.Controls             2.4
import QtQuick.Layouts              1.11
import QtQuick.Dialogs              1.3

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.ScreenTools   1.0

Item {
    anchors.fill:           parent
    anchors.margins:        ScreenTools.defaultFontPixelWidth

    property var curVehicle:        controller ? controller.activeVehicle : null
    property int curMessageIndex:   0
    property var curMessage:        curVehicle && curVehicle.messages.count ? curVehicle.messages.get(curMessageIndex) : null

    MAVLinkInspectorController {
        id: controller
    }

    DeadMouseArea {
        anchors.fill: parent
    }

    //-- Header
    QGCLabel {
        id:                 header
        text:               qsTr("Inspect real time MAVLink messages.")
        anchors.top:        parent.top
        anchors.left:       parent.left
    }

    //-- Messages (Buttons)
    QGCFlickable {
        id:                 buttonGrid
        anchors.top:        header.bottom
        anchors.topMargin:  ScreenTools.defaultFontPixelHeight
        anchors.bottom:     parent.bottom
        anchors.left:       parent.left
        width:              buttonCol.width
        contentWidth:       buttonCol.width
        contentHeight:      buttonCol.height
        ColumnLayout {
            id:             buttonCol
            spacing:        ScreenTools.defaultFontPixelHeight * 0.25
            Repeater {
                model:      curVehicle ? curVehicle.messages : []
                delegate:   MAVLinkMessageButton {
                    text:       object.name
                    checked:    curMessageIndex === index
                    messageHz:  object.messageHz
                    onClicked:  curMessageIndex = index
                    Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 36
                }
            }
        }
    }
    //-- Message Data
    QGCFlickable {
        id:                 messageGrid
        visible:            curMessage !== null
        anchors.top:        buttonGrid.top
        anchors.bottom:     parent.bottom
        anchors.left:       buttonGrid.right
        anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 2
        anchors.right:      parent.right
        contentWidth:       messageCol.width
        contentHeight:      messageCol.height
        ColumnLayout {
            id:                 messageCol
            spacing:            ScreenTools.defaultFontPixelHeight * 0.25
            GridLayout {
                columns:        2
                columnSpacing:  ScreenTools.defaultFontPixelWidth
                rowSpacing:     ScreenTools.defaultFontPixelHeight * 0.25
                QGCLabel {
                    text:       qsTr("Message:")
                    Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 20
                }
                QGCLabel {
                    color:      qgcPal.buttonHighlight
                    text:       curMessage ? curMessage.name + ' (' + curMessage.id + ') ' + curMessage.messageHz.toFixed(1) + 'Hz' : ""
                }
                QGCLabel {
                    text:       qsTr("Component:")
                }
                QGCLabel {
                    text:       curMessage ? curMessage.cid : ""
                }
                QGCLabel {
                    text:       qsTr("Count:")
                }
                QGCLabel {
                    text:       curMessage ? curMessage.count : ""
                }
            }
            Item { height: ScreenTools.defaultFontPixelHeight; width: 1 }
            QGCLabel {
                text:       qsTr("Message Fields:")
            }
            Rectangle {
                Layout.fillWidth: true
                height:     1
                color:      qgcPal.text
            }
            Item { height: ScreenTools.defaultFontPixelHeight * 0.25; width: 1 }
            GridLayout {
                columns:        3
                columnSpacing:  ScreenTools.defaultFontPixelWidth
                rowSpacing:     ScreenTools.defaultFontPixelHeight * 0.25
                Repeater {
                    model:      curMessage ? curMessage.fields : []
                    delegate:   QGCLabel {
                        Layout.row:         index
                        Layout.column:      0
                        Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 20
                        text:               object.name
                    }
                }
                Repeater {
                    model:      curMessage ? curMessage.fields : []
                    delegate:   QGCLabel {
                        Layout.row:         index
                        Layout.column:      1
                        Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 30
                        Layout.maximumWidth: ScreenTools.defaultFontPixelWidth * 30
                        wrapMode:           Text.WordWrap
                        text:               object.value
                    }
                }
                Repeater {
                    model:      curMessage ? curMessage.fields : []
                    delegate:   QGCLabel {
                        Layout.row:         index
                        Layout.column:      2
                        text:               object.type
                    }
                }
            }
        }
    }
}
