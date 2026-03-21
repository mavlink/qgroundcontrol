import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

QGCPopupDialog {
    title:   qsTr("Altitude Frame")
    buttons: Dialog.Close

    property var rgRemoveModes
    property var updateAltFrameFn
    property var currentAltFrame

    Component.onCompleted: {
        // Populate model dynamically since ListElement cannot use script expressions
        buttonModel.append({ modeName: QGroundControl.altitudeFrameShortDescription(QGroundControl.AltitudeFrameRelative),
                             help: qsTr("Altitude above home position"),
                             modeValue: QGroundControl.AltitudeFrameRelative })
        buttonModel.append({ modeName: QGroundControl.altitudeFrameShortDescription(QGroundControl.AltitudeFrameAbsolute),
                             help: qsTr("Altitude above mean sea level"),
                             modeValue: QGroundControl.AltitudeFrameAbsolute })
        buttonModel.append({ modeName: QGroundControl.altitudeFrameShortDescription(QGroundControl.AltitudeFrameTerrain),
                             help: qsTr("Altitude above terrain at waypoint using MAVLink terrain protocol"),
                             modeValue: QGroundControl.AltitudeFrameTerrain })
        buttonModel.append({ modeName: QGroundControl.altitudeFrameShortDescription(QGroundControl.AltitudeFrameCalcAboveTerrain),
                             help: qsTr("Altitudes are terrain-relative; converting to AMSL before upload"),
                             modeValue: QGroundControl.AltitudeFrameCalcAboveTerrain })
        buttonModel.append({ modeName: QGroundControl.altitudeFrameShortDescription(QGroundControl.AltitudeFrameMixed),
                             help: qsTr("Each waypoint specifies its own altitude frame"),
                             modeValue: QGroundControl.AltitudeFrameMixed })

        // Check for custom build override on AMSL usage
        if (!QGroundControl.corePlugin.options.showMissionAbsoluteAltitude && currentAltFrame != QGroundControl.AltitudeFrameAbsolute) {
            rgRemoveModes.push(QGroundControl.AltitudeFrameAbsolute)
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
    }

    Column {
        spacing: ScreenTools.defaultFontPixelWidth

        QGCLabel {
            text: qsTr("Altitude frame for mission items")
            font.pointSize: ScreenTools.smallFontPointSize
        }

        Repeater {
            id: buttonRepeater

            Button {
                hoverEnabled:   true
                checked:        modeValue == currentAltFrame

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
                    updateAltFrameFn(modeValue)
                    close()
                }
            }
        }
    }
}
