/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick

import QGroundControl
import QGroundControl.Controls

ListView {
    id: profilesList

    property var profilesArray: QGroundControl.settingsManager.vehicleProfilesSettings
    readonly property var profilesModel: profilesArray ? profilesArray.groups : null

    model: profilesModel ? profilesModel : 0
    clip: true
    spacing: ScreenTools.defaultFontPixelHeight / 2
    boundsBehavior: Flickable.StopAtBounds
    interactive: true

    delegate: Rectangle {
        width: profilesList.width
        radius: ScreenTools.defaultFontPixelHeight / 3
        color: QGroundControl.globalPalette.windowShadeDark
        border.width: 1
        border.color: QGroundControl.globalPalette.windowShade

        readonly property real _contentMargin: ScreenTools.defaultFontPixelHeight
        property var profile: object

        implicitHeight: contentColumn.implicitHeight + (_contentMargin * 2)
        height: implicitHeight

        Column {
            id: contentColumn
            x: _contentMargin
            y: _contentMargin
            width: parent.width - (_contentMargin * 2)
            spacing: ScreenTools.defaultFontPixelHeight / 3

            QGCLabel {
                text: profile ? profile.vehicleName.valueString : ""
                font.bold: true
                elide: Text.ElideRight
                width: parent.width
            }

            QGCLabel {
                text: profile ? qsTr("MAVLink System ID: %1").arg(profile.mavlinkId.valueString) : ""
                color: QGroundControl.globalPalette.text
                width: parent.width
                wrapMode: Text.WordWrap
            }
        }
    }
}
