/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

pragma ComponentBehavior: Bound

import QtQuick

import QGroundControl
import QGroundControl.Controls

Flow {
    id: profilesList

    property var _profileSettings: QGroundControl.settingsManager.vehicleProfilesSettings

    readonly property int count: _profileSettings.groups.count

    clip: true
    flow: Flow.LeftToRight
    spacing: ScreenTools.defaultFontPixelHeight / 2

    Repeater {
        id: profileRepeater
        model: _profileSettings.groups

        Rectangle {
            id: profileItem

            width: profilesList.width
            radius: ScreenTools.defaultFontPixelHeight / 3
            color: QGroundControl.globalPalette.windowShadeDark
            border.width: 1
            border.color: QGroundControl.globalPalette.windowShade

            readonly property real _contentMargin: ScreenTools.defaultFontPixelHeight

            implicitHeight: contentColumn.implicitHeight + (_contentMargin * 2)
            height: implicitHeight

            Column {
                id: contentColumn
                x: profileItem._contentMargin
                y: profileItem._contentMargin
                width: profileItem.width - (profileItem._contentMargin * 2)
                spacing: ScreenTools.defaultFontPixelHeight / 3

                QGCLabel {
                    text: object.vehicleName.valueString
                    font.bold: true
                    elide: Text.ElideRight
                    width: parent.width
                }

                QGCLabel {
                    text: qsTr("MAVLink System ID: %1").arg(object.mavlinkId.valueString)
                    color: QGroundControl.globalPalette.text
                    width: parent.width
                    wrapMode: Text.WordWrap
                }
            }
        }
    }
}
