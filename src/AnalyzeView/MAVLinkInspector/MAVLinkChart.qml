import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtGraphs

import QGroundControl
import QGroundControl.Controls

GraphsView {
    id:                 chartView
    marginBottom:       ScreenTools.defaultFontPixelHeight * 1.5
    marginTop:          chartHeader.height + (ScreenTools.defaultFontPixelHeight * 2)
    marginLeft:         0
    marginRight:        0
    visible:            chartController.chartFields.length > 0

    required property var inspectorController
    required property int chartIndex

    property var _seriesColors: [qgcPal.colorGreen, qgcPal.colorOrange, qgcPal.colorRed, qgcPal.colorGrey, qgcPal.colorBlue, qgcPal.colorYellow]

    Component {
        id: lineSeriesComponent
        LineSeries { }
    }

    function addDimension(field) {
        var color   = _seriesColors[chartView.seriesList.length]
        var serie   = lineSeriesComponent.createObject(chartView, {color: color, width: 1})
        chartView.addSeries(serie)
        chartController.addSeries(field, serie)
    }

    function delDimension(field) {
        if(chartController) {
            for (var i = 0; i < chartView.seriesList.length; i++) {
                if (chartView.seriesList[i] === field.series) {
                    var s = chartView.seriesList[i]
                    chartView.removeSeries(s)
                    chartController.delSeries(field)
                    s.destroy()
                    break
                }
            }
        }
    }

    function roomForNewDimension() {
        return chartController.chartFields.length < _seriesColors.length
    }

    theme: GraphsTheme {
        colorScheme:                qgcPal.globalTheme === QGCPalette.Light ? GraphsTheme.ColorScheme.Light : GraphsTheme.ColorScheme.Dark
        backgroundColor:            qgcPal.window
        backgroundVisible:          true
        plotAreaBackgroundColor:     qgcPal.window
        grid.mainColor:             Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.3)
        grid.subColor:              Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.15)
        grid.mainWidth:             1
        labelBackgroundVisible:     false
        labelTextColor:             qgcPal.text
        axisXLabelFont.family:      ScreenTools.fixedFontFamily
        axisXLabelFont.pointSize:   ScreenTools.smallFontPointSize
        axisYLabelFont.family:      ScreenTools.fixedFontFamily
        axisYLabelFont.pointSize:   ScreenTools.smallFontPointSize
    }

    MAVLinkChartController {
        id:                     chartController
        inspectorController:    chartView.inspectorController
        chartIndex:             chartView.chartIndex
    }

    axisX: ValueAxis {
        id:                         axisX
        min:                        chartController ? chartController.rangeXMin : 0
        max:                        chartController ? chartController.rangeXMax : 1
        visible:                    chartController !== null
        tickInterval:               chartController ? chartController.rangeXMs / 3 : 5000
        subTickCount:               0
        labelsVisible:              true
        labelDelegate: Component {
            Item {
                property string text
                implicitWidth:  label.implicitWidth
                implicitHeight: label.implicitHeight
                Text {
                    id: label
                    text: {
                        var ms = parseFloat(parent.text)
                        if (isNaN(ms)) return parent.text
                        var d = new Date(ms)
                        return d.getMinutes().toString().padStart(2, '0') + ":" + d.getSeconds().toString().padStart(2, '0')
                    }
                    color:          qgcPal.text
                    font.family:    ScreenTools.fixedFontFamily
                    font.pointSize: ScreenTools.smallFontPointSize
                }
            }
        }
    }

    axisY: ValueAxis {
        id:                         axisY
        min:                        chartController ? chartController.rangeYMin : 0
        max:                        chartController ? chartController.rangeYMax : 0
        visible:                    chartController !== null
        lineVisible:                false
    }

    Row {
        id:                         chartHeader
        anchors.left:               parent ? parent.left : undefined
        anchors.leftMargin:         ScreenTools.defaultFontPixelWidth  * 4
        anchors.right:              parent ? parent.right : undefined
        anchors.rightMargin:        ScreenTools.defaultFontPixelWidth  * 4
        anchors.top:                parent ? parent.top : undefined
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
                model:              inspectorController.timeScales
                currentIndex:       chartController ? chartController.rangeXIndex : 0
                onActivated: (index) => { if(chartController) chartController.rangeXIndex = index; }
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
                model:              inspectorController.rangeList
                currentIndex:       chartController ? chartController.rangeYIndex : 0
                onActivated: (index) => { if(chartController) chartController.rangeYIndex = index; }
                Layout.alignment:   Qt.AlignVCenter
            }
        }
        ColumnLayout {
            anchors.verticalCenter: parent.verticalCenter
            Repeater {
                model:              chartController ? chartController.chartFields : []
                QGCLabel {
                    text:           modelData.label
                    color:          index < chartView.seriesList.length ? chartView.seriesList[index].color : qgcPal.text
                    font.pointSize: ScreenTools.smallFontPointSize
                }
            }
        }
    }
}
