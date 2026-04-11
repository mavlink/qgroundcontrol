/****************************************************************************
 *
 * USV Integrated Instrument Panel
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

Item {
    id: control

    implicitWidth: _panelWidth
    implicitHeight: panelContent.implicitHeight + _m * 2

    property real attitudeSize: ScreenTools.defaultFontPixelWidth * 2
    property real attitudeSpacing: attitudeSize / 2
    property real extraInset: attitudeSize + attitudeSpacing
    property real extraValuesWidth: _compassSize / 2
    property real defaultCompassRadius: (mainWindow.width * 0.12) / 2
    property real maxCompassRadius: ScreenTools.defaultFontPixelHeight * 5 / 2
    property real compassRadius: Math.min(defaultCompassRadius, maxCompassRadius)
    property real compassBorder: ScreenTools.defaultFontPixelHeight / 2
    property var vehicle: globals.activeVehicle
    property bool usedByMultipleVehicleList: false

    property real _m: ScreenTools.defaultFontPixelWidth
    property real _panelWidth: ScreenTools.defaultFontPixelWidth * 25
    property real _compassSize: ScreenTools.defaultFontPixelHeight * 5.5

    property real roll: (vehicle && vehicle.roll && vehicle.roll.rawValue !== undefined) ? vehicle.roll.rawValue : 0
    property real pitch: (vehicle && vehicle.pitch && vehicle.pitch.rawValue !== undefined) ? vehicle.pitch.rawValue : 0

    property real rollWarnTh: 15.0
    property real pitchWarnTh: 10.0
    property real rollCritTh: 25.0
    property real pitchCritTh: 20.0

    property bool isRollWarn: Math.abs(roll) > rollWarnTh
    property bool isPitchWarn: Math.abs(pitch) > pitchWarnTh
    property bool isRollCrit: Math.abs(roll) > rollCritTh
    property bool isPitchCrit: Math.abs(pitch) > pitchCritTh
    property bool isAttWarn: isRollWarn || isPitchWarn
    property bool isAttCrit: isRollCrit || isPitchCrit

    function _metricText(fact, suffix, digits) {
        return (vehicle && fact && fact.rawValue !== undefined)
                ? Number(fact.rawValue).toFixed(digits) + suffix
                : "---"
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    Rectangle {
        anchors.fill: parent
        radius: _m * 0.7
        color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.9)
        border.width: 1
        border.color: isAttCrit
                      ? Qt.rgba(qgcPal.colorRed.r, qgcPal.colorRed.g, qgcPal.colorRed.b, 0.75)
                      : Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.5)
    }

    ColumnLayout {
        id: panelContent
        anchors.fill: parent
        anchors.margins: _m
        spacing: _m * 0.6

        RowLayout {
            Layout.fillWidth: true
            spacing: _m * 0.5

            QGCLabel {
                text: qsTr("航行")
                font.bold: true
                Layout.fillWidth: true
            }

            Rectangle {
                radius: _m * 0.4
                color: isAttCrit
                       ? qgcPal.colorRed
                       : (isAttWarn ? qgcPal.colorOrange : qgcPal.colorGreen)
                width: badgeLabel.implicitWidth + _m
                height: badgeLabel.implicitHeight + _m * 0.35

                QGCLabel {
                    id: badgeLabel
                    anchors.centerIn: parent
                    text: isAttCrit ? qsTr("危险") : (isAttWarn ? qsTr("关注") : qsTr("稳定"))
                    color: "white"
                    font.pointSize: ScreenTools.smallFontPointSize
                    font.bold: true
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: _m * 0.7

            ColumnLayout {
                Layout.fillWidth: true
                spacing: _m * 0.25

                QGCLabel {
                    text: _metricText(vehicle ? vehicle.groundSpeed : null, " m/s", 1)
                    font.pointSize: ScreenTools.mediumFontPointSize
                    font.bold: true
                    color: qgcPal.text
                }

                QGCLabel {
                    text: qsTr("地速")
                    font.pointSize: ScreenTools.smallFontPointSize
                    opacity: 0.55
                }

                GridLayout {
                    columns: 2
                    columnSpacing: _m * 0.5
                    rowSpacing: _m * 0.15

                    QGCLabel { text: qsTr("航向"); opacity: 0.55; font.pointSize: ScreenTools.smallFontPointSize }
                    QGCLabel { text: _metricText(vehicle ? vehicle.heading : null, "\u00B0", 0); font.bold: true; font.pointSize: ScreenTools.smallFontPointSize }
                    QGCLabel { text: qsTr("距 Home"); opacity: 0.55; font.pointSize: ScreenTools.smallFontPointSize }
                    QGCLabel { text: _metricText(vehicle ? vehicle.distanceToHome : null, " m", 0); font.bold: true; font.pointSize: ScreenTools.smallFontPointSize }
                }
            }

            Rectangle {
                width: _compassSize + _m * 1.5
                height: width
                radius: width / 2
                color: Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.16)

                QGCCompassWidget {
                    anchors.centerIn: parent
                    size: _compassSize
                    vehicle: control.vehicle
                    usedByMultipleVehicleList: control.usedByMultipleVehicleList
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: attitudeRow.implicitHeight + _m * 0.55
            radius: _m * 0.4
            color: isAttCrit
                   ? Qt.rgba(qgcPal.colorRed.r, qgcPal.colorRed.g, qgcPal.colorRed.b, 0.92)
                   : (isAttWarn
                      ? Qt.rgba(qgcPal.colorOrange.r, qgcPal.colorOrange.g, qgcPal.colorOrange.b, 0.2)
                      : Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.12))

            RowLayout {
                id: attitudeRow
                anchors.fill: parent
                anchors.margins: _m * 0.35
                spacing: _m * 0.6

                QGCLabel {
                    text: qsTr("R %1\u00B0").arg(Number(roll).toFixed(1))
                    font.pointSize: ScreenTools.smallFontPointSize
                    font.bold: true
                    color: isAttCrit ? "white" : (isRollWarn ? qgcPal.colorOrange : qgcPal.text)
                    Layout.fillWidth: true
                }

                QGCLabel {
                    text: qsTr("P %1\u00B0").arg(Number(pitch).toFixed(1))
                    font.pointSize: ScreenTools.smallFontPointSize
                    font.bold: true
                    color: isAttCrit ? "white" : (isPitchWarn ? qgcPal.colorOrange : qgcPal.text)
                    Layout.fillWidth: true
                }
            }
        }
    }
}
