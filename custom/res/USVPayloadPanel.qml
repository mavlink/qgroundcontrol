import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

import "USVFlyViewLayout.js" as USVLayout

Rectangle {
    id: root

    width: _panelState.compact ? compactWidth : expandedWidth
    height: mainColumn.implicitHeight + _m * 1.3
    radius: ScreenTools.defaultFontPixelWidth
    color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.9)
    border.width: 1
    border.color: Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.45)
    clip: true

    property var vehicle: null
    property bool _expanded: false

    readonly property int _stIdle: USVLayout.StatusIdle
    readonly property int _stSampling: USVLayout.StatusSampling
    readonly property int _stDetecting: USVLayout.StatusDetecting
    readonly property int _stFault: USVLayout.StatusFault
    readonly property int _stCalibrating: USVLayout.StatusCalibrating

    readonly property int _cmdStart: 31010
    readonly property int _cmdStop: 31011
    readonly property int _cmdPause: 31012
    readonly property int _cmdResume: 31013
    readonly property int _cmdCalibrate: 31014
    readonly property int _payloadCompId: 191

    property real _m: ScreenTools.defaultFontPixelWidth
    property real compactWidth: ScreenTools.defaultFontPixelWidth * 18
    property real expandedWidth: ScreenTools.defaultFontPixelWidth * 24

    property bool _hasPayloadGroup: vehicle && vehicle.factGroupNames.indexOf("usvPayload") >= 0
    property var _voltageFact: _hasPayloadGroup ? vehicle.getFact("usvPayload.voltage") : null
    property var _absorbanceFact: _hasPayloadGroup ? vehicle.getFact("usvPayload.absorbance") : null
    property var _pumpXFact: _hasPayloadGroup ? vehicle.getFact("usvPayload.pumpX") : null
    property var _pumpYFact: _hasPayloadGroup ? vehicle.getFact("usvPayload.pumpY") : null
    property var _pumpZFact: _hasPayloadGroup ? vehicle.getFact("usvPayload.pumpZ") : null
    property var _pumpAFact: _hasPayloadGroup ? vehicle.getFact("usvPayload.pumpA") : null
    property var _statusFact: _hasPayloadGroup ? vehicle.getFact("usvPayload.status") : null
    property var _linkActiveFact: _hasPayloadGroup ? vehicle.getFact("usvPayload.linkActive") : null
    property var _packetCountFact: _hasPayloadGroup ? vehicle.getFact("usvPayload.packetCount") : null
    property var _payloadGroup: _hasPayloadGroup ? vehicle.getFactGroup("usvPayload") : null

    property int payloadStatus: _statusFact ? _statusFact.value : _stIdle
    property bool _isWorking: payloadStatus === _stSampling || payloadStatus === _stDetecting || payloadStatus === _stCalibrating
    property bool _linkOk: _linkActiveFact ? _linkActiveFact.value === 1 : !_hasPayloadGroup
    property var _panelState: USVLayout.payloadState(!!vehicle, payloadStatus, _linkOk, _expanded)

    function statusText(st) {
        switch (st) {
        case _stIdle: return qsTr("空闲")
        case _stSampling: return qsTr("采样中")
        case _stDetecting: return qsTr("检测中")
        case _stFault: return qsTr("故障")
        case _stCalibrating: return qsTr("校准中")
        default: return qsTr("未知")
        }
    }

    function statusColor(st) {
        switch (st) {
        case _stSampling: return qgcPal.colorGreen
        case _stDetecting: return qgcPal.brandingBlue
        case _stFault: return qgcPal.colorRed
        case _stCalibrating: return qgcPal.colorOrange
        default: return qgcPal.text
        }
    }

    function _factString(fact, suffix) {
        return (_linkOk && fact) ? fact.valueString + suffix : "--"
    }

    function _pumpString(fact) {
        return (_linkOk && fact && fact.value !== undefined) ? Number(fact.value).toFixed(1) + "\u00B0" : "--"
    }

    function _send(cmdId) {
        if (vehicle) {
            vehicle.sendCommand(_payloadCompId, cmdId, true)
        }
    }

    Behavior on width { NumberAnimation { duration: 180; easing.type: Easing.InOutQuad } }
    Behavior on height { NumberAnimation { duration: 180; easing.type: Easing.InOutQuad } }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    ColumnLayout {
        id: mainColumn
        anchors.fill: parent
        anchors.margins: _m * 1.05
        spacing: _m * 0.8

        RowLayout {
            Layout.fillWidth: true
            spacing: _m * 0.7

            Rectangle {
                width: _m * 0.45
                radius: width / 2
                Layout.fillHeight: true
                color: statusColor(payloadStatus)
                opacity: _panelState.emphasizeSummary ? 1.0 : 0.55
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0

                QGCLabel {
                    text: qsTr("水质载荷")
                    font.bold: true
                    elide: Text.ElideRight
                }

                QGCLabel {
                    text: !vehicle ? qsTr("未连接载具") : (_linkOk ? statusText(payloadStatus) : qsTr("载荷离线"))
                    color: !vehicle ? qgcPal.text :
                           (_linkOk ? statusColor(payloadStatus) : qgcPal.colorOrange)
                    font.pointSize: ScreenTools.smallFontPointSize
                    opacity: 0.85
                    font.bold: _panelState.emphasizeSummary
                    elide: Text.ElideRight
                }
            }

            Rectangle {
                radius: _m * 0.45
                color: Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.22)
                border.width: 1
                border.color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.14)
                width: toggleRow.implicitWidth + _m
                height: toggleRow.implicitHeight + _m * 0.5

                RowLayout {
                    id: toggleRow
                    anchors.centerIn: parent
                    spacing: _m * 0.3

                    QGCLabel {
                        text: root._panelState.compact ? qsTr("展开") : qsTr("收起")
                        font.pointSize: ScreenTools.smallFontPointSize
                        opacity: 0.72
                    }

                    QGCLabel {
                        text: root._panelState.compact ? "\u25B8" : "\u25BE"
                        font.pointSize: ScreenTools.smallFontPointSize
                        opacity: 0.72
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: root._expanded = !root._expanded
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: _m * 0.8

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0

                QGCLabel {
                    text: qsTr("吸光度")
                    font.pointSize: ScreenTools.smallFontPointSize
                    opacity: 0.55
                }

                QGCLabel {
                    text: _factString(_absorbanceFact, " Abs")
                    color: qgcPal.brandingBlue
                    font.bold: true
                    elide: Text.ElideRight
                }
            }

            Rectangle {
                width: 1
                height: ScreenTools.defaultFontPixelHeight * 1.6
                color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.12)
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0

                QGCLabel {
                    text: qsTr("电压")
                    font.pointSize: ScreenTools.smallFontPointSize
                    opacity: 0.55
                }

                QGCLabel {
                    text: _factString(_voltageFact, " V")
                    font.bold: true
                    elide: Text.ElideRight
                }
            }

            Rectangle {
                width: 1
                height: ScreenTools.defaultFontPixelHeight * 1.6
                color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.12)
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0

                QGCLabel {
                    text: qsTr("链路")
                    font.pointSize: ScreenTools.smallFontPointSize
                    opacity: 0.55
                }

                QGCLabel {
                    text: !vehicle ? "--" : (_linkOk ? qsTr("在线") : qsTr("离线"))
                    color: _linkOk ? qgcPal.colorGreen : qgcPal.colorOrange
                    font.bold: true
                    elide: Text.ElideRight
                }
            }
        }

        Item {
            Layout.fillWidth: true
            height: root._panelState.showDetail ? detailColumn.implicitHeight : 0
            clip: true

            Behavior on height { NumberAnimation { duration: 180; easing.type: Easing.InOutQuad } }

            ColumnLayout {
                id: detailColumn
                width: parent.width
                spacing: _m * 0.75

                Rectangle {
                    Layout.fillWidth: true
                    visible: vehicle && (!_linkOk || payloadStatus === _stFault)
                    height: bannerLabel.implicitHeight + _m
                    radius: _m * 0.45
                    color: payloadStatus === _stFault
                           ? Qt.rgba(qgcPal.colorRed.r, qgcPal.colorRed.g, qgcPal.colorRed.b, 0.16)
                           : Qt.rgba(qgcPal.colorOrange.r, qgcPal.colorOrange.g, qgcPal.colorOrange.b, 0.16)

                    QGCLabel {
                        id: bannerLabel
                        anchors.centerIn: parent
                        text: payloadStatus === _stFault ? qsTr("载荷故障，请检查采样模块") : qsTr("载荷通信中断，显示值已冻结")
                        color: payloadStatus === _stFault ? qgcPal.colorRed : qgcPal.colorOrange
                        font.pointSize: ScreenTools.smallFontPointSize
                        font.bold: true
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    radius: _m * 0.55
                    color: Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.2)
                    visible: !!vehicle
                    height: metricsGrid.implicitHeight + _m * 1.1

                    GridLayout {
                        id: metricsGrid
                        anchors.fill: parent
                        anchors.margins: _m * 0.55
                        columns: 2
                        columnSpacing: _m
                        rowSpacing: _m * 0.3

                        QGCLabel { text: qsTr("X 泵"); opacity: 0.55; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: _pumpString(_pumpXFact); font.bold: true; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: qsTr("Y 泵"); opacity: 0.55; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: _pumpString(_pumpYFact); font.bold: true; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: qsTr("Z 泵"); opacity: 0.55; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: _pumpString(_pumpZFact); font.bold: true; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: qsTr("A 泵"); opacity: 0.55; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: _pumpString(_pumpAFact); font.bold: true; font.pointSize: ScreenTools.smallFontPointSize }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    height: root._expanded ? controlGrid.implicitHeight : 0
                    clip: true

                    Behavior on height { NumberAnimation { duration: 180; easing.type: Easing.InOutQuad } }

                    GridLayout {
                        id: controlGrid
                        width: parent.width
                        columns: 2
                        rowSpacing: _m * 0.55
                        columnSpacing: _m * 0.55

                        Repeater {
                            model: [
                                { text: qsTr("开始采样"), cmd: _cmdStart,     en: vehicle && payloadStatus === _stIdle, warn: false, span: 1 },
                                { text: qsTr("停止"),     cmd: _cmdStop,      en: vehicle && _isWorking,                warn: true,  span: 1 },
                                { text: qsTr("暂停"),     cmd: _cmdPause,     en: vehicle && payloadStatus === _stSampling, warn: false, span: 1 },
                                { text: qsTr("恢复"),     cmd: _cmdResume,    en: vehicle && (payloadStatus === _stSampling || payloadStatus === _stIdle), warn: false, span: 1 },
                                { text: qsTr("零点校准"), cmd: _cmdCalibrate, en: vehicle && payloadStatus === _stIdle, warn: false, span: 2 }
                            ]

                            delegate: Rectangle {
                                Layout.fillWidth: true
                                Layout.columnSpan: modelData.span
                                height: ScreenTools.defaultFontPixelHeight * 1.9
                                radius: _m * 0.45
                                color: !modelData.en
                                       ? Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.18)
                                       : (modelData.warn
                                          ? Qt.rgba(qgcPal.colorRed.r, qgcPal.colorRed.g, qgcPal.colorRed.b, 0.85)
                                          : Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.6))

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
                                    onClicked: root._send(modelData.cmd)
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    visible: root._panelState.showDiagnostics && !!vehicle && _hasPayloadGroup
                    radius: _m * 0.45
                    color: Qt.rgba(0, 0, 0, 0.05)
                    height: diagnosticsGrid.implicitHeight + _m

                    GridLayout {
                        id: diagnosticsGrid
                        anchors.fill: parent
                        anchors.margins: _m * 0.5
                        columns: 2
                        columnSpacing: _m * 0.8
                        rowSpacing: _m * 0.2

                        QGCLabel { text: qsTr("通信诊断"); font.bold: true; opacity: 0.72 }
                        QGCLabel { text: _linkOk ? qsTr("在线") : qsTr("离线"); color: _linkOk ? qgcPal.colorGreen : qgcPal.colorOrange; font.bold: true }
                        QGCLabel { text: qsTr("RX 总计"); opacity: 0.5; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: _payloadGroup ? _payloadGroup.rxMsgTotal : "0"; font.bold: true; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: qsTr("NamedValue"); opacity: 0.5; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: _payloadGroup ? _payloadGroup.rxNamedValue : "0"; font.bold: true; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: qsTr("心跳"); opacity: 0.5; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: _payloadGroup ? _payloadGroup.rxHeartbeat : "0"; font.bold: true; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: qsTr("延迟"); opacity: 0.5; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: _payloadGroup ? Number(_payloadGroup.latencyMs).toFixed(0) + " ms" : "--"; font.bold: true; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: qsTr("数据包"); opacity: 0.5; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: _packetCountFact ? _packetCountFact.valueString : "0"; font.bold: true; font.pointSize: ScreenTools.smallFontPointSize }
                    }
                }
            }
        }
    }
}
