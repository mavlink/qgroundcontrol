/****************************************************************************
 *
 * USV Sampling Chart Panel
 * 无人船采样数据实时曲线面板 - 电压/吸光度滚动时间序列
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtCharts

import QGroundControl
import QGroundControl.Controls

import "USVFlyViewLayout.js" as USVLayout

Rectangle {
    id: root

    property var vehicle: null
    property bool isExpanded: false

    property real _m: ScreenTools.defaultFontPixelWidth

    // ========== 图表配置 ==========
    readonly property int _maxPoints: 120      // 60s × 2Hz
    readonly property int _sampleMs: 500       // 采样间隔
    readonly property color _voltageColor: "#3b82f6"
    readonly property color _absorbanceColor: "#f59e0b"

    // ========== Fact 缓存 ==========
    property bool _hasPayloadGroup: vehicle && vehicle.factGroupNames.indexOf("usvPayload") >= 0
    property var _voltageFact:    _hasPayloadGroup ? vehicle.getFact("usvPayload.voltage")    : null
    property var _absorbanceFact: _hasPayloadGroup ? vehicle.getFact("usvPayload.absorbance") : null
    property var _linkActiveFact: _hasPayloadGroup ? vehicle.getFact("usvPayload.linkActive") : null
    property bool _linkOk: _linkActiveFact ? _linkActiveFact.value === 1 : false

    // ========== 统计属性 ==========
    property real _vMin: 0
    property real _vMax: 0
    property real _vAvg: 0
    property int _pointCount: 0

    width: _m * 32
    height: isExpanded ? _m * 28 : 0
    radius: _m * USVLayout.Tokens.radius.md
    color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, USVLayout.Tokens.opacity.panel)
    border.width: 1
    border.color: Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.45)
    clip: true
    visible: height > 0

    Behavior on height { NumberAnimation { duration: 250; easing.type: Easing.InOutQuad } }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    // ========== 数据采样定时器 ==========
    Timer {
        id: sampleTimer
        interval: root._sampleMs
        repeat: true
        running: root.isExpanded && root._linkOk
        onTriggered: {
            var v = root._voltageFact ? root._voltageFact.value : 0
            var a = root._absorbanceFact ? root._absorbanceFact.value : 0
            var idx = root._pointCount

            // 滚动窗口
            if (voltageSeries.count >= root._maxPoints) {
                voltageSeries.remove(0)
                absorbanceSeries.remove(0)
            }

            voltageSeries.append(idx, v)
            absorbanceSeries.append(idx, a)
            root._pointCount = idx + 1

            // 更新 X 轴范围
            var xMin = Math.max(0, root._pointCount - root._maxPoints)
            axisX.min = xMin
            axisX.max = root._pointCount

            // 自适应 Y 轴（电压）
            updateVoltageAxis()
        }
    }

    function updateVoltageAxis() {
        if (voltageSeries.count === 0) return
        var min = voltageSeries.at(0).y
        var max = min
        var sum = 0
        for (var i = 0; i < voltageSeries.count; i++) {
            var val = voltageSeries.at(i).y
            if (val < min) min = val
            if (val > max) max = val
            sum += val
        }
        var range = max - min
        var margin = range > 0 ? range * 0.15 : 0.001
        axisVoltage.min = min - margin
        axisVoltage.max = max + margin
        root._vMin = min
        root._vMax = max
        root._vAvg = sum / voltageSeries.count
    }

    function clearChart() {
        voltageSeries.clear()
        absorbanceSeries.clear()
        root._pointCount = 0
        root._vMin = 0
        root._vMax = 0
        root._vAvg = 0
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: _m * 1.2
        spacing: _m * 0.5

        // 标题栏
        RowLayout {
            Layout.fillWidth: true
            spacing: _m

            QGCLabel {
                text: qsTr("采样数据趋势")
                font.bold: true
                font.pointSize: ScreenTools.smallFontPointSize
            }
            Item { Layout.fillWidth: true }
            QGCLabel {
                text: root._pointCount + qsTr(" 点")
                font.pointSize: ScreenTools.smallFontPointSize
                opacity: 0.6
            }
            QGCButton {
                text: qsTr("清空")
                pointSize: ScreenTools.smallFontPointSize
                onClicked: root.clearChart()
            }
        }

        // 图表
        ChartView {
            id: chartView
            Layout.fillWidth: true
            Layout.fillHeight: true
            theme: ChartView.ChartThemeDark
            antialiasing: true
            animationOptions: ChartView.NoAnimation
            legend.visible: true
            legend.alignment: Qt.AlignBottom
            legend.font.pointSize: ScreenTools.smallFontPointSize
            backgroundColor: "transparent"
            plotAreaColor: Qt.rgba(0, 0, 0, 0.1)
            backgroundRoundness: 4

            ValueAxis {
                id: axisX
                min: 0
                max: root._maxPoints
                labelsVisible: false
                gridVisible: true
                gridLineColor: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.1)
                lineVisible: false
            }

            ValueAxis {
                id: axisVoltage
                min: 0
                max: 1
                titleText: qsTr("电压 (V)")
                titleFont.pointSize: ScreenTools.smallFontPointSize
                labelsFont.family: ScreenTools.fixedFontFamily
                labelsFont.pointSize: ScreenTools.smallFontPointSize
                labelsColor: root._voltageColor
                gridLineColor: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.08)
                lineVisible: false
                tickCount: 5
            }

            ValueAxis {
                id: axisAbsorbance
                min: 0
                max: 3
                titleText: qsTr("吸光度")
                titleFont.pointSize: ScreenTools.smallFontPointSize
                labelsFont.family: ScreenTools.fixedFontFamily
                labelsFont.pointSize: ScreenTools.smallFontPointSize
                labelsColor: root._absorbanceColor
                gridVisible: false
                lineVisible: false
                tickCount: 5
            }

            LineSeries {
                id: voltageSeries
                name: qsTr("电压")
                color: root._voltageColor
                width: 2
                axisX: axisX
                axisY: axisVoltage
            }

            LineSeries {
                id: absorbanceSeries
                name: qsTr("吸光度")
                color: root._absorbanceColor
                width: 2
                axisX: axisX
                axisYRight: axisAbsorbance
            }
        }

        // 底部统计栏
        RowLayout {
            Layout.fillWidth: true
            spacing: _m * 2

            Repeater {
                model: [
                    { label: qsTr("当前"), value: root._voltageFact ? root._voltageFact.value.toPrecision(5) : "--", clr: root._voltageColor },
                    { label: qsTr("最小"), value: root._vMin.toPrecision(4), clr: root._voltageColor },
                    { label: qsTr("最大"), value: root._vMax.toPrecision(4), clr: root._voltageColor },
                    { label: qsTr("平均"), value: root._vAvg.toPrecision(4), clr: root._voltageColor },
                    { label: qsTr("吸光度"), value: root._absorbanceFact ? root._absorbanceFact.value.toPrecision(4) : "--", clr: root._absorbanceColor }
                ]
                delegate: RowLayout {
                    spacing: _m * 0.3
                    QGCLabel {
                        text: modelData.label
                        font.pointSize: ScreenTools.smallFontPointSize
                        opacity: 0.5
                    }
                    QGCLabel {
                        text: modelData.value
                        font.pointSize: ScreenTools.smallFontPointSize
                        font.bold: true
                        font.family: ScreenTools.fixedFontFamily
                        color: modelData.clr
                    }
                }
            }
        }
    }
}
