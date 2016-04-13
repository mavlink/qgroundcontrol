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

import QtQuick          2.5
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactControls  1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0

FactPanel {
    id:     _root
    height: monitorColumn.height

    QGCPalette { id: qgcPal; colorGroupEnabled: panel.enabled }

    RCChannelMonitorController {
        id:             controller
        factPanel:      _root
    }

    // Live channel monitor control component
    Component {
        id: channelMonitorDisplayComponent

        Item {
            property int    rcValue:    1500


            property int            __lastRcValue:      1500
            readonly property int   __rcValueMaxJitter: 2
            property color          __barColor:         qgcPal.windowShade

            // Bar
            Rectangle {
                id:                     bar
                anchors.verticalCenter: parent.verticalCenter
                width:                  parent.width
                height:                 parent.height / 2
                color:                  __barColor
            }

            // Center point
            Rectangle {
                anchors.horizontalCenter:   parent.horizontalCenter
                width:                      ScreenTools.defaultTextWidth / 2
                height:                     parent.height
                color:                      qgcPal.window
            }

            // Indicator
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width:                  parent.height * 0.75
                height:                 width
                x:                      ((Math.abs((rcValue - 1000) - (reversed ? 1000 : 0)) / 1000) * parent.width) - (width / 2)
                radius:                 width / 2
                color:                  qgcPal.text
                visible:                mapped
            }

            QGCLabel {
                anchors.fill:           parent
                horizontalAlignment:    Text.AlignHCenter
                verticalAlignment:      Text.AlignVCenter
                text:                   "Not Mapped"
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
        }
    } // Component - channelMonitorDisplayComponent

    Column {
        id:         monitorColumn
        width:      parent.width
        spacing:    5

        QGCLabel { text: "Channel Monitor" }

        Connections {
            target: controller

            onChannelRCValueChanged: {
                if (channelMonitorRepeater.itemAt(channel)) {
                    channelMonitorRepeater.itemAt(channel).loader.item.rcValue = rcValue
                }
            }
        }

        Repeater {
            id:     channelMonitorRepeater
            model:  controller.channelCount
            width:  parent.width

            Row {
                spacing:    5

                // Need this to get to loader from Connections above
                property Item loader: theLoader

                QGCLabel {
                    id:     channelLabel
                    text:   modelData + 1
                }

                Loader {
                    id:                     theLoader
                    anchors.verticalCenter: channelLabel.verticalCenter
                    height:                 qgcView.defaultTextHeight
                    width:                  200
                    sourceComponent:        channelMonitorDisplayComponent

                    property bool mapped:               true
                    readonly property bool reversed:    false
                }
            }
        }
    }
}
