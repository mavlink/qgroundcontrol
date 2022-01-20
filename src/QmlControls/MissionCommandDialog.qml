/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2
import QtQuick.Dialogs  1.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

QGCPopupDialog {
    id:         root
    title:      qsTr("Select Mission Command")
    buttons:    StandardButton.Cancel

    property var    vehicle
    property var    missionItem
    property var    map
    property bool   flyThroughCommandsAllowed

    ColumnLayout {
        RowLayout {
            spacing: ScreenTools.defaultFontPixelWidth

            QGCLabel {
                text: qsTr("Category:")
            }

            QGCComboBox {
                id:                     categoryCombo
                Layout.preferredWidth:  30 * ScreenTools.defaultFontPixelWidth
                model:                  QGroundControl.missionCommandTree.categoriesForVehicle(vehicle)

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
        }

        Repeater {
            id:                 commandList
            Layout.fillWidth:   true

            delegate: Rectangle {
                width:  parent.width
                height: commandColumn.height + ScreenTools.defaultFontPixelHeight
                color:  QGroundControl.globalPalette.button

                property var    mavCmdInfo: modelData
                property color  textColor:  QGroundControl.globalPalette.buttonText

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
                        root.close()
                    }
                }
            }
        }
    }
}
