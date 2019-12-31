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

    property int  maxSeriesCount:    seriesColors.length
    property var  seriesColors:      ["chartreuse", "chocolate", "yellowgreen", "thistle", "silver", "darkturquoise", "blue", "green"]
    property alias max: axisY.max
    property alias min: axisY.min

    function addDimension(field, left) {
        console.log(field.name + ' AxisY: ' + axisY)
        console.log(chartView.count + ' ' + chartView.seriesColors[chartView.count])
        var serie   = createSeries(ChartView.SeriesTypeLine, field.label)
        serie.axisX = axisX
        serie.axisY = axisY
        serie.useOpenGL = true
        serie.color = chartView.seriesColors[chartView.count]
        serie.width = 1
        controller.addSeries(field, serie, left)
    }

    function delDimension(field) {
        chartView.removeSeries(field.series)
        controller.delSeries(field)
        console.log('Remove: ' + chartView.count + ' ' + field.name)
    }

    DateTimeAxis {
        id:             axisX
        min:            controller.rangeXMin
        max:            controller.rangeXMax
        format:         "mm:ss.zzz"
        tickCount:      5
        gridVisible:    true
        labelsFont.family:      "Fixed"
        labelsFont.pixelSize:   ScreenTools.smallFontPointSize
    }

    ValueAxis {
        id:             axisY
        lineVisible:    false
        labelsFont.family:      "Fixed"
        labelsFont.pixelSize:   ScreenTools.smallFontPointSize
    }

    RowLayout {
        id:                     chartHeader
        anchors.left:           parent.left
        anchors.leftMargin:     ScreenTools.defaultFontPixelWidth  * 4
        anchors.right:          parent.right
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth  * 4
        anchors.top:            parent.top
        anchors.topMargin:      ScreenTools.defaultFontPixelHeight * 1.5
        spacing:                0
        QGCLabel {
            text:               qsTr("Scale:");
            font.pixelSize:     ScreenTools.smallFontPointSize
            Layout.alignment:   Qt.AlignVCenter
        }
        QGCComboBox {
            id:                 timeScaleSelector
            width:              ScreenTools.defaultFontPixelWidth  * 10
            height:             ScreenTools.defaultFontPixelHeight
            model:              controller.timeScales
            currentIndex:       controller.timeScale
            onActivated:        controller.timeScale = index
            font.pixelSize:     ScreenTools.smallFontPointSize
            Layout.alignment:   Qt.AlignVCenter
        }
        GridLayout {
            columns:            2
            columnSpacing:      ScreenTools.defaultFontPixelWidth
            rowSpacing:         ScreenTools.defaultFontPixelHeight * 0.25
            Layout.alignment:   Qt.AlignRight | Qt.AlignVCenter
            Layout.fillWidth:   true
            QGCLabel {
                text:               qsTr("Range Left:");
                font.pixelSize:     ScreenTools.smallFontPointSize
                Layout.alignment:   Qt.AlignVCenter
            }
            QGCComboBox {
                Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 8
                height:             ScreenTools.defaultFontPixelHeight * 1.5
                model:              controller.rangeList
                currentIndex:       controller.leftRangeIdx
                onActivated:        controller.leftRangeIdx = index
                font.pixelSize:     ScreenTools.smallFontPointSize
                Layout.alignment:   Qt.AlignVCenter
            }
            QGCLabel {
                text:               qsTr("Range Right:");
                font.pixelSize:     ScreenTools.smallFontPointSize
                Layout.alignment:   Qt.AlignVCenter
            }
            QGCComboBox {
                Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 8
                height:             ScreenTools.defaultFontPixelHeight * 1.5
                model:              controller.rangeList
                currentIndex:       controller.rightRangeIdx
                onActivated:        controller.rightRangeIdx = index
                font.pixelSize:     ScreenTools.smallFontPointSize
                Layout.alignment:   Qt.AlignVCenter
            }
        }
    }
}
