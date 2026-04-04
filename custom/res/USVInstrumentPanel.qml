/****************************************************************************
 *
 * USV Integrated Instrument Panel
 * 替换默认的 IntegratedCompassAttitude.qml
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

Item {
    id:             control
    implicitWidth:  _panelWidth
    implicitHeight: panelCol.height + _m * 2

    // ========== 兼容接口属性 ==========
    property real attitudeSize:              ScreenTools.defaultFontPixelWidth * 2
    property real attitudeSpacing:           attitudeSize / 2
    property real extraInset:                attitudeSize + attitudeSpacing
    property real extraValuesWidth:          _compassSize / 2
    property real defaultCompassRadius:      (mainWindow.width * 0.15) / 2
    property real maxCompassRadius:          ScreenTools.defaultFontPixelHeight * 7 / 2
    property real compassRadius:             Math.min(defaultCompassRadius, maxCompassRadius)
    property real compassBorder:             ScreenTools.defaultFontPixelHeight / 2
    property var  vehicle:                   globals.activeVehicle
    property bool usedByMultipleVehicleList: false

    // ========== 内部属性 ==========
    property real _m:            ScreenTools.defaultFontPixelWidth
    property real _compassSize:  Math.min(ScreenTools.defaultFontPixelHeight * 7, _panelWidth * 0.65)
    property real _panelWidth:   ScreenTools.defaultFontPixelWidth * 18

    property real roll:   (vehicle && vehicle.roll  && vehicle.roll.rawValue  !== undefined) ? vehicle.roll.rawValue  : 0
    property real pitch:  (vehicle && vehicle.pitch && vehicle.pitch.rawValue !== undefined) ? vehicle.pitch.rawValue : 0

    property real rollWarnTh:   15.0
    property real pitchWarnTh:  10.0
    property real rollCritTh:   25.0
    property real pitchCritTh:  20.0

    property bool isRollWarn:     Math.abs(roll)  > rollWarnTh
    property bool isPitchWarn:    Math.abs(pitch) > pitchWarnTh
    property bool isRollCrit:     Math.abs(roll)  > rollCritTh
    property bool isPitchCrit:    Math.abs(pitch) > pitchCritTh
    property bool isAttWarn:      isRollWarn  || isPitchWarn
    property bool isAttCrit:      isRollCrit  || isPitchCrit

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    Rectangle {
        anchors.fill: parent
        color:   Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.88)
        radius:  _m * 0.5
    }

    ColumnLayout {
        id: panelCol
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: _m
        width: parent.width - _m * 2
        spacing: _m * 0.8

        // ===== 罗盘 =====
        QGCCompassWidget {
            Layout.alignment: Qt.AlignHCenter
            size: _compassSize
            vehicle: control.vehicle
        }

        // ===== 航行数据 =====
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            rowSpacing: 1
            columnSpacing: _m

            QGCLabel { text: "航速"; opacity: 0.6; font.pointSize: ScreenTools.smallFontPointSize; Layout.alignment: Qt.AlignRight }
            QGCLabel {
                text: (vehicle && vehicle.groundSpeed && vehicle.groundSpeed.rawValue !== undefined)
                      ? Number(vehicle.groundSpeed.rawValue).toFixed(1) + " m/s" : "---"
                font.bold: true; font.pointSize: ScreenTools.smallFontPointSize
            }

            QGCLabel { text: "航向"; opacity: 0.6; font.pointSize: ScreenTools.smallFontPointSize; Layout.alignment: Qt.AlignRight }
            QGCLabel {
                text: (vehicle && vehicle.heading && vehicle.heading.rawValue !== undefined)
                      ? Number(vehicle.heading.rawValue).toFixed(0) + "°" : "---"
                font.bold: true; font.pointSize: ScreenTools.smallFontPointSize
            }

            QGCLabel { text: "距Home"; opacity: 0.6; font.pointSize: ScreenTools.smallFontPointSize; Layout.alignment: Qt.AlignRight }
            QGCLabel {
                text: (vehicle && vehicle.distanceToHome && vehicle.distanceToHome.rawValue !== undefined)
                      ? Number(vehicle.distanceToHome.rawValue).toFixed(0) + " m" : "---"
                font.bold: true; font.pointSize: ScreenTools.smallFontPointSize
            }
        }

        // ===== 姿态状态条 =====
        Rectangle {
            Layout.fillWidth: true
            height: attRow.height + _m * 0.8
            radius: _m * 0.3
            color: isAttCrit ? qgcPal.colorRed :
                   (isAttWarn ? qgcPal.colorOrange : "transparent")

            RowLayout {
                id: attRow
                anchors.centerIn: parent
                width: parent.width - _m
                spacing: _m

                QGCLabel {
                    text: isAttCrit ? "⚠" : (isAttWarn ? "⚠" : "✓")
                    font.pointSize: ScreenTools.smallFontPointSize
                    color: isAttCrit ? "white" : (isAttWarn ? "white" : qgcPal.colorGreen)
                    font.bold: true
                }

                QGCLabel {
                    text: "R " + Number(roll).toFixed(1) + "°"
                    font.pointSize: ScreenTools.smallFontPointSize
                    font.bold: true
                    color: isRollCrit ? "white" : (isRollWarn ? qgcPal.colorOrange : qgcPal.text)
                    Layout.fillWidth: true
                }

                QGCLabel {
                    text: "P " + Number(pitch).toFixed(1) + "°"
                    font.pointSize: ScreenTools.smallFontPointSize
                    font.bold: true
                    color: isPitchCrit ? "white" : (isPitchWarn ? qgcPal.colorOrange : qgcPal.text)
                    Layout.fillWidth: true
                }
            }
        }
    }
}
