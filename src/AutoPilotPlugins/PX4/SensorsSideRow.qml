import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

import "PX4SensorsOrientation.js" as Orientation

/// One side of an orientation calibration in the list beside the stage: a small picture of the vehicle on it, its name, and
/// where the calibration is with it.
Rectangle {
    id: control

    required property string side

    /// SensorsComponentController.SideCalState
    property int    calState:           0
    property bool   suggested:          false

    readonly property bool _done:   calState === 3
    readonly property bool _active: calState === 2

    Layout.fillWidth:   true
    implicitHeight:     rowLayout.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.6
    radius:             ScreenTools.defaultBorderRadius
    color:              _active ? qgcPal.accentSubtle : qgcPal.windowCard
    border.color:       _done ? qgcPal.colorGreen : (_active || suggested) ? qgcPal.accent : qgcPal.borderSubtle

    QGCPalette { id: qgcPal }

    RowLayout {
        id:                 rowLayout
        anchors.fill:       parent
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        spacing:            ScreenTools.defaultFontPixelWidth

        Image {
            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 3
            Layout.preferredWidth:  Layout.preferredHeight * 1.25
            source:                 Orientation.imageSource(control.side, false)
            fillMode:               Image.PreserveAspectFit
            opacity:                control._done ? 0.55 : 1
        }

        QGCLabel {
            Layout.fillWidth:   true
            text:               Orientation.sideName(control.side)
            font.bold:          true
        }

        QGCLabel {
            text:           control._done ? qsTr("Done") :
                                control._active ? qsTr("Detected") :
                                control.suggested ? qsTr("Suggested") : qsTr("Waiting")
            font.pointSize: ScreenTools.smallFontPointSize
            font.bold:      control._done || control._active || control.suggested
            color:          control._done ? qgcPal.successText :
                                (control._active || control.suggested) ? qgcPal.accent : qgcPal.textSecondary
        }

        Rectangle {
            implicitWidth:  ScreenTools.defaultFontPixelHeight * 1.1
            implicitHeight: implicitWidth
            radius:         implicitWidth / 2
            color:          control._done ? qgcPal.colorGreen : "transparent"
            border.color:   control._done ? qgcPal.colorGreen : (control._active || control.suggested) ? qgcPal.accent : qgcPal.borderSubtle
            border.width:   control._active ? 2 : 1

            QGCLabel {
                anchors.centerIn:   parent
                text:               "✓"
                font.pointSize:     ScreenTools.smallFontPointSize
                font.bold:          true
                color:              qgcPal.window
                visible:            control._done
            }
        }
    }
}
