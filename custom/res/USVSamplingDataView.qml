import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtCharts

import QGroundControl
import QGroundControl.Controls

import "USVSamplingDataTokens.js" as SDTokens

Rectangle {
    id: root

    anchors.fill: parent

    signal popout()

    color: qgcPal.window

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property bool _hasPayloadGroup: _activeVehicle && _activeVehicle.factGroupNames.indexOf("usvPayload") >= 0
    property var _voltageFact:     _hasPayloadGroup ? _activeVehicle.getFact("usvPayload.voltage")     : null
    property var _absorbanceFact:  _hasPayloadGroup ? _activeVehicle.getFact("usvPayload.absorbance")  : null
    property var _statusFact:      _hasPayloadGroup ? _activeVehicle.getFact("usvPayload.status")      : null
    property var _linkActiveFact:  _hasPayloadGroup ? _activeVehicle.getFact("usvPayload.linkActive")  : null
    property var _stepCurrentFact: _hasPayloadGroup ? _activeVehicle.getFact("usvPayload.stepCurrent") : null
    property var _stepTotalFact:   _hasPayloadGroup ? _activeVehicle.getFact("usvPayload.stepTotal")   : null
    property var _sampleCountFact: _hasPayloadGroup ? _activeVehicle.getFact("usvPayload.sampleCount") : null
    property var _packetCountFact: _hasPayloadGroup ? _activeVehicle.getFact("usvPayload.packetCount") : null
    property var _pumpXFact:       _hasPayloadGroup ? _activeVehicle.getFact("usvPayload.pumpX")       : null
    property var _pumpYFact:       _hasPayloadGroup ? _activeVehicle.getFact("usvPayload.pumpY")       : null
    property var _pumpZFact:       _hasPayloadGroup ? _activeVehicle.getFact("usvPayload.pumpZ")       : null
    property var _pumpAFact:       _hasPayloadGroup ? _activeVehicle.getFact("usvPayload.pumpA")       : null
    property var _pidErrorFact:    _hasPayloadGroup ? _activeVehicle.getFact("usvPayload.pidError")    : null
    property var _pidModeFact:     _hasPayloadGroup ? _activeVehicle.getFact("usvPayload.pidMode")     : null

    property real _m: ScreenTools.defaultFontPixelWidth
    property real _h: ScreenTools.defaultFontPixelHeight

    property bool _linkOk: _linkActiveFact ? Number(_linkActiveFact.value) === 1 : false
    property int payloadStatus: _statusFact ? Number(_statusFact.value) : SDTokens.StatusIdle
    // 只有在活跃工作状态时才向曲线追加数据点，避免非采样时持续累积导致内存增长和 heap 904
    property bool _shouldChart: payloadStatus === SDTokens.StatusSampling
                                || payloadStatus === SDTokens.StatusDetecting
                                || payloadStatus === SDTokens.StatusCalibrating
                                || payloadStatus === SDTokens.StatusWaitingStable

    property bool _chartPaused: false
    property int _sampleIndex: 0

    property real _voltageMinStat: 0
    property real _voltageMaxStat: 0
    property real _voltageSum: 0
    property real _voltageSumSquares: 0
    property int  _voltageCount: 0

    property real _absorbanceMinStat: 0
    property real _absorbanceMaxStat: 0
    property real _absorbanceSum: 0
    property real _absorbanceSumSquares: 0
    property int  _absorbanceCount: 0

    property double _samplingStartedMs: 0
    property int _elapsedSeconds: 0

    function _cardColor() {
        return Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, SDTokens.Tokens.opacity.cardBg)
    }

    function _cardBorderColor() {
        return Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.45)
    }

    function _mutedColor(baseColor, alpha) {
        return Qt.rgba(baseColor.r, baseColor.g, baseColor.b, alpha)
    }

    function _valueOrDefault(fact, fallback) {
        return fact && fact.value !== undefined ? Number(fact.value) : fallback
    }

    function _formatFixed(value, digits) {
        return isFinite(value) ? Number(value).toFixed(digits) : "--"
    }

    function _formatPrecision(value, digits) {
        return isFinite(value) ? Number(value).toPrecision(digits) : "--"
    }

    function _formatPump(fact) {
        return (fact && fact.value !== undefined) ? Number(fact.value).toFixed(1) : "--"
    }

    function _formatDuration(totalSeconds) {
        var safe = Math.max(0, totalSeconds)
        var minutes = Math.floor(safe / 60)
        var seconds = safe % 60
        return (minutes < 10 ? "0" : "") + minutes + ":" + (seconds < 10 ? "0" : "") + seconds
    }

    function _statAvg(sum, count) {
        return count > 0 ? sum / count : NaN
    }

    function _statStdDev(sum, sumSquares, count) {
        if (count <= 0) {
            return NaN
        }
        var mean = sum / count
        return Math.sqrt(Math.max(0, (sumSquares / count) - mean * mean))
    }

    function _pidModeColor(mode) {
        var colorName = SDTokens.pidModeColorName(mode)
        var candidate = qgcPal[colorName]
        return candidate ? candidate : qgcPal.text
    }

    function _statusColor(statusValue) {
        switch (statusValue) {
        case SDTokens.StatusSampling:
        case SDTokens.StatusSamplingDone:
            return qgcPal.colorGreen
        case SDTokens.StatusDetecting:
        case SDTokens.StatusNavigating:
        case SDTokens.StatusResumingAuto:
            return qgcPal.brandingBlue
        case SDTokens.StatusFault:
        case SDTokens.StatusAborted:
            return qgcPal.colorRed
        case SDTokens.StatusCalibrating:
        case SDTokens.StatusHolding:
        case SDTokens.StatusWaitingStable:
        case SDTokens.StatusWaypointReached:
        case SDTokens.StatusPaused:
            return qgcPal.colorOrange
        default:
            return qgcPal.text
        }
    }

    function _linkColor() {
        return _linkOk ? qgcPal.colorGreen : qgcPal.colorRed
    }

    function _updateSeriesAxis(series, axis, fallbackCenter) {
        if (series.count <= 0) {
            axis.min = fallbackCenter - 1
            axis.max = fallbackCenter + 1
            return
        }

        var minVal = series.at(0).y
        var maxVal = minVal
        for (var i = 1; i < series.count; i++) {
            var y = series.at(i).y
            if (y < minVal) {
                minVal = y
            }
            if (y > maxVal) {
                maxVal = y
            }
        }

        var range = maxVal - minVal
        var margin = range > 0 ? range * SDTokens.Tokens.chart.marginPercent : Math.max(Math.abs(minVal) * SDTokens.Tokens.chart.marginPercent, 0.001)
        axis.min = minVal - margin
        axis.max = maxVal + margin
    }

    function _updateChartAxes() {
        _updateSeriesAxis(voltageSeries, axisVoltage, 0)
        _updateSeriesAxis(absorbanceSeries, axisAbsorbance, 0)
    }

    function _appendSample(voltageValue, absorbanceValue) {
        if (voltageSeries.count >= SDTokens.Tokens.chart.maxPoints) {
            voltageSeries.remove(0)
            absorbanceSeries.remove(0)
        }

        voltageSeries.append(_sampleIndex, voltageValue)
        absorbanceSeries.append(_sampleIndex, absorbanceValue)
        _sampleIndex += 1

        axisX.min = Math.max(0, _sampleIndex - SDTokens.Tokens.chart.maxPoints)
        axisX.max = Math.max(SDTokens.Tokens.chart.maxPoints, _sampleIndex)
        _updateChartAxes()
    }

    function _updateStatistics(voltageValue, absorbanceValue) {
        if (_voltageCount === 0) {
            _voltageMinStat = voltageValue
            _voltageMaxStat = voltageValue
        } else {
            _voltageMinStat = Math.min(_voltageMinStat, voltageValue)
            _voltageMaxStat = Math.max(_voltageMaxStat, voltageValue)
        }
        _voltageCount += 1
        _voltageSum += voltageValue
        _voltageSumSquares += voltageValue * voltageValue

        if (_absorbanceCount === 0) {
            _absorbanceMinStat = absorbanceValue
            _absorbanceMaxStat = absorbanceValue
        } else {
            _absorbanceMinStat = Math.min(_absorbanceMinStat, absorbanceValue)
            _absorbanceMaxStat = Math.max(_absorbanceMaxStat, absorbanceValue)
        }
        _absorbanceCount += 1
        _absorbanceSum += absorbanceValue
        _absorbanceSumSquares += absorbanceValue * absorbanceValue
    }

    function clearAllData() {
        voltageSeries.clear()
        absorbanceSeries.clear()
        _sampleIndex = 0
        axisX.min = 0
        axisX.max = SDTokens.Tokens.chart.maxPoints
        axisVoltage.min = -1
        axisVoltage.max = 1
        axisAbsorbance.min = -1
        axisAbsorbance.max = 1

        _voltageMinStat = 0
        _voltageMaxStat = 0
        _voltageSum = 0
        _voltageSumSquares = 0
        _voltageCount = 0

        _absorbanceMinStat = 0
        _absorbanceMaxStat = 0
        _absorbanceSum = 0
        _absorbanceSumSquares = 0
        _absorbanceCount = 0
    }

    onPayloadStatusChanged: {
        if (payloadStatus === SDTokens.StatusSampling) {
            _samplingStartedMs = Date.now()
            _elapsedSeconds = 0
        } else {
            _samplingStartedMs = 0
            _elapsedSeconds = 0
        }
    }

    Component.onDestruction: {
        // 销毁前清空 LineSeries 数据，防止 Qt Charts 内部
        // 释放动态分配的数据点时触发 heap 904 断言
        sampleTimer.stop()
        durationTimer.stop()
        voltageSeries.clear()
        absorbanceSeries.clear()
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    DeadMouseArea { anchors.fill: parent }

    QGCLabel {
        anchors.centerIn: parent
        text: qsTr("请连接载具以查看采样数据")
        font.pointSize: ScreenTools.largeFontPointSize
        opacity: 0.5
        visible: !root._hasPayloadGroup
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: _m * SDTokens.Tokens.spacing.lg
        spacing: _m * SDTokens.Tokens.spacing.lg
        visible: root._hasPayloadGroup

        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.6
            radius: _m * SDTokens.Tokens.radius.md
            color: root._cardColor()
            border.width: 1
            border.color: root._cardBorderColor()
            clip: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: _m * SDTokens.Tokens.spacing.lg
                spacing: _m * SDTokens.Tokens.spacing.md

                RowLayout {
                    Layout.fillWidth: true
                    spacing: _m * SDTokens.Tokens.spacing.md

                    QGCLabel {
                        text: qsTr("实时电压 / 吸光度曲线")
                        font.bold: true
                        font.pointSize: ScreenTools.defaultFontPointSize
                    }

                    Rectangle {
                        radius: _m * 0.4
                        color: root._mutedColor(qgcPal.windowShade, 0.22)
                        border.width: 1
                        border.color: root._mutedColor(qgcPal.text, 0.12)
                        Layout.alignment: Qt.AlignVCenter
                        implicitWidth: pointCountLabel.implicitWidth + _m * 0.9
                        implicitHeight: pointCountLabel.implicitHeight + _m * 0.45

                        QGCLabel {
                            id: pointCountLabel
                            anchors.centerIn: parent
                            text: voltageSeries.count + qsTr(" 点")
                            font.pointSize: ScreenTools.smallFontPointSize
                            opacity: 0.75
                        }
                    }

                    Item { Layout.fillWidth: true }

                    QGCButton {
                        text: qsTr("清空")
                        pointSize: ScreenTools.smallFontPointSize
                        onClicked: root.clearAllData()
                    }

                    QGCButton {
                        text: root._chartPaused ? qsTr("恢复") : qsTr("暂停")
                        pointSize: ScreenTools.smallFontPointSize
                        onClicked: root._chartPaused = !root._chartPaused
                    }
                }

                ChartView {
                    id: chartView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    theme: ChartView.ChartThemeDark
                    antialiasing: true
                    animationOptions: ChartView.NoAnimation
                    backgroundColor: "transparent"
                    plotAreaColor: root._mutedColor(qgcPal.window, 0.10)
                    backgroundRoundness: _m * SDTokens.Tokens.radius.sm
                    legend.visible: true
                    legend.alignment: Qt.AlignBottom
                    legend.font.pointSize: ScreenTools.smallFontPointSize

                    ValueAxis {
                        id: axisX
                        min: 0
                        max: SDTokens.Tokens.chart.maxPoints
                        titleText: qsTr("采样点")
                        titleFont.pointSize: ScreenTools.smallFontPointSize
                        labelsFont.pointSize: ScreenTools.smallFontPointSize
                        labelsColor: qgcPal.text
                        gridVisible: true
                        gridLineColor: root._mutedColor(qgcPal.text, 0.10)
                        lineVisible: false
                        tickCount: 7
                    }

                    ValueAxis {
                        id: axisVoltage
                        min: -1
                        max: 1
                        titleText: qsTr("电压 (V)")
                        titleFont.pointSize: ScreenTools.smallFontPointSize
                        labelsFont.family: ScreenTools.fixedFontFamily
                        labelsFont.pointSize: ScreenTools.smallFontPointSize
                        labelsColor: SDTokens.Tokens.chart.voltageColor
                        gridVisible: true
                        gridLineColor: root._mutedColor(qgcPal.text, 0.08)
                        lineVisible: false
                        tickCount: 5
                    }

                    ValueAxis {
                        id: axisAbsorbance
                        min: -1
                        max: 1
                        titleText: qsTr("吸光度 (AU)")
                        titleFont.pointSize: ScreenTools.smallFontPointSize
                        labelsFont.family: ScreenTools.fixedFontFamily
                        labelsFont.pointSize: ScreenTools.smallFontPointSize
                        labelsColor: SDTokens.Tokens.chart.absorbanceColor
                        gridVisible: false
                        lineVisible: false
                        tickCount: 5
                    }

                    LineSeries {
                        id: voltageSeries
                        name: qsTr("电压")
                        color: SDTokens.Tokens.chart.voltageColor
                        width: 2
                        axisX: axisX
                        axisY: axisVoltage
                    }

                    LineSeries {
                        id: absorbanceSeries
                        name: qsTr("吸光度")
                        color: SDTokens.Tokens.chart.absorbanceColor
                        width: 2
                        axisX: axisX
                        axisYRight: axisAbsorbance
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    radius: _m * SDTokens.Tokens.radius.sm
                    color: root._mutedColor(qgcPal.windowShade, 0.18)
                    border.width: 1
                    border.color: root._mutedColor(qgcPal.text, 0.08)
                    implicitHeight: currentValuesRow.implicitHeight + _m * 0.9

                    RowLayout {
                        id: currentValuesRow
                        anchors.fill: parent
                        anchors.margins: _m * 0.6
                        spacing: _m * SDTokens.Tokens.spacing.xl

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: _m * 0.15

                            QGCLabel {
                                text: qsTr("当前电压")
                                font.pointSize: ScreenTools.smallFontPointSize
                                opacity: SDTokens.Tokens.opacity.subtle
                            }

                            QGCLabel {
                                text: root._formatPrecision(root._valueOrDefault(root._voltageFact, NaN), 5) + (_voltageFact ? " V" : "")
                                color: SDTokens.Tokens.chart.voltageColor
                                font.bold: true
                                font.pointSize: ScreenTools.largeFontPointSize
                                font.family: ScreenTools.fixedFontFamily
                            }
                        }

                        Rectangle {
                            width: 1
                            Layout.fillHeight: true
                            color: root._mutedColor(qgcPal.text, 0.12)
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: _m * 0.15

                            QGCLabel {
                                text: qsTr("当前吸光度")
                                font.pointSize: ScreenTools.smallFontPointSize
                                opacity: SDTokens.Tokens.opacity.subtle
                            }

                            QGCLabel {
                                text: root._formatPrecision(root._valueOrDefault(root._absorbanceFact, NaN), 4) + (_absorbanceFact ? " AU" : "")
                                color: SDTokens.Tokens.chart.absorbanceColor
                                font.bold: true
                                font.pointSize: ScreenTools.largeFontPointSize
                                font.family: ScreenTools.fixedFontFamily
                            }
                        }
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            spacing: _m * SDTokens.Tokens.spacing.lg

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: parent.height * 0.3
                radius: _m * SDTokens.Tokens.radius.md
                color: root._cardColor()
                border.width: 1
                border.color: root._cardBorderColor()

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: _m * SDTokens.Tokens.spacing.lg
                    spacing: _m * SDTokens.Tokens.spacing.md

                    RowLayout {
                        Layout.fillWidth: true

                        QGCLabel {
                            text: qsTr("采样任务概览")
                            font.bold: true
                        }

                        Item { Layout.fillWidth: true }

                        Rectangle {
                            width: _m * 0.9
                            height: width
                            radius: width / 2
                            color: root._linkColor()
                        }

                        QGCLabel {
                            text: _linkOk ? qsTr("在线") : qsTr("离线")
                            color: root._linkColor()
                            font.bold: true
                            font.pointSize: ScreenTools.smallFontPointSize
                        }
                    }

                    GridLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        columns: 2
                        columnSpacing: _m * SDTokens.Tokens.spacing.lg
                        rowSpacing: _m * SDTokens.Tokens.spacing.sm

                        QGCLabel { text: qsTr("状态"); opacity: SDTokens.Tokens.opacity.subtle }
                        QGCLabel {
                            text: SDTokens.statusText(payloadStatus)
                            color: root._statusColor(payloadStatus)
                            font.bold: true
                        }

                        QGCLabel { text: qsTr("步骤"); opacity: SDTokens.Tokens.opacity.subtle }
                        QGCLabel {
                            text: qsTr("步骤 %1/%2").arg(Math.max(0, Math.round(root._valueOrDefault(root._stepCurrentFact, 0)))).arg(Math.max(0, Math.round(root._valueOrDefault(root._stepTotalFact, 0))))
                            font.bold: true
                        }

                        QGCLabel { text: qsTr("采样数"); opacity: SDTokens.Tokens.opacity.subtle }
                        QGCLabel {
                            text: String(Math.max(0, Math.round(root._valueOrDefault(root._sampleCountFact, 0))))
                            font.bold: true
                        }

                        QGCLabel { text: qsTr("数据包"); opacity: SDTokens.Tokens.opacity.subtle }
                        QGCLabel {
                            text: String(Math.max(0, Math.round(root._valueOrDefault(root._packetCountFact, 0))))
                            font.bold: true
                        }

                        QGCLabel { text: qsTr("时长"); opacity: SDTokens.Tokens.opacity.subtle }
                        QGCLabel {
                            text: root._formatDuration(root._elapsedSeconds)
                            font.bold: true
                            font.family: ScreenTools.fixedFontFamily
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: parent.height * 0.35
                radius: _m * SDTokens.Tokens.radius.md
                color: root._cardColor()
                border.width: 1
                border.color: root._cardBorderColor()

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: _m * SDTokens.Tokens.spacing.lg
                    spacing: _m * SDTokens.Tokens.spacing.md

                    RowLayout {
                        Layout.fillWidth: true

                        QGCLabel {
                            text: qsTr("泵组状态与 PID")
                            font.bold: true
                        }

                        Item { Layout.fillWidth: true }

                        Rectangle {
                            width: _m * 0.9
                            height: width
                            radius: width / 2
                            color: root._linkColor()
                        }

                        QGCLabel {
                            text: _linkOk ? qsTr("链路正常") : qsTr("链路断开")
                            color: root._linkColor()
                            font.bold: true
                            font.pointSize: ScreenTools.smallFontPointSize
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        radius: _m * SDTokens.Tokens.radius.sm
                        color: root._mutedColor(qgcPal.windowShade, 0.18)
                        border.width: 1
                        border.color: root._mutedColor(qgcPal.text, 0.08)
                        implicitHeight: pumpGrid.implicitHeight + _m * 0.9

                        GridLayout {
                            id: pumpGrid
                            anchors.fill: parent
                            anchors.margins: _m * 0.55
                            columns: 2
                            columnSpacing: _m * SDTokens.Tokens.spacing.lg
                            rowSpacing: _m * SDTokens.Tokens.spacing.sm

                            Repeater {
                                model: [
                                    { label: "X", fact: root._pumpXFact },
                                    { label: "Y", fact: root._pumpYFact },
                                    { label: "Z", fact: root._pumpZFact },
                                    { label: "A", fact: root._pumpAFact }
                                ]

                                delegate: RowLayout {
                                    Layout.fillWidth: true
                                    spacing: _m * SDTokens.Tokens.spacing.sm

                                    QGCLabel {
                                        text: qsTr("泵 ") + modelData.label
                                        opacity: SDTokens.Tokens.opacity.subtle
                                        font.pointSize: ScreenTools.smallFontPointSize
                                    }

                                    Item { Layout.fillWidth: true }

                                    QGCLabel {
                                        text: root._formatPump(modelData.fact)
                                        font.bold: true
                                        font.family: ScreenTools.fixedFontFamily
                                    }

                                    QGCLabel {
                                        text: "°"
                                        opacity: SDTokens.Tokens.opacity.subtle
                                    }
                                }
                            }
                        }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: _m * SDTokens.Tokens.spacing.lg
                        rowSpacing: _m * SDTokens.Tokens.spacing.sm

                        QGCLabel { text: qsTr("PID 模式"); opacity: SDTokens.Tokens.opacity.subtle }
                        QGCLabel {
                            text: SDTokens.pidModeText(root._valueOrDefault(root._pidModeFact, SDTokens.PidIdle))
                            color: root._pidModeColor(root._valueOrDefault(root._pidModeFact, SDTokens.PidIdle))
                            font.bold: true
                        }

                        QGCLabel { text: qsTr("PID 误差"); opacity: SDTokens.Tokens.opacity.subtle }
                        QGCLabel {
                            text: root._formatFixed(root._valueOrDefault(root._pidErrorFact, NaN), 3)
                            font.bold: true
                            font.family: ScreenTools.fixedFontFamily
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: _m * SDTokens.Tokens.radius.md
                color: root._cardColor()
                border.width: 1
                border.color: root._cardBorderColor()

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: _m * SDTokens.Tokens.spacing.lg
                    spacing: _m * SDTokens.Tokens.spacing.md

                    RowLayout {
                        Layout.fillWidth: true

                        QGCLabel {
                            text: qsTr("统计分析")
                            font.bold: true
                        }

                        Item { Layout.fillWidth: true }

                        QGCButton {
                            text: qsTr("清空统计")
                            pointSize: ScreenTools.smallFontPointSize
                            onClicked: root.clearAllData()
                        }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: _m * SDTokens.Tokens.spacing.lg
                        rowSpacing: _m * SDTokens.Tokens.spacing.md

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: _m * SDTokens.Tokens.radius.sm
                            color: root._mutedColor(qgcPal.windowShade, 0.18)
                            border.width: 1
                            border.color: root._mutedColor(qgcPal.text, 0.08)

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: _m * 0.6
                                spacing: _m * SDTokens.Tokens.spacing.sm

                                QGCLabel {
                                    text: qsTr("电压")
                                    color: SDTokens.Tokens.chart.voltageColor
                                    font.bold: true
                                }

                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 2
                                    columnSpacing: _m * SDTokens.Tokens.spacing.md
                                    rowSpacing: _m * SDTokens.Tokens.spacing.xs

                                    QGCLabel { text: qsTr("最小值"); opacity: SDTokens.Tokens.opacity.subtle; font.pointSize: ScreenTools.smallFontPointSize }
                                    QGCLabel { text: root._voltageCount > 0 ? root._formatPrecision(root._voltageMinStat, 5) : "--"; font.bold: true; font.family: ScreenTools.fixedFontFamily; font.pointSize: ScreenTools.smallFontPointSize }
                                    QGCLabel { text: qsTr("最大值"); opacity: SDTokens.Tokens.opacity.subtle; font.pointSize: ScreenTools.smallFontPointSize }
                                    QGCLabel { text: root._voltageCount > 0 ? root._formatPrecision(root._voltageMaxStat, 5) : "--"; font.bold: true; font.family: ScreenTools.fixedFontFamily; font.pointSize: ScreenTools.smallFontPointSize }
                                    QGCLabel { text: qsTr("平均值"); opacity: SDTokens.Tokens.opacity.subtle; font.pointSize: ScreenTools.smallFontPointSize }
                                    QGCLabel { text: root._voltageCount > 0 ? root._formatPrecision(root._statAvg(root._voltageSum, root._voltageCount), 5) : "--"; font.bold: true; font.family: ScreenTools.fixedFontFamily; font.pointSize: ScreenTools.smallFontPointSize }
                                    QGCLabel { text: qsTr("标准差"); opacity: SDTokens.Tokens.opacity.subtle; font.pointSize: ScreenTools.smallFontPointSize }
                                    QGCLabel { text: root._voltageCount > 0 ? root._formatPrecision(root._statStdDev(root._voltageSum, root._voltageSumSquares, root._voltageCount), 5) : "--"; font.bold: true; font.family: ScreenTools.fixedFontFamily; font.pointSize: ScreenTools.smallFontPointSize }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: _m * SDTokens.Tokens.radius.sm
                            color: root._mutedColor(qgcPal.windowShade, 0.18)
                            border.width: 1
                            border.color: root._mutedColor(qgcPal.text, 0.08)

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: _m * 0.6
                                spacing: _m * SDTokens.Tokens.spacing.sm

                                QGCLabel {
                                    text: qsTr("吸光度")
                                    color: SDTokens.Tokens.chart.absorbanceColor
                                    font.bold: true
                                }

                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 2
                                    columnSpacing: _m * SDTokens.Tokens.spacing.md
                                    rowSpacing: _m * SDTokens.Tokens.spacing.xs

                                    QGCLabel { text: qsTr("最小值"); opacity: SDTokens.Tokens.opacity.subtle; font.pointSize: ScreenTools.smallFontPointSize }
                                    QGCLabel { text: root._absorbanceCount > 0 ? root._formatPrecision(root._absorbanceMinStat, 4) : "--"; font.bold: true; font.family: ScreenTools.fixedFontFamily; font.pointSize: ScreenTools.smallFontPointSize }
                                    QGCLabel { text: qsTr("最大值"); opacity: SDTokens.Tokens.opacity.subtle; font.pointSize: ScreenTools.smallFontPointSize }
                                    QGCLabel { text: root._absorbanceCount > 0 ? root._formatPrecision(root._absorbanceMaxStat, 4) : "--"; font.bold: true; font.family: ScreenTools.fixedFontFamily; font.pointSize: ScreenTools.smallFontPointSize }
                                    QGCLabel { text: qsTr("平均值"); opacity: SDTokens.Tokens.opacity.subtle; font.pointSize: ScreenTools.smallFontPointSize }
                                    QGCLabel { text: root._absorbanceCount > 0 ? root._formatPrecision(root._statAvg(root._absorbanceSum, root._absorbanceCount), 4) : "--"; font.bold: true; font.family: ScreenTools.fixedFontFamily; font.pointSize: ScreenTools.smallFontPointSize }
                                    QGCLabel { text: qsTr("标准差"); opacity: SDTokens.Tokens.opacity.subtle; font.pointSize: ScreenTools.smallFontPointSize }
                                    QGCLabel { text: root._absorbanceCount > 0 ? root._formatPrecision(root._statStdDev(root._absorbanceSum, root._absorbanceSumSquares, root._absorbanceCount), 4) : "--"; font.bold: true; font.family: ScreenTools.fixedFontFamily; font.pointSize: ScreenTools.smallFontPointSize }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Timer {
        id: sampleTimer
        interval: SDTokens.Tokens.chart.sampleIntervalMs
        repeat: true
        running: root._hasPayloadGroup && !root._chartPaused && root._shouldChart
        onTriggered: {
            if (!root._hasPayloadGroup) {
                return
            }

            var voltageValue = root._valueOrDefault(root._voltageFact, 0)
            var absorbanceValue = root._valueOrDefault(root._absorbanceFact, 0)
            root._appendSample(voltageValue, absorbanceValue)
            root._updateStatistics(voltageValue, absorbanceValue)
        }
    }

    Timer {
        id: durationTimer
        interval: 1000
        repeat: true
        running: root._hasPayloadGroup
        onTriggered: {
            if (root.payloadStatus === SDTokens.StatusSampling && root._samplingStartedMs > 0) {
                root._elapsedSeconds = Math.floor((Date.now() - root._samplingStartedMs) / 1000)
            } else {
                root._elapsedSeconds = 0
            }
        }
    }
}
