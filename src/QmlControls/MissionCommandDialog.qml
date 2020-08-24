/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                  2.3
import QtQuick.Controls         1.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

QGCViewDialog {
    id: root

    property var    vehicle
    property var    missionItem
    property var    map
    property bool   flyThroughCommandsAllowed

    QGCPalette { id: qgcPal }

    QGCLabel {
        id:                 categoryLabel
        anchors.baseline:   categoryCombo.baseline
        text:               qsTr("Category:")
    }

    QGCComboBox {
        id:                 categoryCombo
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        anchors.left:       categoryLabel.right
        anchors.right:      parent.right
        model:              QGroundControl.missionCommandTree.categoriesForVehicle(vehicle)

        function categorySelected(category) {
            commandList.model = QGroundControl.missionCommandTree.getCommandsForCategory(vehicle, category, flyThroughCommandsAllowed)
        }

        Component.onCompleted: {
            var category  = missionItem.category
            currentIndex = find(category)
            categorySelected(category)
        }

        onActivated: categorySelected(textAt(index))
    }

    QGCListView {
        id:                 commandList
        anchors.margins:    ScreenTools.defaultFontPixelHeight
        anchors.left:       parent.left
        anchors.right:      parent.right
        anchors.top:        categoryCombo.bottom
        anchors.bottom:     parent.bottom
        spacing:            ScreenTools.defaultFontPixelHeight / 2
        orientation:        ListView.Vertical
        clip:               true

        delegate: Rectangle {
            width:  parent.width
            height: commandColumn.height + ScreenTools.defaultFontPixelHeight
            color:  qgcPal.button

            property var    mavCmdInfo: modelData
            property color  textColor:  qgcPal.buttonText

            Column {
                id:                 commandColumn
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.top:        parent.top

                QGCLabel {
                    text:           mavCmdInfo.friendlyName
                    color:          textColor
                    font.family:    ScreenTools.demiboldFontFamily
                }

                QGCLabel {
                    anchors.margins:    ScreenTools.defaultFontPixelWidth
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    text:               mavCmdInfo.description
                    wrapMode:           Text.WordWrap
                    color:              textColor
                }
            }

            MouseArea {
                anchors.fill:   parent
                onClicked: {
                    missionItem.setMapCenterHintForCommandChange(map.center)
                    missionItem.command = mavCmdInfo.command
                    root.reject()
                }
            }
        }
    } // QGCListView
} // QGCViewDialog
