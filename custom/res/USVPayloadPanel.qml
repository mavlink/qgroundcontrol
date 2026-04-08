import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

Rectangle {
    id:         root
    width:      Math.min(ScreenTools.defaultFontPixelWidth * 26, parent ? parent.width * 0.9 : 300)
    height:     mainColumn.height
    color:      Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.88)
    radius:     ScreenTools.defaultFontPixelWidth
    clip:       true

    property var vehicle: null
    property bool _expanded: true

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
    readonly property int _payloadCompId: 191

    property real _m: ScreenTools.defaultFontPixelWidth

    // ========== Fact 缓存 ==========
    property bool _hasPayloadGroup: vehicle && vehicle.factGroupNames.indexOf("usvPayload") >= 0
    property var _voltageFact:     _hasPayloadGroup ? vehicle.getFact("usvPayload.voltage")    : null
    property var _absorbanceFact:  _hasPayloadGroup ? vehicle.getFact("usvPayload.absorbance") : null
    property var _pumpXFact:       _hasPayloadGroup ? vehicle.getFact("usvPayload.pumpX")      : null
    property var _pumpYFact:       _hasPayloadGroup ? vehicle.getFact("usvPayload.pumpY")      : null
    property var _pumpZFact:       _hasPayloadGroup ? vehicle.getFact("usvPayload.pumpZ")      : null
    property var _pumpAFact:       _hasPayloadGroup ? vehicle.getFact("usvPayload.pumpA")      : null
    property var _statusFact:      _hasPayloadGroup ? vehicle.getFact("usvPayload.status")     : null
    property var _linkActiveFact:  _hasPayloadGroup ? vehicle.getFact("usvPayload.linkActive") : null
    property var _packetCountFact: _hasPayloadGroup ? vehicle.getFact("usvPayload.packetCount"): null

    property int payloadStatus: _statusFact ? _statusFact.value : _stIdle

    function statusText(st) {
        switch(st) {
        case _stIdle:        return "空闲"
        case _stSampling:    return "采样中"
        case _stDetecting:   return "检测中"
        case _stFault:       return "故障"
        case _stCalibrating: return "校准中"
        default:             return "未知"
        }
    }

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
    property bool _linkOk: _linkActiveFact ? _linkActiveFact.value === 1 : !_hasPayloadGroup

    function _send(cmdId) {
        if (vehicle) vehicle.sendCommand(_payloadCompId, cmdId, false)
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    Column {
        id: mainColumn
        width: parent.width

        // ===== 标题栏 =====
        Item {
            width: parent.width
            height: headerRow.height + _m * 2

            MouseArea {
                anchors.fill: parent
                onClicked: root._expanded = !root._expanded
            }

            RowLayout {
                id: headerRow
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: _m * 1.5
                spacing: _m

                QGCLabel {
                    text: root._expanded ? "▾" : "▸"
                    font.pointSize: ScreenTools.smallFontPointSize
                    opacity: 0.5
                }

                QGCLabel {
                    text: "水质载荷"
                    font.bold: true
                    Layout.fillWidth: true
                }

                Rectangle {
                    width: _m * 1.2; height: width; radius: width / 2
                    color: statusColor(payloadStatus)
                    SequentialAnimation on opacity {
                        running: root._isWorking
                        loops: Animation.Infinite
                        NumberAnimation { to: 0.2; duration: 700; easing.type: Easing.InOutQuad }
                        NumberAnimation { to: 1.0; duration: 700; easing.type: Easing.InOutQuad }
                    }
                }

                QGCLabel {
                    text: statusText(payloadStatus)
                    color: statusColor(payloadStatus)
                    font.pointSize: ScreenTools.smallFontPointSize
                    font.bold: true
                }
            }
        }

        // ===== 可折叠区 =====
        Item {
            width: parent.width
            height: root._expanded ? expandedCol.height + _m : 0
            clip: true
            Behavior on height { NumberAnimation { duration: 200; easing.type: Easing.InOutQuad } }

            ColumnLayout {
                id: expandedCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: _m * 1.5
                spacing: _m

                // 分隔线
                Rectangle { Layout.fillWidth: true; height: 1; color: qgcPal.windowShade; opacity: 0.4 }

                // 链路警告
                Rectangle {
                    Layout.fillWidth: true
                    height: warnLabel.height + _m
                    color: Qt.rgba(1, 0, 0, 0.12)
                    radius: _m * 0.5
                    visible: _hasPayloadGroup && _linkActiveFact && _linkActiveFact.value === 0
                    QGCLabel {
                        id: warnLabel
                        anchors.centerIn: parent
                        text: "遥测超时"
                        color: qgcPal.colorRed
                        font.pointSize: ScreenTools.smallFontPointSize
                        font.bold: true
                    }
                }

                // 数据显示
                RowLayout {
                    Layout.fillWidth: true
                    spacing: _m * 2
                    opacity: _linkOk ? 1.0 : 0.4

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        QGCLabel { text: "电压"; font.pointSize: ScreenTools.smallFontPointSize; opacity: 0.6 }
                        QGCLabel {
                            text: _voltageFact ? _voltageFact.valueString + " V" : "--"
                            font.bold: true
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        QGCLabel { text: "吸光度"; font.pointSize: ScreenTools.smallFontPointSize; opacity: 0.6 }
                        QGCLabel {
                            text: _absorbanceFact ? _absorbanceFact.valueString + " Abs" : "--"
                            color: qgcPal.brandingBlue
                            font.bold: true
                        }
                    }
                }

                // 泵组角度
                Rectangle {
                    Layout.fillWidth: true
                    height: pumpRow.height + _m
                    color: Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.3)
                    radius: _m * 0.5
                    opacity: _linkOk ? 1.0 : 0.4

                    RowLayout {
                        id: pumpRow
                        anchors.centerIn: parent
                        width: parent.width - _m * 2
                        spacing: 0

                        Repeater {
                            model: [
                                { label: "X", fact: root._pumpXFact },
                                { label: "Y", fact: root._pumpYFact },
                                { label: "Z", fact: root._pumpZFact },
                                { label: "A", fact: root._pumpAFact }
                            ]
                            delegate: RowLayout {
                                Layout.fillWidth: true
                                spacing: _m * 0.3
                                QGCLabel {
                                    text: modelData.label
                                    font.pointSize: ScreenTools.smallFontPointSize
                                    opacity: 0.5
                                }
                                QGCLabel {
                                    text: (modelData.fact && modelData.fact.value !== undefined) ? Number(modelData.fact.value).toFixed(1) + "°" : "--"
                                    font.bold: true
                                    font.pointSize: ScreenTools.smallFontPointSize
                                }
                                Rectangle {
                                    width: 1; height: ScreenTools.defaultFontPixelHeight * 0.7
                                    color: qgcPal.text; opacity: 0.15
                                    visible: index < 3
                                    Layout.leftMargin: _m * 0.5
                                }
                            }
                        }
                    }
                }

                // 控制按钮
                GridLayout {
                    columns: 2
                    rowSpacing: _m * 0.6
                    columnSpacing: _m * 0.6
                    Layout.fillWidth: true
                    opacity: vehicle ? 1.0 : 0.5

                    Repeater {
                        model: [
                            { text: "▶ 采样",  cmd: _cmdStart,     en: vehicle && payloadStatus === _stIdle, warn: false, span: 1 },
                            { text: "■ 停止",   cmd: _cmdStop,      en: vehicle && _isWorking,                warn: true,  span: 1 },
                            { text: "⏸ 暂停",  cmd: _cmdPause,     en: vehicle && payloadStatus === _stSampling, warn: false, span: 1 },
                            { text: "⏵ 恢复",  cmd: _cmdResume,    en: vehicle && (payloadStatus === _stSampling || payloadStatus === _stIdle), warn: false, span: 1 },
                            { text: "⚙ 校准",  cmd: _cmdCalibrate, en: vehicle && payloadStatus === _stIdle, warn: false, span: 2 }
                        ]
                        delegate: Rectangle {
                            Layout.fillWidth: true
                            Layout.columnSpan: modelData.span
                            height: ScreenTools.defaultFontPixelHeight * 1.8
                            radius: _m * 0.5
                            color: !modelData.en ? Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.3) :
                                   modelData.warn ? Qt.rgba(0.8, 0.2, 0.2, 0.85) :
                                   Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.7)

                            QGCLabel {
                                anchors.centerIn: parent
                                text: modelData.text
                                color: modelData.warn && modelData.en ? "white" : qgcPal.text
                                opacity: modelData.en ? 1.0 : 0.35
                                font.bold: true
                                font.pointSize: ScreenTools.smallFontPointSize
                            }

                            MouseArea {
                                anchors.fill: parent
                                enabled: modelData.en
                                onClicked: _send(modelData.cmd)
                            }
                        }
                    }
                }

                // ========== 通信诊断折叠面板 ==========
                Rectangle {
                    id: diagRect
                    Layout.fillWidth: true
                    height: diagCol.height
                    color: Qt.rgba(0, 0, 0, 0.05)
                    radius: _m * 0.5
                    visible: _hasPayloadGroup

                    property bool diagExpanded: false
                    property var payloadGroup: _hasPayloadGroup ? vehicle.getFactGroup("usvPayload") : null

                    ColumnLayout {
                        id: diagCol
                        width: parent.width
                        spacing: _m * 0.3

                        // 标题栏 (可点击展开/折叠)
                        Item {
                            Layout.fillWidth: true
                            height: diagTitleRow.height + _m

                            RowLayout {
                                id: diagTitleRow
                                anchors.fill: parent
                                anchors.margins: _m * 0.5

                                QGCLabel {
                                    text: diagRect.diagExpanded ? "▼ 通信诊断" : "▶ 通信诊断"
                                    font.pointSize: ScreenTools.smallFontPointSize
                                    font.bold: true
                                    opacity: 0.7
                                }
                                Item { Layout.fillWidth: true }
                                Rectangle {
                                    width: _m * 0.8; height: _m * 0.8; radius: width / 2
                                    color: _linkOk ? qgcPal.colorGreen : qgcPal.colorRed
                                }
                                QGCLabel {
                                    text: _linkOk ? "在线" : "离线"
                                    font.pointSize: ScreenTools.smallFontPointSize
                                    opacity: 0.6
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: diagRect.diagExpanded = !diagRect.diagExpanded
                            }
                        }

                        // 展开内容
                        GridLayout {
                            columns: 2
                            columnSpacing: _m * 0.8
                            rowSpacing: _m * 0.2
                            Layout.leftMargin: _m
                            Layout.rightMargin: _m
                            Layout.bottomMargin: _m * 0.5
                            visible: diagRect.diagExpanded

                            QGCLabel { text: "RX总计"; font.pointSize: ScreenTools.smallFontPointSize; opacity: 0.5 }
                            QGCLabel {
                                text: diagRect.payloadGroup ? diagRect.payloadGroup.rxMsgTotal : "0"
                                font.pointSize: ScreenTools.smallFontPointSize; font.bold: true
                            }
                            QGCLabel { text: "Named值"; font.pointSize: ScreenTools.smallFontPointSize; opacity: 0.5 }
                            QGCLabel {
                                text: diagRect.payloadGroup ? diagRect.payloadGroup.rxNamedValue : "0"
                                font.pointSize: ScreenTools.smallFontPointSize; font.bold: true
                            }
                            QGCLabel { text: "心跳"; font.pointSize: ScreenTools.smallFontPointSize; opacity: 0.5 }
                            QGCLabel {
                                text: diagRect.payloadGroup ? diagRect.payloadGroup.rxHeartbeat : "0"
                                font.pointSize: ScreenTools.smallFontPointSize; font.bold: true
                            }
                            QGCLabel { text: "来源SysID"; font.pointSize: ScreenTools.smallFontPointSize; opacity: 0.5 }
                            QGCLabel {
                                text: diagRect.payloadGroup ? diagRect.payloadGroup.lastSysid : "--"
                                font.pointSize: ScreenTools.smallFontPointSize; font.bold: true
                            }
                            QGCLabel { text: "来源CompID"; font.pointSize: ScreenTools.smallFontPointSize; opacity: 0.5 }
                            QGCLabel {
                                text: diagRect.payloadGroup ? diagRect.payloadGroup.lastCompid : "--"
                                font.pointSize: ScreenTools.smallFontPointSize; font.bold: true
                            }
                            QGCLabel { text: "心跳间隔"; font.pointSize: ScreenTools.smallFontPointSize; opacity: 0.5 }
                            QGCLabel {
                                text: diagRect.payloadGroup ? Number(diagRect.payloadGroup.latencyMs).toFixed(0) + " ms" : "--"
                                font.pointSize: ScreenTools.smallFontPointSize; font.bold: true
                            }
                        }
                    }
                }

                // 无连接提示
                QGCLabel {
                    Layout.alignment: Qt.AlignHCenter
                    text: "未连接载具"
                    opacity: 0.5
                    visible: !vehicle
                    font.pointSize: ScreenTools.smallFontPointSize
                }
            }
        }
    }
}
