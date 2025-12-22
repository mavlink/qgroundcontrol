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
    title:   qsTr("Altitude Mode")
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
            modeName:   qsTr("Relative")
            help:       qsTr("Altitude above home position")
            modeValue:  QGroundControl.AltitudeModeRelative
        }
        ListElement {
            modeName:   qsTr("Absolute")
            help:       qsTr("Altitude above mean sea level (AMSL)")
            modeValue:  QGroundControl.AltitudeModeAbsolute
        }
        ListElement {
            modeName:   qsTr("Terrain")
            help:       qsTr("Altitude above terrain at waypoint")
            modeValue:  QGroundControl.AltitudeModeTerrainFrame
        }
        ListElement {
            modeName:   qsTr("Terrain Calculated")
            help:       qsTr("Altitudes are terrain-relative; converting to AMSL before upload")
            modeValue:  QGroundControl.AltitudeModeCalcAboveTerrain
        }
        ListElement {
            modeName:   qsTr("Waypoint Defined")
            help:       qsTr("Each waypoint specifies its own altitude mode")
            modeValue:  QGroundControl.AltitudeModeMixed
        }
    }

    Column {
        spacing: ScreenTools.defaultFontPixelWidth

        QGCLabel {
            text: qsTr("Altitude mode for mission items")
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
