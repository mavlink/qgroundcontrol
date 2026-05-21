import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

import "USVSamplingDataTokens.js" as SDTokens
import "USVFlyViewLayout.js" as USVLayout

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
    property var _baselineSetFact:      _hasPayloadGroup ? _activeVehicle.getFact("usvPayload.baselineSet")      : null
    property var _referenceVoltageFact: _hasPayloadGroup ? _activeVehicle.getFact("usvPayload.referenceVoltage") : null
    property var _baselineVoltageFact:  _hasPayloadGroup ? _activeVehicle.getFact("usvPayload.baselineVoltage")  : null

    property real _m: ScreenTools.defaultFontPixelWidth
    property real _h: ScreenTools.defaultFontPixelHeight

    property bool _linkOk: _linkActiveFact ? Number(_linkActiveFact.value) === 1 : false
    property int payloadStatus: _statusFact ? Number(_statusFact.value) : SDTokens.StatusIdle
    property bool baselineSet: _baselineSetFact ? Number(_baselineSetFact.value) >= 1 : false
    // 只有在活跃工作状态时才向曲线追加数据点，避免非采样时持续累积导致内存增长和 heap 904
    property bool _shouldChart: payloadStatus === SDTokens.StatusSampling
                                || payloadStatus === SDTokens.StatusDetecting
                                || payloadStatus === SDTokens.StatusCalibrating
                                || payloadStatus === SDTokens.StatusWaitingStable
                                || payloadStatus === SDTokens.StatusSurveying
    property bool _shouldSampleAbsorbance: USVLayout.shouldSampleAbsorbance(payloadStatus, baselineSet)

    property bool _chartPaused: false
    property bool _pageActive: true
    property int _sampleIndex: 0
    property var _voltagePoints: []
    property var _absorbancePoints: []
    property int _axisXMin: 0
    property int _axisXMax: SDTokens.Tokens.chart.maxPoints
    property real _axisVoltageMin: -1
    property real _axisVoltageMax: 1
    property real _axisAbsorbanceMin: -1
    property real _axisAbsorbanceMax: 1

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

    function _requestChartPaint() {
        if (!root._pageActive) {
            return
        }

        chartCanvas.requestPaint()
    }

    function prepareForUnload() {
        _pageActive = false
        sampleTimer.stop()
        durationTimer.stop()
        _voltagePoints = []
        _absorbancePoints = []
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
        case SDTokens.StatusSurveying:
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

    function _cssColor(baseColor, alpha) {
        return "rgba("
                + Math.round(baseColor.r * 255) + ","
                + Math.round(baseColor.g * 255) + ","
                + Math.round(baseColor.b * 255) + ","
                + alpha + ")"
    }

    function _pointsAxisRange(points, fallbackCenter) {
        if (points.length <= 0) {
            return { min: fallbackCenter - 1, max: fallbackCenter + 1 }
        }

        var minVal = points[0].y
        var maxVal = minVal
        for (var i = 1; i < points.length; i++) {
            var y = points[i].y
            if (y < minVal) {
                minVal = y
            }
            if (y > maxVal) {
                maxVal = y
            }
        }

        var range = maxVal - minVal
        var margin = range > 0 ? range * SDTokens.Tokens.chart.marginPercent : Math.max(Math.abs(minVal) * SDTokens.Tokens.chart.marginPercent, 0.001)
        return { min: minVal - margin, max: maxVal + margin }
    }

    function _updateChartAxes() {
        var voltageRange = _pointsAxisRange(_voltagePoints, 0)
        var absorbanceRange = _pointsAxisRange(_absorbancePoints, 0)
        _axisXMin = Math.max(0, _sampleIndex - SDTokens.Tokens.chart.maxPoints)
        _axisXMax = Math.max(SDTokens.Tokens.chart.maxPoints, _sampleIndex)
        _axisVoltageMin = voltageRange.min
        _axisVoltageMax = voltageRange.max
        _axisAbsorbanceMin = absorbanceRange.min
        _axisAbsorbanceMax = absorbanceRange.max
    }

    function _appendSample(voltageValue, absorbanceValue, includeAbsorbance) {
        if (!_pageActive) {
            return
        }

        var nextVoltagePoints = _voltagePoints.slice()
        nextVoltagePoints.push({ x: _sampleIndex, y: voltageValue })
        if (nextVoltagePoints.length > SDTokens.Tokens.chart.maxPoints) {
            nextVoltagePoints.shift()
        }
        _voltagePoints = nextVoltagePoints

        if (includeAbsorbance) {
            var nextAbsorbancePoints = _absorbancePoints.slice()
            nextAbsorbancePoints.push({ x: _sampleIndex, y: absorbanceValue })
            if (nextAbsorbancePoints.length > SDTokens.Tokens.chart.maxPoints) {
                nextAbsorbancePoints.shift()
            }
            _absorbancePoints = nextAbsorbancePoints
        }

        _sampleIndex += 1

        _updateChartAxes()
        _requestChartPaint()
    }

    function _updateStatistics(voltageValue, absorbanceValue, includeAbsorbance) {
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

        if (!includeAbsorbance) {
            return
        }

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
        _voltagePoints = []
        _absorbancePoints = []
        _sampleIndex = 0
        _axisXMin = 0
        _axisXMax = SDTokens.Tokens.chart.maxPoints
        _axisVoltageMin = -1
        _axisVoltageMax = 1
        _axisAbsorbanceMin = -1
        _axisAbsorbanceMax = 1

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
        _requestChartPaint()
    }

    onPayloadStatusChanged: {
        if (payloadStatus === SDTokens.StatusSampling || payloadStatus === SDTokens.StatusSurveying) {
            _samplingStartedMs = Date.now()
            _elapsedSeconds = 0
        } else {
            _samplingStartedMs = 0
            _elapsedSeconds = 0
        }
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

                    QGCLabel {
                        text: root.baselineSet ? qsTr("Baseline OK") : qsTr("未设基线")
                        color: root.baselineSet ? qgcPal.colorGreen : qgcPal.colorOrange
                        font.bold: true
                        font.pointSize: ScreenTools.smallFontPointSize
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
                            text: root._voltagePoints.length + qsTr(" 点")
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

                Item {
                    id: chartArea
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: _h * 12
                    clip: true

                    Rectangle {
                        anchors.fill: parent
                        radius: _m * SDTokens.Tokens.radius.sm
                        color: root._mutedColor(qgcPal.window, 0.10)
                        border.width: 1
                        border.color: root._mutedColor(qgcPal.text, 0.08)
                    }

                    Canvas {
                        id: chartCanvas
                        anchors.fill: parent
                        anchors.margins: _m * 0.45

                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            if (width <= 0 || height <= 0) {
                                return
                            }

                            var left = Math.max(46, root._m * 5.2)
                            var right = root.baselineSet ? Math.max(44, root._m * 4.8) : Math.max(18, root._m * 1.8)
                            var top = Math.max(18, root._h * 0.75)
                            var bottom = Math.max(32, root._h * 1.8)
                            var plotW = Math.max(1, width - left - right)
                            var plotH = Math.max(1, height - top - bottom)
                            var plotRight = left + plotW
                            var plotBottom = top + plotH
                            var xRange = Math.max(1, root._axisXMax - root._axisXMin)

                            function clamp(value, minValue, maxValue) {
                                return Math.max(minValue, Math.min(maxValue, value))
                            }

                            function mapX(x) {
                                return left + ((x - root._axisXMin) / xRange) * plotW
                            }

                            function mapY(y, minValue, maxValue) {
                                var yRange = maxValue - minValue
                                if (Math.abs(yRange) < 0.000001) {
                                    return top + plotH / 2
                                }
                                return plotBottom - ((y - minValue) / yRange) * plotH
                            }

                            function drawGrid() {
                                ctx.save()
                                ctx.lineWidth = 1
                                ctx.strokeStyle = root._cssColor(qgcPal.text, 0.10)
                                for (var i = 0; i <= 6; i++) {
                                    var x = left + plotW * i / 6
                                    ctx.beginPath()
                                    ctx.moveTo(x, top)
                                    ctx.lineTo(x, plotBottom)
                                    ctx.stroke()
                                }
                                for (var j = 0; j <= 4; j++) {
                                    var y = top + plotH * j / 4
                                    ctx.beginPath()
                                    ctx.moveTo(left, y)
                                    ctx.lineTo(plotRight, y)
                                    ctx.stroke()
                                }
                                ctx.strokeStyle = root._cssColor(qgcPal.text, 0.30)
                                ctx.strokeRect(left, top, plotW, plotH)
                                ctx.restore()
                            }

                            function drawAxisLabels() {
                                var labelSize = Math.max(10, Math.round(root._h * 0.72))
                                ctx.save()
                                ctx.font = labelSize + "px sans-serif"
                                ctx.textBaseline = "middle"
                                ctx.textAlign = "right"
                                ctx.fillStyle = SDTokens.Tokens.chart.voltageColor
                                ctx.fillText(root._formatPrecision(root._axisVoltageMax, 4), left - 8, top)
                                ctx.fillText(root._formatPrecision(root._axisVoltageMin, 4), left - 8, plotBottom)

                                if (root.baselineSet) {
                                    ctx.textAlign = "left"
                                    ctx.fillStyle = SDTokens.Tokens.chart.absorbanceColor
                                    ctx.fillText(root._formatPrecision(root._axisAbsorbanceMax, 4), plotRight + 8, top)
                                    ctx.fillText(root._formatPrecision(root._axisAbsorbanceMin, 4), plotRight + 8, plotBottom)
                                }

                                ctx.textAlign = "center"
                                ctx.fillStyle = root._cssColor(qgcPal.text, 0.72)
                                ctx.fillText(root._axisXMin + " - " + root._axisXMax + qsTr(" 点"), left + plotW / 2, height - labelSize)
                                ctx.restore()
                            }

                            function drawSeries(points, minValue, maxValue, color) {
                                if (points.length <= 0) {
                                    return
                                }

                                ctx.save()
                                ctx.strokeStyle = color
                                ctx.fillStyle = color
                                ctx.lineWidth = Math.max(1.5, root._m * 0.18)
                                ctx.beginPath()
                                for (var k = 0; k < points.length; k++) {
                                    var x = clamp(mapX(points[k].x), left, plotRight)
                                    var y = clamp(mapY(points[k].y, minValue, maxValue), top, plotBottom)
                                    if (k === 0) {
                                        ctx.moveTo(x, y)
                                    } else {
                                        ctx.lineTo(x, y)
                                    }
                                }
                                ctx.stroke()

                                if (points.length === 1) {
                                    var singleX = clamp(mapX(points[0].x), left, plotRight)
                                    var singleY = clamp(mapY(points[0].y, minValue, maxValue), top, plotBottom)
                                    ctx.beginPath()
                                    ctx.arc(singleX, singleY, Math.max(2, root._m * 0.28), 0, Math.PI * 2)
                                    ctx.fill()
                                }
                                ctx.restore()
                            }

                            drawGrid()
                            drawSeries(root._voltagePoints, root._axisVoltageMin, root._axisVoltageMax, SDTokens.Tokens.chart.voltageColor)
                            if (root.baselineSet) {
                                drawSeries(root._absorbancePoints, root._axisAbsorbanceMin, root._axisAbsorbanceMax, SDTokens.Tokens.chart.absorbanceColor)
                            }
                            drawAxisLabels()
                        }
                    }

                    QGCLabel {
                        anchors.centerIn: parent
                        text: root._chartPaused ? qsTr("曲线已暂停") : qsTr("等待采样数据")
                        opacity: 0.45
                        font.pointSize: ScreenTools.smallFontPointSize
                        visible: root._voltagePoints.length === 0
                    }

                    RowLayout {
                        anchors.left: parent.left
                        anchors.leftMargin: _m * 1.2
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: _m * 0.45
                        spacing: _m * SDTokens.Tokens.spacing.md

                        RowLayout {
                            spacing: _m * 0.35
                            Rectangle {
                                width: _m * 1.1
                                height: _m * 0.32
                                radius: height / 2
                                color: SDTokens.Tokens.chart.voltageColor
                            }
                            QGCLabel {
                                text: qsTr("电压")
                                font.pointSize: ScreenTools.smallFontPointSize
                                opacity: 0.78
                            }
                        }

                        RowLayout {
                            spacing: _m * 0.35
                            visible: root.baselineSet
                            Rectangle {
                                width: _m * 1.1
                                height: _m * 0.32
                                radius: height / 2
                                color: SDTokens.Tokens.chart.absorbanceColor
                            }
                            QGCLabel {
                                text: qsTr("吸光度")
                                font.pointSize: ScreenTools.smallFontPointSize
                                opacity: 0.78
                            }
                        }
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
                                text: root.baselineSet ? root._formatPrecision(root._valueOrDefault(root._absorbanceFact, NaN), 4) + (_absorbanceFact ? " AU" : "") : qsTr("未设基线")
                                color: root.baselineSet ? SDTokens.Tokens.chart.absorbanceColor : qgcPal.colorOrange
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

                        QGCLabel { text: qsTr("Baseline"); opacity: SDTokens.Tokens.opacity.subtle }
                        QGCLabel {
                            text: root.baselineSet ? qsTr("已设置") : qsTr("未设基线")
                            color: root.baselineSet ? qgcPal.colorGreen : qgcPal.colorOrange
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
        running: root._pageActive && root._hasPayloadGroup && !root._chartPaused && root._shouldChart
        onTriggered: {
            if (!root._pageActive || !root._hasPayloadGroup) {
                return
            }

            var voltageValue = root._valueOrDefault(root._voltageFact, 0)
            var absorbanceValue = root._valueOrDefault(root._absorbanceFact, 0)
            var includeAbsorbance = root._shouldSampleAbsorbance
            root._appendSample(voltageValue, absorbanceValue, includeAbsorbance)
            root._updateStatistics(voltageValue, absorbanceValue, includeAbsorbance)
        }
    }

    Timer {
        id: durationTimer
        interval: 1000
        repeat: true
        running: root._pageActive && root._hasPayloadGroup
        onTriggered: {
            if (!root._pageActive) {
                return
            }

            if ((root.payloadStatus === SDTokens.StatusSampling || root.payloadStatus === SDTokens.StatusSurveying) && root._samplingStartedMs > 0) {
                root._elapsedSeconds = Math.floor((Date.now() - root._samplingStartedMs) / 1000)
            } else {
                root._elapsedSeconds = 0
            }
        }
    }
}
