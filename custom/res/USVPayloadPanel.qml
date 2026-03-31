import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

Rectangle {
    id:         root
    width:      ScreenTools.defaultFontPixelWidth * 32
    height:     contentCol.height + _margins * 2
    color:      qgcPal.window
    opacity:    0.95
    radius:     ScreenTools.defaultFontPixelWidth
    border.color: qgcPal.windowShade
    border.width: 1

    property var vehicle: null

    // 载荷状态枚举
    readonly property int _stIdle:       0
    readonly property int _stSampling:   1
    readonly property int _stDetecting:  2
    readonly property int _stFault:      3
    readonly property int _stCalibrating: 4

    // MAVLink 指令 ID
    readonly property int _cmdStart:     31010
    readonly property int _cmdStop:      31011
    readonly property int _cmdPause:     31012
    readonly property int _cmdResume:    31013
    readonly property int _cmdCalibrate: 31014

    // 当前载荷状态（绑定 Fact，确保 valueChanged 能驱动 UI）
    property var payloadStatusFact: {
        if (!vehicle) return null
        try { return vehicle.getFact("usvPayload.status") } catch(e) { return null }
    }

    property int payloadStatus: payloadStatusFact ? payloadStatusFact.value : _stIdle

    // 状态文本映射
    function statusText(st) {
        switch(st) {
        case _stIdle:        return "空闲"
        case _stSampling:    return "正在采样"
        case _stDetecting:   return "正在检测"
        case _stFault:       return "故障"
        case _stCalibrating: return "校准中"
        default:             return "未知"
        }
    }

    // 状态颜色
    function statusColor(st) {
        switch(st) {
        case _stIdle:        return qgcPal.text
        case _stSampling:    return qgcPal.colorGreen
        case _stDetecting:   return qgcPal.brandingBlue
        case _stFault:       return qgcPal.colorRed
        case _stCalibrating: return qgcPal.colorOrange
        default:             return qgcPal.text
        }
    }

    property bool _isWorking: payloadStatus === _stSampling ||
                              payloadStatus === _stDetecting ||
                              payloadStatus === _stCalibrating
    property real _margins:   ScreenTools.defaultFontPixelHeight

    // TODO: compId=1 targets autopilot. Ideally use compId=191 (ONBOARD_COMPUTER).
    // Current approach works because MAVROS forwards all from-messages.
    function _send(cmdId) {
        if (vehicle) vehicle.sendCommand(1, cmdId, true)
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    Column {
        id:                 contentCol
        anchors.margins:    _margins
        anchors.left:       parent.left
        anchors.right:      parent.right
        anchors.top:        parent.top
        spacing:            ScreenTools.defaultFontPixelHeight * 0.75

        // ===== 标题栏 =====
        RowLayout {
            anchors.left: parent.left; anchors.right: parent.right
            QGCLabel {
                text:               "水质监测载荷"
                font.pointSize:     ScreenTools.mediumFontPointSize
                font.bold:          true
                Layout.fillWidth:   true
            }
            // 状态指示灯
            Rectangle {
                width:  ScreenTools.defaultFontPixelHeight * 0.8
                height: width
                radius: width / 2
                color:  statusColor(payloadStatus)

                SequentialAnimation on opacity {
                    running: root._isWorking
                    loops:   Animation.Infinite
                    NumberAnimation { to: 0.3; duration: 600 }
                    NumberAnimation { to: 1.0; duration: 600 }
                }
            }
            QGCLabel {
                text:  statusText(payloadStatus)
                color: statusColor(payloadStatus)
                font.bold: true
            }
        }

        Rectangle { height: 1; anchors.left: parent.left; anchors.right: parent.right; color: qgcPal.windowShade }

        // ===== 链路超时警告 =====
        Rectangle {
            anchors.left: parent.left; anchors.right: parent.right
            height: _linkWarnLabel.height + ScreenTools.defaultFontPixelHeight * 0.5
            color: "#44FF0000"
            radius: ScreenTools.defaultFontPixelWidth * 0.5
            visible: {
                if (!vehicle) return false
                try { return vehicle.getFact("usvPayload.linkActive").value === 0
                      && vehicle.getFact("usvPayload.status") !== undefined }
                catch(e) { return false }
            }
            QGCLabel {
                id: _linkWarnLabel
                anchors.centerIn: parent
                text: "数据链路超时"
                color: qgcPal.colorRed
                font.bold: true
            }
        }

        // ===== 数据显示 =====
        GridLayout {
            columns: 2
            rowSpacing:    ScreenTools.defaultFontPixelHeight * 0.3
            columnSpacing: ScreenTools.defaultFontPixelWidth * 2
            anchors.left: parent.left; anchors.right: parent.right

            QGCLabel { text: "检测电压:" }
            QGCLabel {
                text: {
                    try { return vehicle ? vehicle.getFact("usvPayload.voltage").valueString + " V" : "N/A" }
                    catch(e) { return "N/A" }
                }
                color: qgcPal.brandingBlue
                font.bold: true
            }

            QGCLabel { text: "吸光度:" }
            QGCLabel {
                text: {
                    try { return vehicle ? vehicle.getFact("usvPayload.absorbance").valueString + " Abs" : "N/A" }
                    catch(e) { return "N/A" }
                }
                color: qgcPal.brandingBlue
                font.bold: true
            }
        }

        Rectangle { height: 1; anchors.left: parent.left; anchors.right: parent.right; color: qgcPal.windowShade }

        // ===== 泵组角度 =====
        QGCLabel { text: "泵组角度"; font.pointSize: ScreenTools.smallFontPointSize; color: qgcPal.textFieldText }

        RowLayout {
            anchors.left: parent.left; anchors.right: parent.right
            spacing: ScreenTools.defaultFontPixelWidth * 0.5

            Repeater {
                model: ["X", "Y", "Z", "A"]
                delegate: Rectangle {
                    Layout.fillWidth: true
                    height: _pumpCol.height + ScreenTools.defaultFontPixelHeight * 0.5
                    color:  qgcPal.windowShade
                    radius: ScreenTools.defaultFontPixelWidth * 0.5
                    Column {
                        id: _pumpCol
                        anchors.centerIn: parent
                        spacing: 2
                        QGCLabel {
                            text: modelData
                            anchors.horizontalCenter: parent.horizontalCenter
                            font.pointSize: ScreenTools.smallFontPointSize
                            color: qgcPal.textFieldText
                        }
                        QGCLabel {
                            text: {
                                var factName = "usvPayload.pump" + modelData
                                try { return vehicle ? vehicle.getFact(factName).value.toFixed(1) + "°" : "--" }
                                catch(e) { return "--" }
                            }
                            anchors.horizontalCenter: parent.horizontalCenter
                            font.bold: true
                        }
                    }
                }
            }
        }

        Rectangle { height: 1; anchors.left: parent.left; anchors.right: parent.right; color: qgcPal.windowShade }

        // ===== 控制按钮 =====
        GridLayout {
            columns: 2
            rowSpacing:    ScreenTools.defaultFontPixelHeight * 0.4
            columnSpacing: ScreenTools.defaultFontPixelWidth
            anchors.left: parent.left; anchors.right: parent.right

            // 开始采样
            QGCButton {
                Layout.fillWidth: true
                text:    "▶ 开始采样"
                enabled: vehicle && payloadStatus === _stIdle
                onClicked: _send(_cmdStart)
            }

            // 停止采样
            QGCButton {
                Layout.fillWidth: true
                text:    "■ 停止"
                enabled: vehicle && (payloadStatus === _stSampling || payloadStatus === _stDetecting)
                onClicked: _send(_cmdStop)
            }

            // 暂停
            QGCButton {
                Layout.fillWidth: true
                text:    "⏸ 暂停"
                enabled: vehicle && payloadStatus === _stSampling
                onClicked: _send(_cmdPause)
            }

            // 恢复
            QGCButton {
                Layout.fillWidth: true
                text:    "⏵ 恢复"
                enabled: vehicle && payloadStatus === _stIdle  // 暂停后回到 idle
                onClicked: _send(_cmdResume)
            }

            // 零点校准 - 占满一行
            QGCButton {
                Layout.fillWidth:  true
                Layout.columnSpan: 2
                text:    "⚙ 零点校准"
                enabled: vehicle && payloadStatus === _stIdle
                onClicked: _send(_cmdCalibrate)
            }
        }

        // ===== 无连接提示 =====
        QGCLabel {
            anchors.horizontalCenter: parent.horizontalCenter
            text:    "未连接载具"
            color:   qgcPal.textFieldText
            visible: !vehicle
            font.pointSize: ScreenTools.smallFontPointSize
        }
    }
}
