/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

/// Generic version of Channel Monitor which should work with both RC Transmitters and Joysticks
/// Used to display raw channel values
GridLayout {
    required property int channelCount
    required property int channelValueMin
    required property int channelValueMax

    property bool twoColumn: false

    /// Should be called by consumers whenever a raw channel value changes
    function rawChannelValueChanged(channel, channelValue) {
        if (channelMonitorRepeater.itemAt(channel)) {
            let channelValueDisplayItem = channelMonitorRepeater.itemAt(channel)._channelValueDisplayLoader.item
            let filteredChannelValue = Math.min(Math.max(channelValue, channelValueMin), channelValueMax)
            channelValueDisplayItem.channelValue = filteredChannelValue
        }
    }

    id: control
    columns: control.twoColumn ? 2 : 1
    columnSpacing: ScreenTools.defaultFontPixelWidth / 2
    rowSpacing: 0

    QGCPalette { id: qgcPal; colorGroupEnabled: control.enabled }

    QGCLabel {
        Layout.columnSpan: parent.columns
        text: qsTr("Raw Channel Monitor")
    }

    Repeater {
        id: channelMonitorRepeater
        model: channelCount

        RowLayout {
            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelWidth / 2

            property var _channelValueDisplayLoader: channelValueDisplayLoader

            QGCLabel {
                text: index + 1
            }

            Loader {
                id: channelValueDisplayLoader
                Layout.fillWidth: true
                sourceComponent: RemoteControlChannelValueDisplay {
                    mode: RemoteControlChannelValueDisplay.RawValue
                    channelValueMin: control.channelValueMin
                    channelValueMax: control.channelValueMax
                }
            }
        }
    }
}
