/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs  1.3
import QtQuick.Layouts  1.12

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

QGCPopupDialog {
    title:   qsTr("Select Altitude Mode")
    buttons: StandardButton.Close

    Component.onCompleted: {
        // Check for custom build override on AMSL usage
        if (!QGroundControl.corePlugin.options.showMissionAbsoluteAltitude && dialogProperties.currentAltMode != QGroundControl.AltitudeModeAbsolute) {
            dialogProperties.rgRemoveModes.push(QGroundControl.AltitudeModeAbsolute)
        }

        // Remove modes specified by consumer
        for (var i=0; i<dialogProperties.rgRemoveModes.length; i++) {
            for (var j=0; j<buttonModel.count; j++) {
                if (buttonModel.get(j).modeValue == dialogProperties.rgRemoveModes[i]) {
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
            modeName:   qsTr("Relative To Launch")
            help:       qsTr("Specified altitudes are relative to launch position height.")
            modeValue:  QGroundControl.AltitudeModeRelative
        }
        ListElement {
            modeName:   qsTr("AMSL")
            help:       qsTr("Specified altitudes are Above Mean Sea Level.")
            modeValue:  QGroundControl.AltitudeModeAbsolute
        }
        ListElement {
            modeName:   qsTr("Calculated Above Terrain")
            help:       qsTr("Specified altitudes are distance above terrain. Actual altitudes sent to vehicle are calculated from terrain data and sent as AMSL values.")
            modeValue:  QGroundControl.AltitudeModeCalcAboveTerrain
        }
        ListElement {
            modeName:   qsTr("Terrain Frame")
            help:       qsTr("Specified altitudes are distance above terrain. The actual altitude flown is controlled by the vehicle either from terrain height maps being sent to vehicle or a distance sensor.")
            modeValue:  QGroundControl.AltitudeModeTerrainFrame
        }
        ListElement {
            modeName:   qsTr("Mixed Modes")
            help:       qsTr("The altitude mode can differ for each individual item.")
            modeValue:  QGroundControl.AltitudeModeMixed
        }
    }

    Column {
        spacing: ScreenTools.defaultFontPixelWidth

        Repeater {
            id: buttonRepeater

            Button {
                hoverEnabled:   true
                checked:        modeValue == dialogProperties.currentAltMode

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
                    dialogProperties.updateAltModeFn(modeValue)
                    hideDialog()
                }
            }
        }
    }
}
