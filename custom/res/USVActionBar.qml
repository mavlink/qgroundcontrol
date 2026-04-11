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
    
    // MAVLink 指令 ID
    readonly property int _cmdStart:     31010
    readonly property int _cmdStop:      31011
    readonly property int _cmdPause:     31012
    readonly property int _cmdResume:    31013
    readonly property int _cmdCalibrate: 31014
    readonly property int _payloadCompId: 191

    property real _m: ScreenTools.defaultFontPixelWidth

    width: actionRow.implicitWidth + _m * 3
    height: ScreenTools.defaultFontPixelHeight * USVLayout.Tokens.touch.minHeight + _m
    radius: height / 2
    color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, USVLayout.Tokens.opacity.panel)
    border.width: 1
    border.color: Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.45)
    
    // 隐藏逻辑：如果设备未连接，可以整体淡出
    opacity: vehicle ? 1.0 : 0.0
    visible: opacity > 0
    Behavior on opacity { NumberAnimation { duration: 300 } }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    function _send(cmdId) {
        if (vehicle) vehicle.sendCommand(_payloadCompId, cmdId, false)
    }

    property bool _isWorking: payloadStatus === USVLayout.StatusSampling || payloadStatus === USVLayout.StatusDetecting || payloadStatus === USVLayout.StatusCalibrating

    RowLayout {
        id: actionRow
        anchors.centerIn: parent
        spacing: _m * USVLayout.Tokens.spacing.md

        Repeater {
            model: [
                {
                    text: qsTr("采样"),
                    cmd: _cmdStart,
                    en: vehicle && payloadStatus === USVLayout.StatusIdle,
                    warn: false
                },
                {
                    text: qsTr("暂停"),
                    cmd: _cmdPause,
                    en: vehicle && payloadStatus === USVLayout.StatusSampling,
                    warn: false
                },
                {
                    text: qsTr("恢复"),
                    cmd: _cmdResume,
                    en: vehicle && (payloadStatus === USVLayout.StatusSampling || payloadStatus === USVLayout.StatusIdle),
                    warn: false
                },
                {
                    text: qsTr("校准"),
                    cmd: _cmdCalibrate,
                    en: vehicle && payloadStatus === USVLayout.StatusIdle,
                    warn: false
                },
                {
                    text: qsTr("停止"),
                    cmd: _cmdStop,
                    en: vehicle && _isWorking,
                    warn: true
                }
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
                    onClicked: {
                        if (modelData.warn) {
                            // 实际项目中可以添加 Confirmation Dialog，这里简化为直接发送
                            _send(modelData.cmd)
                        } else {
                            _send(modelData.cmd)
                        }
                    }
                }
            }
        }
    }
}
