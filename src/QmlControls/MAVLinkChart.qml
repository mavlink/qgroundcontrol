import QtQuick                      2.11
import QtQuick.Controls             2.4
import QtQuick.Layouts              1.11
import QtCharts                     2.3

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.ScreenTools   1.0

ChartView {
    id:                 chartView
    theme:              ChartView.ChartThemeDark
    antialiasing:       true
    animationOptions:   ChartView.NoAnimation
    legend.visible:     false
    backgroundColor:    qgcPal.window
    backgroundRoundness: 0
    margins.bottom:     ScreenTools.defaultFontPixelHeight * 1.5
    margins.top:        chartHeader.height + (ScreenTools.defaultFontPixelHeight * 2)

    property var chartController:   null
    property var seriesColors:      ["#00E04B","#DE8500","#F32836","#BFBFBF","#536DFF","#EECC44"]

    function addDimension(field) {
        if(!chartController) {
            chartController = controller.createChart()
        }
        var color   = chartView.seriesColors[chartView.count]
        var serie   = createSeries(ChartView.SeriesTypeLine, field.label)
        serie.axisX = axisX
        serie.axisY = axisY
        serie.useOpenGL = true
        serie.color = color
        serie.width = 1
        chartController.addSeries(field, serie)
    }

    function delDimension(field) {
        if(chartController) {
            chartView.removeSeries(field.series)
            chartController.delSeries(field)
            if(chartView.count === 0) {
                controller.deleteChart(chartController)
                chartController = null
            }
        }
    }

    DateTimeAxis {
        id:                         axisX
        min:                        chartController ? chartController.rangeXMin : new Date()
        max:                        chartController ? chartController.rangeXMax : new Date()
        visible:                    chartController !== null
        format:                     "<br/>mm:ss.zzz"
        tickCount:                  5
        gridVisible:                true
        labelsFont.family:          "Fixed"
        labelsFont.pointSize:       ScreenTools.smallFontPointSize
        labelsColor:                qgcPal.text
    }

    ValueAxis {
        id:                         axisY
        min:                        chartController ? chartController.rangeYMin : 0
        max:                        chartController ? chartController.rangeYMax : 0
        visible:                    chartController !== null
        lineVisible:                false
        labelsFont.family:          "Fixed"
        labelsFont.pointSize:       ScreenTools.smallFontPointSize
        labelsColor:                qgcPal.text
    }

    Row {
        id:                         chartHeader
        anchors.left:               parent.left
        anchors.leftMargin:         ScreenTools.defaultFontPixelWidth  * 4
        anchors.right:              parent.right
        anchors.rightMargin:        ScreenTools.defaultFontPixelWidth  * 4
        anchors.top:                parent.top
        anchors.topMargin:          ScreenTools.defaultFontPixelHeight * 1.5
        spacing:                    ScreenTools.defaultFontPixelWidth  * 2
        visible:                    chartController !== null
        GridLayout {
            columns:                2
            columnSpacing:          ScreenTools.defaultFontPixelWidth
            rowSpacing:             ScreenTools.defaultFontPixelHeight * 0.25
            anchors.verticalCenter: parent.verticalCenter
            QGCLabel {
                text:               qsTr("Scale:");
                Layout.alignment:   Qt.AlignVCenter
            }
            QGCComboBox {
                Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 10
                Layout.maximumWidth: ScreenTools.defaultFontPixelWidth * 10
                height:             ScreenTools.defaultFontPixelHeight
                model:              controller.timeScales
                currentIndex:       chartController ? chartController.rangeXIndex : 0
                onActivated:        { if(chartController) chartController.rangeXIndex = index; }
                Layout.alignment:   Qt.AlignVCenter
            }
            QGCLabel {
                text:               qsTr("Range:");
                Layout.alignment:   Qt.AlignVCenter
            }
            QGCComboBox {
                Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 10
                Layout.maximumWidth: ScreenTools.defaultFontPixelWidth * 10
                height:             ScreenTools.defaultFontPixelHeight
                model:              controller.rangeList
                currentIndex:       chartController ? chartController.rangeYIndex : 0
                onActivated:        { if(chartController) chartController.rangeYIndex = index; }
                Layout.alignment:   Qt.AlignVCenter
            }
        }
        ColumnLayout {
            anchors.verticalCenter: parent.verticalCenter
            Repeater {
                model:              chartController ? chartController.chartFields : []
                QGCLabel {
                    text:           modelData.label
                    color:          chartView.series(index).color
                    font.pointSize: ScreenTools.smallFontPointSize
                }
            }
        }
    }
}
