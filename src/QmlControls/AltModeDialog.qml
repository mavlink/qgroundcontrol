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

QGCPopupDialog {
    title:   qsTr("Global Altitude Mode")
    buttons: Dialog.Close

    property var rgRemoveModes
    property var updateAltModeFn
    property var currentAltMode

    Component.onCompleted: {
        // Check for custom build override on AMSL usage
        if (!QGroundControl.corePlugin.options.showMissionAbsoluteAltitude && currentAltMode != QGroundControl.AltitudeModeAbsolute) {
            rgRemoveModes.push(QGroundControl.AltitudeModeAbsolute)
        }

        // Remove modes specified by consumer
        for (var i=0; i<rgRemoveModes.length; i++) {
            for (var j=0; j<buttonModel.count; j++) {
                if (buttonModel.get(j).modeValue == rgRemoveModes[i]) {
                    buttonModel.remove(j)
                    break
                }
            }
        }

        buttonRepeater.model = buttonModel
    }

    ListModel {
        id: buttonModel

        ListElement {
            modeName:   qsTr("Relative To Launch (Rel)")
            help:       qsTr("Altitudes are relative to the launch point.")
            modeValue:  QGroundControl.AltitudeModeRelative
        }
        ListElement {
            modeName:   qsTr("AMSL")
            help:       qsTr("Altitudes are absolute above mean sea level.")
            modeValue:  QGroundControl.AltitudeModeAbsolute
        }
        ListElement {
            modeName:   qsTr("Calculated Above Terrain (TerrC)")
            help:       qsTr("Altitudes are terrain-relative; converting to AMSL before upload.")
            modeValue:  QGroundControl.AltitudeModeCalcAboveTerrain
        }
        ListElement {
            modeName:   qsTr("Above Terrain (Terr)")
            help:       qsTr("Vehicle maintains a terrain-relative height using onboard data.")
            modeValue:  QGroundControl.AltitudeModeTerrainFrame
        }
        ListElement {
            modeName:   qsTr("Mixed Modes (Mix)")
            help:       qsTr("Each mission item can choose its own altitude mode.")
            modeValue:  QGroundControl.AltitudeModeMixed
        }
    }

    Column {
        spacing: ScreenTools.defaultFontPixelWidth

        QGCLabel {
            text: qsTr("Altitude mode for all mission items.")
            font.pointSize: ScreenTools.smallFontPointSize
        }

        Repeater {
            id: buttonRepeater

            Button {
                hoverEnabled:   true
                checked:        modeValue == currentAltMode

                background: Rectangle {
                    radius: ScreenTools.defaultFontPixelHeight / 2
                    color:  pressed | hovered | checked ? QGroundControl.globalPalette.buttonHighlight: QGroundControl.globalPalette.button
                }

                contentItem: Column {
                    spacing: 0

                    QGCLabel {
                        id:     modeNameLabel
                        text:   modeName
                        color:  pressed | hovered | checked ? QGroundControl.globalPalette.buttonHighlightText: QGroundControl.globalPalette.buttonText
                    }

                    QGCLabel {
                        width:              ScreenTools.defaultFontPixelWidth * 40
                        text:               help
                        wrapMode:           Label.WordWrap
                        font.pointSize:     ScreenTools.smallFontPointSize
                        color:              modeNameLabel.color
                    }
                }

                onClicked: {
                    updateAltModeFn(modeValue)
                    close()
                }
            }
        }
    }
}
