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
    readonly property int _stNavigating: USVLayout.StatusNavigating
    readonly property int _stHolding: USVLayout.StatusHolding
    readonly property int _stWaitingStable: USVLayout.StatusWaitingStable
    readonly property int _stSamplingDone: USVLayout.StatusSamplingDone
    readonly property int _stResumingAuto: USVLayout.StatusResumingAuto
    readonly property int _stPaused: USVLayout.StatusPaused
    readonly property int _stAborted: USVLayout.StatusAborted
    readonly property int _stHoldNoMission: USVLayout.StatusHoldNoMission
    readonly property int _stSurveying: USVLayout.StatusSurveying

    readonly property int _cmdStart: 31010
    readonly property int _cmdStop: 31011
    readonly property int _cmdPause: 31012
    readonly property int _cmdResume: 31013
    readonly property int _cmdCalibrate: 31014
    readonly property int _cmdStartSurvey: 31015
    readonly property int _cmdStopSurvey: 31016
    readonly property int _cmdSetBaseline: 31017
    readonly property int _cmdSpectroStart: 31018
    readonly property int _cmdSpectroStop: 31019
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
    property var _baselineSetFact: _hasPayloadGroup ? vehicle.getFact("usvPayload.baselineSet") : null
    property var _referenceVoltageFact: _hasPayloadGroup ? vehicle.getFact("usvPayload.referenceVoltage") : null
    property var _baselineVoltageFact: _hasPayloadGroup ? vehicle.getFact("usvPayload.baselineVoltage") : null
    property var _spectrometerValidFact: _hasPayloadGroup ? vehicle.getFact("usvPayload.spectrometerValid") : null
    property var _jetsonTempFact: _hasPayloadGroup ? vehicle.getFact("usvPayload.jetsonTemp") : null
    property var _detectorTempFact: _hasPayloadGroup ? vehicle.getFact("usvPayload.detectorTemp") : null
    property var _jetsonCpuFact: _hasPayloadGroup ? vehicle.getFact("usvPayload.jetsonCpu") : null
    property var _jetsonMemoryFact: _hasPayloadGroup ? vehicle.getFact("usvPayload.jetsonMemory") : null
    property var _detectorHeapFact: _hasPayloadGroup ? vehicle.getFact("usvPayload.detectorHeap") : null
    property var _payloadGroup: _hasPayloadGroup ? vehicle.getFactGroup("usvPayload") : null

    property int payloadStatus: _statusFact ? _statusFact.value : _stIdle
    property bool baselineSet: _baselineSetFact ? Number(_baselineSetFact.value) >= 1 : false
    property bool spectrometerValid: _spectrometerValidFact ? Number(_spectrometerValidFact.value) >= 1 : false
    property bool _isWorking: payloadStatus === _stSampling || payloadStatus === _stDetecting || payloadStatus === _stCalibrating
                              || payloadStatus === _stNavigating || payloadStatus === _stHolding
                              || payloadStatus === _stWaitingStable || payloadStatus === _stResumingAuto
                              || payloadStatus === _stSurveying
    property bool _canStartPointSample: payloadStatus === _stIdle || payloadStatus === _stSamplingDone || payloadStatus === _stHoldNoMission
    property bool _linkOk: _linkActiveFact ? _linkActiveFact.value === 1 : !_hasPayloadGroup
    property var _panelState: USVLayout.payloadState(!!vehicle, payloadStatus, _linkOk, _expanded)
    property string _lastCommandMessage: ""
    property bool _lastCommandWarning: false
    property int _pendingCommand: 0
    property bool _hasPendingCommand: _pendingCommand > 0
    readonly property int _commandTimeoutMs: 5000

    function statusText(st) {
        if (st === _stSurveying) {
            return qsTr("走航检测")
        }
        switch (st) {
        case _stIdle: return qsTr("空闲")
        case _stSampling: return qsTr("采样中")
        case _stDetecting: return qsTr("检测中")
        case _stFault: return qsTr("任务失败")
        case _stCalibrating: return qsTr("校准中")
        case _stNavigating: return qsTr("航行中")
        case _stHolding: return qsTr("保持")
        case _stWaitingStable: return qsTr("稳定等待")
        case _stSamplingDone: return qsTr("采样完成")
        case _stResumingAuto: return qsTr("恢复航行")
        case _stPaused: return qsTr("已暂停")
        case _stAborted: return qsTr("已中止")
        case _stHoldNoMission: return qsTr("无任务")
        default: return qsTr("未知")
        }
    }

    function statusColor(st) {
        if (st === _stSurveying) {
            return qgcPal.brandingBlue
        }
        switch (st) {
        case _stSampling:
        case _stSamplingDone:
            return qgcPal.colorGreen
        case _stDetecting:
        case _stNavigating:
        case _stResumingAuto:
            return qgcPal.brandingBlue
        case _stFault:
        case _stAborted:
            return qgcPal.colorRed
        case _stCalibrating:
        case _stHolding:
        case _stWaitingStable:
        case _stPaused:
            return qgcPal.colorOrange
        default: return qgcPal.text
        }
    }

    function _factString(fact, suffix) {
        return (_linkOk && fact) ? fact.valueString + suffix : "--"
    }

    function _pumpString(fact) {
        return (_linkOk && fact && fact.value !== undefined) ? Number(fact.value).toFixed(1) + "\u00B0" : "--"
    }

    function _healthString(fact, suffix) {
        if (!_linkOk || !fact || fact.value === undefined || Number(fact.value) < 0) {
            return "--"
        }
        return Number(fact.value).toFixed(1) + suffix
    }

    function _send(cmdId, param1) {
        if (vehicle && !_hasPendingCommand) {
            _pendingCommand = cmdId
            _lastCommandWarning = false
            _lastCommandMessage = _commandName(cmdId) + qsTr("发送中")
            commandTimeoutTimer.restart()
            vehicle.sendCommand(_payloadCompId, cmdId, false, param1 || 0)
        }
    }

    function _clearPendingCommand(command) {
        if (_pendingCommand === command) {
            _pendingCommand = 0
            commandTimeoutTimer.stop()
        }
    }

    function _commandName(cmdId) {
        switch (cmdId) {
        case _cmdSpectroStart: return qsTr("启动信号采集")
        case _cmdSpectroStop: return qsTr("停止信号采集")
        case _cmdSetBaseline: return qsTr("设基线")
        case _cmdStart: return qsTr("开始点采样")
        case _cmdStartSurvey: return qsTr("开始走航")
        case _cmdStopSurvey: return qsTr("停止走航")
        case _cmdStop: return qsTr("停止检测")
        default: return qsTr("载荷命令")
        }
    }

    Connections {
        target: vehicle
        ignoreUnknownSignals: true

        function onMavCommandResult(vehicleId, targetComponent, command, ackResult, failureCode) {
            if (targetComponent !== _payloadCompId || command < _cmdStart || command > _cmdSpectroStop) {
                return
            }
            _clearPendingCommand(command)
            _lastCommandWarning = ackResult !== 0
            _lastCommandMessage = _commandName(command) + (ackResult === 0 ? qsTr("已接受") : qsTr("未执行"))
        }
    }

    Timer {
        id: commandTimeoutTimer
        interval: _commandTimeoutMs
        repeat: false
        onTriggered: {
            if (_hasPendingCommand) {
                _lastCommandWarning = true
                _lastCommandMessage = _commandName(_pendingCommand) + qsTr("响应超时")
                _pendingCommand = 0
            }
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
                    text: !vehicle ? qsTr("未连接载具")
                         : (payloadStatus === _stFault ? statusText(payloadStatus)
                         : (_linkOk ? statusText(payloadStatus) : qsTr("载荷离线")))
                    color: !vehicle ? qgcPal.text :
                           (payloadStatus === _stFault ? qgcPal.colorRed
                           : (_linkOk ? statusColor(payloadStatus) : qgcPal.colorOrange))
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
                    text: !vehicle ? "--" : (!_linkOk ? qsTr("离线") : (spectrometerValid ? qsTr("信号有效") : qsTr("无有效信号")))
                    color: !_linkOk ? qgcPal.colorOrange : (spectrometerValid ? qgcPal.colorGreen : qgcPal.colorOrange)
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
                    visible: _lastCommandMessage.length > 0
                    height: commandStatusLabel.implicitHeight + _m
                    radius: _m * 0.45
                    color: _lastCommandWarning
                           ? Qt.rgba(qgcPal.colorOrange.r, qgcPal.colorOrange.g, qgcPal.colorOrange.b, 0.16)
                           : Qt.rgba(qgcPal.colorGreen.r, qgcPal.colorGreen.g, qgcPal.colorGreen.b, 0.12)

                    QGCLabel {
                        id: commandStatusLabel
                        anchors.centerIn: parent
                        text: _lastCommandMessage
                        color: _lastCommandWarning ? qgcPal.colorOrange : qgcPal.colorGreen
                        font.pointSize: ScreenTools.smallFontPointSize
                        font.bold: true
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    visible: vehicle && (!_linkOk || payloadStatus === _stFault || !spectrometerValid)
                    height: bannerLabel.implicitHeight + _m
                    radius: _m * 0.45
                    color: payloadStatus === _stFault
                           ? Qt.rgba(qgcPal.colorRed.r, qgcPal.colorRed.g, qgcPal.colorRed.b, 0.16)
                           : Qt.rgba(qgcPal.colorOrange.r, qgcPal.colorOrange.g, qgcPal.colorOrange.b, 0.16)

                    QGCLabel {
                        id: bannerLabel
                        anchors.centerIn: parent
                        text: payloadStatus === _stFault ? qsTr("采样任务失败，请检查流程前置条件")
                             : (!_linkOk ? qsTr("载荷通信中断，显示值已冻结") : qsTr("检测器暂无有效信号，请先启动信号采集"))
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
                        QGCLabel { text: qsTr("Baseline"); opacity: 0.55; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel {
                            text: baselineSet ? qsTr("已设置") : qsTr("未设基线")
                            color: baselineSet ? qgcPal.colorGreen : qgcPal.colorOrange
                            font.bold: true
                            font.pointSize: ScreenTools.smallFontPointSize
                        }
                        QGCLabel { text: qsTr("Ref V"); opacity: 0.55; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: _factString(_referenceVoltageFact, " V"); font.bold: true; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: qsTr("Base V"); opacity: 0.55; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: _factString(_baselineVoltageFact, " V"); font.bold: true; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: qsTr("信号"); opacity: 0.55; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel {
                            text: spectrometerValid ? qsTr("有效") : qsTr("无有效采样")
                            color: spectrometerValid ? qgcPal.colorGreen : qgcPal.colorOrange
                            font.bold: true
                            font.pointSize: ScreenTools.smallFontPointSize
                        }
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
                                { text: qsTr("启动信号采集"), cmd: _cmdSpectroStart, param1: 0, en: vehicle && _linkOk && payloadStatus !== _stFault, warn: false, span: 2 },
                                { text: qsTr("设基线"), cmd: _cmdSetBaseline, param1: 0, en: vehicle && _linkOk && spectrometerValid && payloadStatus !== _stFault, warn: false, span: 1 },
                                { text: qsTr("停止信号采集"), cmd: _cmdSpectroStop, param1: 0, en: vehicle && _linkOk, warn: true, span: 1 },
                                { text: qsTr("开始点采样"), cmd: _cmdStart, param1: 0, en: vehicle && _linkOk && spectrometerValid && baselineSet && _canStartPointSample, warn: false, span: 1 },
                                { text: qsTr("开始走航"), cmd: _cmdStartSurvey, param1: 5, en: vehicle && _linkOk && spectrometerValid && baselineSet && payloadStatus !== _stFault && payloadStatus !== _stSurveying, warn: false, span: 1 },
                                { text: qsTr("停止检测"), cmd: _cmdStop, param1: 0, en: vehicle && _isWorking, warn: true, span: 1 },
                                { text: qsTr("停止走航"), cmd: _cmdStopSurvey, param1: 0, en: vehicle && payloadStatus === _stSurveying, warn: true, span: 1 },
                                { text: qsTr("暂停"), cmd: _cmdPause, param1: 0, en: vehicle && payloadStatus === _stSampling, warn: false, span: 1 },
                                { text: qsTr("恢复"), cmd: _cmdResume, param1: 0, en: vehicle && payloadStatus === _stPaused, warn: false, span: 1 },
                                { text: qsTr("零点校准"), cmd: _cmdCalibrate, param1: 0, en: vehicle && _linkOk && _canStartPointSample, warn: false, span: 2 }
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
                                    text: _pendingCommand === modelData.cmd ? qsTr("发送中") : modelData.text
                                    color: modelData.warn && modelData.en ? "white" : qgcPal.text
                                    opacity: (modelData.en && !_hasPendingCommand) || _pendingCommand === modelData.cmd ? 1.0 : 0.35
                                    font.bold: true
                                    font.pointSize: ScreenTools.smallFontPointSize
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    enabled: modelData.en && !_hasPendingCommand
                                    onClicked: root._send(modelData.cmd, modelData.param1)
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
                        QGCLabel { text: qsTr("Jetson 温度"); opacity: 0.5; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: _healthString(_jetsonTempFact, " \u00B0C"); font.bold: true; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: qsTr("ESP32 温度"); opacity: 0.5; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: _healthString(_detectorTempFact, " \u00B0C"); font.bold: true; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: qsTr("Jetson CPU"); opacity: 0.5; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: _healthString(_jetsonCpuFact, "%"); font.bold: true; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: qsTr("Jetson 内存"); opacity: 0.5; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: _healthString(_jetsonMemoryFact, "%"); font.bold: true; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: qsTr("ESP32 Heap"); opacity: 0.5; font.pointSize: ScreenTools.smallFontPointSize }
                        QGCLabel { text: _healthString(_detectorHeapFact, "%"); font.bold: true; font.pointSize: ScreenTools.smallFontPointSize }
                    }
                }
            }
        }
    }
}
