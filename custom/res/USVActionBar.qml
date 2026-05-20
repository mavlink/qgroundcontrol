import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

import "USVFlyViewLayout.js" as USVLayout

Rectangle {
    id: root

    property var vehicle: null
    property int payloadStatus: USVLayout.StatusIdle

    readonly property int _cmdStart: 31010
    readonly property int _cmdStop: 31011
    readonly property int _cmdStartSurvey: 31015
    readonly property int _cmdStopSurvey: 31016
    readonly property int _cmdSetBaseline: 31017
    readonly property int _cmdSpectroStart: 31018
    readonly property int _cmdSpectroStop: 31019
    readonly property int _payloadCompId: 191

    property real _m: ScreenTools.defaultFontPixelWidth
    property bool _isWorking: payloadStatus === USVLayout.StatusSampling
                              || payloadStatus === USVLayout.StatusDetecting
                              || payloadStatus === USVLayout.StatusCalibrating
                              || payloadStatus === USVLayout.StatusSurveying

    width: actionRow.implicitWidth + _m * 3
    height: ScreenTools.defaultFontPixelHeight * USVLayout.Tokens.touch.minHeight + _m
    radius: height / 2
    color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, USVLayout.Tokens.opacity.panel)
    border.width: 1
    border.color: Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.45)
    opacity: vehicle ? 1.0 : 0.0
    visible: opacity > 0

    Behavior on opacity { NumberAnimation { duration: 300 } }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    function _send(cmdId, param1) {
        if (vehicle) {
            vehicle.sendCommand(_payloadCompId, cmdId, false, param1 || 0)
        }
    }

    RowLayout {
        id: actionRow
        anchors.centerIn: parent
        spacing: _m * USVLayout.Tokens.spacing.md

        Repeater {
            model: [
                { text: qsTr("启动信号"), cmd: _cmdSpectroStart, param1: 0, en: vehicle && payloadStatus !== USVLayout.StatusFault, warn: false },
                { text: qsTr("设基线"), cmd: _cmdSetBaseline, param1: 0, en: vehicle && payloadStatus !== USVLayout.StatusFault, warn: false },
                { text: qsTr("点采样"), cmd: _cmdStart, param1: 0, en: vehicle && payloadStatus === USVLayout.StatusIdle, warn: false },
                { text: qsTr("走航"), cmd: _cmdStartSurvey, param1: 5, en: vehicle && payloadStatus !== USVLayout.StatusFault && payloadStatus !== USVLayout.StatusSurveying, warn: false },
                { text: qsTr("停止检测"), cmd: _cmdStop, param1: 0, en: vehicle && _isWorking, warn: true },
                { text: qsTr("停止走航"), cmd: _cmdStopSurvey, param1: 0, en: vehicle && payloadStatus === USVLayout.StatusSurveying, warn: true },
                { text: qsTr("停信号"), cmd: _cmdSpectroStop, param1: 0, en: vehicle && payloadStatus !== USVLayout.StatusFault, warn: true }
            ]

            delegate: Rectangle {
                id: btnRect
                Layout.fillHeight: true
                height: ScreenTools.defaultFontPixelHeight * 1.8
                width: textLabel.implicitWidth + _m * 4
                radius: height / 2

                property bool isHovered: btnMouse.containsMouse
                property bool isPressed: btnMouse.pressed
                property color baseColor: modelData.warn
                    ? Qt.rgba(qgcPal.colorRed.r, qgcPal.colorRed.g, qgcPal.colorRed.b, 0.85)
                    : Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.7)

                color: !modelData.en
                       ? Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.3)
                       : (isPressed ? Qt.darker(baseColor, 1.2) : (isHovered ? Qt.lighter(baseColor, 1.1) : baseColor))

                scale: isPressed ? 0.95 : 1.0
                Behavior on scale { NumberAnimation { duration: 100 } }
                Behavior on color { ColorAnimation { duration: 150 } }

                QGCLabel {
                    id: textLabel
                    anchors.centerIn: parent
                    text: modelData.text
                    color: modelData.warn && modelData.en ? "white" : qgcPal.text
                    opacity: modelData.en ? 1.0 : USVLayout.Tokens.opacity.disabled
                    font.bold: true
                    font.pointSize: ScreenTools.defaultFontPointSize
                }

                MouseArea {
                    id: btnMouse
                    anchors.fill: parent
                    enabled: modelData.en
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: _send(modelData.cmd, modelData.param1)
                }
            }
        }
    }
}
