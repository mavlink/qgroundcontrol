import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtGraphs

import QGroundControl
import QGroundControl.Controls

ColumnLayout {
    id: chartView
    visible: chartController.chartFields.length > 0
    spacing: ScreenTools.defaultFontPixelHeight

    required property var inspectorController
    required property int chartIndex

    property var _seriesColors: [qgcPal.colorGreen, qgcPal.colorOrange, qgcPal.colorRed, qgcPal.colorGrey, qgcPal.colorBlue, qgcPal.colorYellow]

    Component {
        id: lineSeriesComponent
        LineSeries { }
    }

    function addDimension(field) {
        if (!roomForNewDimension()) return
        var color   = _seriesColors[_graphsView.seriesList.length]
        var serie   = lineSeriesComponent.createObject(_graphsView, {color: color, width: 1, axisX: axisX, axisY: axisY})
        _graphsView.addSeries(serie)
        chartController.addSeries(field, serie)
    }

    function delDimension(field) {
        for (var i = 0; i < _graphsView.seriesList.length; i++) {
            if (_graphsView.seriesList[i] === field.series) {
                var s = _graphsView.seriesList[i]
                _graphsView.removeSeries(s)
                chartController.delSeries(field)
                // Do not call s.destroy() here. QGraphsView holds an internal
                // pointer to the series (via QPointer) that is only cleared
                // during the next updatePolish() pass. Destroying the series
                // before that pass — whether immediately or via Qt.callLater —
                // causes a SIGSEGV in QGraphsView::updatePolish(). The series
                // is parented to _graphsView so it is freed when the chart is
                // closed. GPU/graph resources are released by removeSeries().
                break
            }
        }
    }

    function roomForNewDimension() {
        return chartController.chartFields.length < _seriesColors.length
    }

    MAVLinkChartController {
        id: chartController
        inspectorController: chartView.inspectorController
        chartIndex: chartView.chartIndex
        plotPixelWidth: Math.max(1, Math.floor(_graphsView.plotArea.width))
    }

    // -------------------------------------------------------------------------
    // Header: Scale / Range controls + active field labels
    // -------------------------------------------------------------------------
    Row {
        spacing: ScreenTools.defaultFontPixelWidth

        ColumnLayout {
            LabelledComboBox {
                label: qsTr("Scale")
                model: inspectorController.timeScales
                currentIndex: chartController.rangeXIndex
                onActivated: (index) => { chartController.rangeXIndex = index }
            }

            LabelledComboBox {
                label: qsTr("Range")
                model: inspectorController.rangeList
                currentIndex: chartController.rangeYIndex
                onActivated: (index) => { chartController.rangeYIndex = index }
            }
        }

        ColumnLayout {
            spacing: 0

            Repeater {
                model: chartController.chartFields

                QGCLabel {
                    text: modelData.label
                    color: index < _graphsView.seriesList.length ? _graphsView.seriesList[index].color : qgcPal.text
                    font.pointSize: ScreenTools.smallFontPointSize
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // Chart
    // -------------------------------------------------------------------------
    GraphsView {
        id: _graphsView
        Layout.fillWidth: true
        Layout.fillHeight: true
        marginBottom: ScreenTools.defaultFontPixelHeight * 1.5
        marginTop: 0
        marginLeft: ScreenTools.defaultFontPixelWidth // Without this the Y axis tick labels are cut off
        marginRight: 0

        theme: GraphsTheme {
            colorScheme: qgcPal.globalTheme === QGCPalette.Light ? GraphsTheme.ColorScheme.Light : GraphsTheme.ColorScheme.Dark
            backgroundColor: qgcPal.window
            backgroundVisible: true
            plotAreaBackgroundColor: qgcPal.window
            grid.mainColor: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.3)
            grid.subColor: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.15)
            grid.mainWidth: 1
            labelBackgroundVisible: false
            labelTextColor: qgcPal.text
            axisXLabelFont.family: ScreenTools.fixedFontFamily
            axisXLabelFont.pointSize: ScreenTools.smallFontPointSize
            axisYLabelFont.family: ScreenTools.fixedFontFamily
            axisYLabelFont.pointSize: ScreenTools.smallFontPointSize
        }

        axisX: ValueAxis {
            id: axisX
            min: chartController.rangeXMin
            max: chartController.rangeXMax
            tickInterval: chartController.rangeXMs / 3
            subTickCount: 0
            labelsVisible: true
            labelDelegate: Component {
                Item {
                    property string text
                    implicitWidth: label.implicitWidth
                    implicitHeight: label.implicitHeight
                    Text {
                        id: label
                        text: {
                            var ms = parseFloat(parent.text)
                            if (isNaN(ms)) return parent.text
                            var d = new Date(ms)
                            return d.getMinutes().toString().padStart(2, '0') + ":" + d.getSeconds().toString().padStart(2, '0')
                        }
                        color: qgcPal.text
                        font.family: ScreenTools.fixedFontFamily
                        font.pointSize: ScreenTools.smallFontPointSize
                    }
                }
            }
        }

        axisY: ValueAxis {
            id: axisY
            min: chartController.rangeYMin
            max: chartController.rangeYMax
            lineVisible: false
        }
    }
}
