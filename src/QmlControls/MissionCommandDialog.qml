/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

import QtQuick                  2.5
import QtQuick.Controls         1.4

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

QGCViewDialog {
    id: root

    property var missionItem

    property var _vehicle: QGroundControl.multiVehicleManager.activeVehicle

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
        model:              QGroundControl.missionCommands.categories(_vehicle)

        function categorySelected(category) {
            commandList.model = QGroundControl.missionCommands.getCommandsForCategory(_vehicle, category)
        }

        Component.onCompleted: {
            var category  = missionItem.category
            currentIndex = find(category)
            categorySelected(category)
        }

        onActivated: categorySelected(textAt(index))
    }

    ListView {
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
            height: commandColumn.height + ScreenTools.defaultFontPixelSize
            color:  qgcPal.button

            property var    mavCmdInfo: object
            property var    textColor:  qgcPal.buttonText

            Column {
                id:                 commandColumn
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.top:        parent.top

                QGCLabel {
                    text:           mavCmdInfo.friendlyName
                    color:          textColor
                    font.weight:    Font.DemiBold
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
                    missionItem.command = mavCmdInfo.command
                    root.reject()
                }
            }
        }
    } // ListView
} // QGCViewDialog
