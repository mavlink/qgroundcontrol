/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtCharts                 2.0
import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.ScreenTools   1.0


QGCView {
    id:         freqCalibration
    viewPanel:  panel
    property var _qgcView: qgcView
    property real __xMaxNum: 3
    property real __yminNum: 47000
    property real __ymaxNum: 47820
    property real __xcurrent: 0
    property bool __isAoto:true


    QGCPalette { id: qgcPal; colorGroupEnabled: panel.enabled }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent
        Rectangle {
            id:              logwindow
            anchors.fill:    parent
            anchors.margins: ScreenTools.defaultFontPixelWidth
            color:           qgcPal.window

            ChartView{
                id: chartView
                objectName: "FreqChartView"
                anchors.top:     parent.top
                anchors.left:    parent.left
                anchors.right:   parent.right
                anchors.bottom:  parent.bottom//writeButton.top
                theme: ChartView.ChartThemeBrownSand
                antialiasing: true
                legend.visible: false
                backgroundColor:qgcPal.window


                LineSeries {
                    id: currentLine
                    width:5
                    color: "#FFFF0000"
                }

                Grid{
                    id:     grid
                    anchors.top:     parent.top
                    anchors.topMargin: 69
                    anchors.left:    parent.left
                    anchors.leftMargin: 222
                    anchors.right:   parent.right
                    anchors.rightMargin: 30
                    anchors.bottom:  parent.bottom
                    anchors.bottomMargin: 102

                    rows: 116;
                    columns: __xMaxNum;
                    rowSpacing: 0;
                    columnSpacing: 0;

                    flow: Grid.TopToBottom;
                    Repeater {
                        id:rep
                        model : pD2dInforData.getModelList()
                        Rectangle {
                            color: modelData.color;
                            width: (parent.width - 56)/__xMaxNum;
                            height: parent.height/116
                        }
                    }
                }
             }
            //update data
            Connections{
                target: pD2dInforData;
                onSignalList:{
                    __yminNum = pD2dInforData.getYMinValue();
                    __ymaxNum = pD2dInforData.getYMaxValue();
                    chartView.axisY().min = __yminNum;
                    chartView.axisY().max = __ymaxNum;


                    __xcurrent = pD2dInforData.getCurrentNumValue(0);

                    currentLine.clear();
                    for(var i = 0;i < pD2dInforData.getCurrentNumList();i++)
                    {
                        currentLine.append(i, pD2dInforData.getCurrentNumValue(i));
                        currentLine.append(i+1, pD2dInforData.getCurrentNumValue(i));
                    }
                    rep.model = pD2dInforData.getModelList();
                }
            }
            Component.onCompleted: {
                chartView.axisX().min = 0;
                chartView.axisX().max = __xMaxNum;
                chartView.axisY().min = __yminNum;
                chartView.axisY().max = __ymaxNum;

                __yminNum = pD2dInforData.getYMinValue();
                __ymaxNum = pD2dInforData.getYMaxValue();
            }
        }
    } // QGCViewPanel
} // QGCView

