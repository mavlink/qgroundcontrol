import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

import "PX4SensorsOrientation.js" as Orientation

/// The large picture of the vehicle on the side to calibrate next, or on the side it is on now.
Rectangle {
    id: control

    property string side:       "level"
    /// The vehicle is being turned on this side, so the picture shows the turn
    property bool   rotating:   false
    /// What to do with the vehicle now, shown with a busy indicator. Empty when there is nothing to do.
    property string instruction: ""
    property bool   active:     false
    property string tag:        ""
    property string caption:    ""
    /// "ok" or "error" draws that mark over the picture
    property string mark:       ""
    /// "multiRotor", "fixedWing", "vtol", "rover" or anything else for a flat board
    property string vehicleKind: ""

    objectName:         "sensorsCal_stage"
    implicitWidth:      ScreenTools.defaultFontPixelWidth * 50
    implicitHeight:     ScreenTools.defaultFontPixelHeight * 19
    radius:             ScreenTools.defaultBorderRadius * 2
    color:              active ? qgcPal.accentSubtle : qgcPal.windowCard
    border.color:       mark === "ok" ? qgcPal.colorGreen : mark === "error" ? qgcPal.colorRed : active ? qgcPal.accent : qgcPal.borderSubtle

    QGCPalette { id: qgcPal }

    QGCChip {
        anchors.left:       parent.left
        anchors.top:        parent.top
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        text:               control.tag
        textColor:          qgcPal.accent
        visible:            control.tag !== ""
    }

    BusyIndicator {
        anchors.right:      parent.right
        anchors.top:        parent.top
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        running:            visible
        visible:            control.instruction !== ""
        implicitWidth:      ScreenTools.defaultFontPixelHeight * 1.5
        implicitHeight:     implicitWidth
    }

    ColumnLayout {
        anchors.fill:       parent
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        spacing:            ScreenTools.defaultFontPixelHeight * 0.25

        // Qt Quick 3D needs a GPU renderer. Without one the still pictures are shown instead.
        Loader {
            id:                     picture
            Layout.fillWidth:       true
            Layout.fillHeight:      true
            opacity:                control.mark === "" ? 1 : 0.3
            sourceComponent: {
                switch (GraphicsInfo.api) {
                case GraphicsInfo.Unknown:
                    return null
                case GraphicsInfo.Software:
                    return stillPicture
                }
                return vehicleView
            }
        }

        Component {
            id: vehicleView

            SensorsVehicleView {
                side:           control.side
                rotating:       control.rotating
                vehicleKind:    control.vehicleKind
            }
        }

        Component {
            id: stillPicture

            Image {
                source:     Orientation.imageSource(control.side, control.rotating)
                fillMode:   Image.PreserveAspectFit
            }
        }

        QGCLabel {
            Layout.alignment:   Qt.AlignHCenter
            text:               control.instruction !== "" ? qsTr("%1 - %2").arg(control.caption).arg(control.instruction) : control.caption
            font.pointSize:     ScreenTools.mediumFontPointSize
            font.bold:          true
        }
    }

    Rectangle {
        anchors.centerIn:   parent
        implicitWidth:      ScreenTools.defaultFontPixelHeight * 5
        implicitHeight:     implicitWidth
        radius:             implicitWidth / 2
        color:              control.mark === "ok" ? qgcPal.colorGreen : qgcPal.colorRed
        visible:            control.mark !== ""

        QGCLabel {
            anchors.centerIn:   parent
            text:               control.mark === "ok" ? "✓" : "!"
            font.pointSize:     ScreenTools.largeFontPointSize
            font.bold:          true
            color:              qgcPal.window
        }
    }
}
