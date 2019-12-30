/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                      2.11
import QtQuick.Controls             2.4
import QtQuick.Layouts              1.11
import QtQuick.Dialogs              1.3
import QtQuick.Window               2.2
import QtCharts                     2.3

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.ScreenTools   1.0

AnalyzePage {
    headerComponent:    headerComponent
    pageComponent:      pageComponent

    property var    curVehicle:         controller ? controller.activeVehicle : null
    property int    curMessageIndex:    0
    property var    curMessage:         curVehicle && curVehicle.messages.count ? curVehicle.messages.get(curMessageIndex) : null
    property int    curCompID:          0
    property bool   selectionValid:     false
    property real   maxButtonWidth:     0

    MAVLinkInspectorController {
        id: controller
    }

    /*
    Window {
        id:             chartWindow
        width:          ScreenTools.defaultFontPixelWidth  * 80
        height:         ScreenTools.defaultFontPixelHeight * 20
        visible:        true
        title:          "Chart"
        Rectangle {
            color:          qgcPal.window
            anchors.fill:   parent
            QGCComboBox {
                id:             timeScaleSelector
                anchors.margins: ScreenTools.defaultFontPixelWidth
                anchors.right:  parent.right
                anchors.top:    parent.top
                width:          ScreenTools.defaultFontPixelWidth  * 10
                height:         ScreenTools.defaultFontPixelHeight * 1.5
                model:          controller.timeScales
                currentIndex:   controller.timeScale
                onActivated:    controller.timeScale = index
            }
            ChartView {
                id:             chartView
                anchors.right:  parent.right
                anchors.left:   parent.left
                anchors.top:    timeScaleSelector.bottom
                anchors.bottom: parent.bottom
                theme:          ChartView.ChartThemeDark
                antialiasing:   true
                animationOptions: ChartView.NoAnimation
                legend.font.pixelSize: ScreenTools.smallFontPointSize
                margins.bottom: ScreenTools.defaultFontPixelHeight * 1.5

                DateTimeAxis {
                    id:             axisX
                    min:            visible ? controller.rangeXMin : new Date()
                    max:            visible ? controller.rangeXMax : new Date()
                    visible:        controller.chartFieldCount > 0
                    format:         "mm:ss"
                    tickCount:      5
                    gridVisible:    true
                    labelsFont.pixelSize: ScreenTools.smallFontPointSize
                }

                ValueAxis {
                    id:             axisY1
                    min:            visible ? controller.chartFields[0].rangeMin : 0
                    max:            visible ? controller.chartFields[0].rangeMax : 0
                    visible:        controller.chartFieldCount > 0
                    lineVisible:    false
                    labelsFont.pixelSize: ScreenTools.smallFontPointSize
                }

                ValueAxis {
                    id:             axisY2
                    min:            visible ? controller.chartFields[1].rangeMin : 0
                    max:            visible ? controller.chartFields[1].rangeMax : 0
                    visible:        controller.chartFieldCount > 1
                    lineVisible:    false
                    labelsFont.pixelSize: ScreenTools.smallFontPointSize
                }

                LineSeries {
                    id:             lineSeries1
                    name:           controller.chartFieldCount ? controller.chartFields[0].label : ""
                    axisX:          axisX
                    axisY:          axisY1
                    color:          qgcPal.colorRed
                    useOpenGL:      true
                }

                LineSeries {
                    id:             lineSeries2
                    name:           controller.chartFieldCount > 1 ? controller.chartFields[1].label : ""
                    axisX:          axisX
                    axisYRight:     axisY2
                    color:          qgcPal.colorGreen
                    useOpenGL:      true
                }

            }
            Timer {
                id:         refreshTimer
                interval:   1 / 20 * 1000 // 20 Hz
                running:    controller.chartFieldCount > 0
                repeat:     true
                onTriggered: {
                    if(controller.chartFieldCount > 0) {
                        controller.updateSeries(0, lineSeries1)
                    }
                    if(controller.chartFieldCount > 1) {
                        controller.updateSeries(1, lineSeries2)
                    } else {
                        if(lineSeries2.count > 0) {
                            lineSeries2.removePoints(0,lineSeries2.count)
                        }
                    }
                }
                onRunningChanged: {
                    if(!running) {
                        if(lineSeries1.count > 0) {
                            lineSeries1.removePoints(0,lineSeries1.count)
                        }
                    }
                }
            }
        }
    }
    */

    Component {
        id:  headerComponent
        //-- Header
        RowLayout {
            id:                 header
            anchors.left:       parent.left
            anchors.right:      parent.right
            QGCLabel {
                text:           qsTr("Inspect real time MAVLink messages.")
            }
            RowLayout {
                Layout.alignment:   Qt.AlignRight
                visible:            curVehicle ? curVehicle.compIDsStr.length > 2 : false
                QGCLabel {
                    text:           qsTr("Component ID:")
                }
                QGCComboBox {
                    id:             cidCombo
                    model:          curVehicle ? curVehicle.compIDsStr : []
                    Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 10
                    currentIndex:   0
                    onActivated: {
                        if(curVehicle && curVehicle.compIDsStr.length > 1) {
                            selectionValid = false
                            if(index < 1)
                                curCompID = 0
                            else
                                curCompID = curVehicle.compIDs[index - 1]
                        }
                    }
                }
            }
        }
    }

    Component {
        id:                         pageComponent
        RowLayout {
            width:                  availableWidth
            height:                 availableHeight
            spacing:                ScreenTools.defaultFontPixelWidth
            //-- Messages (Buttons)
            QGCFlickable {
                id:                 buttonGrid
                width:              maxButtonWidth
                Layout.maximumWidth:maxButtonWidth
                Layout.fillHeight:  true
                Layout.fillWidth:   true
                contentWidth:       width
                contentHeight:      buttonCol.height
                ColumnLayout {
                    id:             buttonCol
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    spacing:        ScreenTools.defaultFontPixelHeight * 0.25
                    Repeater {
                        model:      curVehicle ? curVehicle.messages : []
                        delegate:   MAVLinkMessageButton {
                            text:       object.name + (object.selected ?  " *" : "")
                            compID:     object.cid
                            checked:    curMessageIndex === index
                            messageHz:  object.messageHz
                            visible:    curCompID === 0 || curCompID === compID
                            onClicked: {
                                selectionValid  = true
                                curMessageIndex = index
                            }
                            Layout.fillWidth: true
                        }
                    }
                }
            }
            //-- Message Data
            QGCFlickable {
                id:                 messageGrid
                visible:            curMessage !== null && selectionValid
                Layout.fillHeight:  true
                Layout.fillWidth:   true
                contentWidth:       messageCol.width
                contentHeight:      messageCol.height
                ColumnLayout {
                    id:                 messageCol
                    spacing:            ScreenTools.defaultFontPixelHeight * 0.25
                    GridLayout {
                        columns:        2
                        columnSpacing:  ScreenTools.defaultFontPixelWidth
                        rowSpacing:     ScreenTools.defaultFontPixelHeight * 0.25
                        QGCLabel {
                            text:       qsTr("Message:")
                            Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 20
                        }
                        QGCLabel {
                            color:      qgcPal.buttonHighlight
                            text:       curMessage ? curMessage.name + ' (' + curMessage.id + ') ' + curMessage.messageHz.toFixed(1) + 'Hz' : ""
                        }
                        QGCLabel {
                            text:       qsTr("Component:")
                        }
                        QGCLabel {
                            text:       curMessage ? curMessage.cid : ""
                        }
                        QGCLabel {
                            text:       qsTr("Count:")
                        }
                        QGCLabel {
                            text:       curMessage ? curMessage.count : ""
                        }
                    }
                    Item { height: ScreenTools.defaultFontPixelHeight; width: 1 }
                    QGCLabel {
                        text:       qsTr("Message Fields:")
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        height:     1
                        color:      qgcPal.text
                    }
                    Item { height: ScreenTools.defaultFontPixelHeight * 0.25; width: 1 }
                    GridLayout {
                        columns:        4
                        columnSpacing:  ScreenTools.defaultFontPixelWidth
                        rowSpacing:     ScreenTools.defaultFontPixelHeight * 0.25
                        Repeater {
                            model:      curMessage ? curMessage.fields : []
                            delegate:   QGCLabel {
                                Layout.row:         index
                                Layout.column:      0
                                Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 20
                                text:               object.name
                            }
                        }
                        Repeater {
                            model:      curMessage ? curMessage.fields : []
                            delegate:   QGCLabel {
                                Layout.row:         index
                                Layout.column:      1
                                Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 30
                                Layout.maximumWidth: ScreenTools.defaultFontPixelWidth * 30
                                text:               object.value
                            }
                        }
                        Repeater {
                            model:      curMessage ? curMessage.fields : []
                            delegate:   QGCLabel {
                                Layout.row:         index
                                Layout.column:      2
                                text:               object.type
                            }
                        }
                        Repeater {
                            model:      curMessage ? curMessage.fields : []
                            delegate:   QGCCheckBox {
                                Layout.row:         index
                                Layout.column:      3
                                enabled:            object.selected || (object.selectable && controller.chartFieldCount < 2)
                                checked:            enabled ? object.selected : false
                                onClicked:          { if(enabled) object.selected = checked }
                            }
                        }
                    }
                    Item { height: ScreenTools.defaultFontPixelHeight * 0.25; width: 1 }
                    ChartView {
                        id:             chartView
                        Layout.fillWidth: true
                        height:         ScreenTools.defaultFontPixelHeight * 20
                        theme:          ChartView.ChartThemeDark
                        antialiasing:   true
                        visible:        controller.chartFieldCount > 0
                        animationOptions: ChartView.NoAnimation
                        legend.visible: false
                        margins.bottom: ScreenTools.defaultFontPixelHeight * 1.5
                        margins.top:    chartHeader.height + (ScreenTools.defaultFontPixelHeight * 2)

                        DateTimeAxis {
                            id:             axisX
                            min:            visible ? controller.rangeXMin : new Date()
                            max:            visible ? controller.rangeXMax : new Date()
                            visible:        controller.chartFieldCount > 0
                            format:         "mm:ss"
                            tickCount:      5
                            gridVisible:    true
                            labelsFont.pixelSize: ScreenTools.smallFontPointSize
                        }

                        ValueAxis {
                            id:             axisY1
                            min:            visible ? controller.chartFields[0].rangeMin : 0
                            max:            visible ? controller.chartFields[0].rangeMax : 0
                            visible:        controller.chartFieldCount > 0
                            lineVisible:    false
                            labelsFont.pixelSize: ScreenTools.smallFontPointSize
                            labelsColor:    qgcPal.colorRed
                        }

                        ValueAxis {
                            id:             axisY2
                            min:            visible ? controller.chartFields[1].rangeMin : 0
                            max:            visible ? controller.chartFields[1].rangeMax : 0
                            visible:        controller.chartFieldCount > 1
                            lineVisible:    false
                            labelsFont.pixelSize: ScreenTools.smallFontPointSize
                            labelsColor:    qgcPal.colorGreen
                        }

                        LineSeries {
                            id:             lineSeries1
                            name:           controller.chartFieldCount ? controller.chartFields[0].label : ""
                            axisX:          axisX
                            axisY:          axisY1
                            color:          qgcPal.colorRed
                            useOpenGL:      true
                        }

                        LineSeries {
                            id:             lineSeries2
                            name:           controller.chartFieldCount > 1 ? controller.chartFields[1].label : ""
                            axisX:          axisX
                            axisYRight:     axisY2
                            color:          qgcPal.colorGreen
                            useOpenGL:      true
                        }

                        Timer {
                            id:         refreshTimer
                            interval:   1 / 20 * 1000 // 20 Hz
                            running:    controller.chartFieldCount > 0
                            repeat:     true
                            onTriggered: {
                                if(controller.chartFieldCount > 0) {
                                    controller.updateSeries(0, lineSeries1)
                                }
                                if(controller.chartFieldCount > 1) {
                                    controller.updateSeries(1, lineSeries2)
                                } else {
                                    if(lineSeries2.count > 0) {
                                        lineSeries2.removePoints(0,lineSeries2.count)
                                    }
                                }
                            }
                            onRunningChanged: {
                                if(!running) {
                                    if(lineSeries1.count > 0) {
                                        lineSeries1.removePoints(0,lineSeries1.count)
                                    }
                                }
                            }
                        }
                        RowLayout {
                            id:                 chartHeader
                            anchors.left:       parent.left
                            anchors.leftMargin: ScreenTools.defaultFontPixelWidth  * 4
                            anchors.right:      parent.right
                            anchors.rightMargin:ScreenTools.defaultFontPixelWidth  * 4
                            anchors.top:        parent.top
                            anchors.topMargin:  ScreenTools.defaultFontPixelHeight * 1.5
                            spacing:            0
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
                                columns:            3
                                columnSpacing:      ScreenTools.defaultFontPixelWidth
                                rowSpacing:         ScreenTools.defaultFontPixelHeight * 0.25
                                Layout.alignment:   Qt.AlignRight | Qt.AlignVCenter
                                Layout.fillWidth:   true
                                Repeater {
                                    model:          controller.chartFieldCount ? controller.chartFields : []
                                    delegate: QGCLabel {
                                        text:               chartView.series(index).name
                                        color:              chartView.series(index).color
                                        font.pixelSize:     ScreenTools.smallFontPointSize
                                        Layout.row:         index
                                        Layout.column:      0
                                        Layout.alignment:   Qt.AlignVCenter
                                    }
                                }
                                Repeater {
                                    model:          controller.chartFieldCount ? controller.chartFields : []
                                    delegate: QGCLabel {
                                        text:               qsTr("Range:");
                                        font.pixelSize:     ScreenTools.smallFontPointSize
                                        Layout.row:         index
                                        Layout.column:      1
                                        Layout.alignment:   Qt.AlignVCenter
                                    }
                                }
                                Repeater {
                                    model:          controller.chartFieldCount ? controller.chartFields : []
                                    delegate: QGCComboBox {
                                        width:              ScreenTools.defaultFontPixelWidth  * 12
                                        height:             ScreenTools.defaultFontPixelHeight * 1.5
                                        model:              modelData.rangeList
                                        currentIndex:       modelData.range
                                        onActivated:        modelData.range = index
                                        font.pixelSize:     ScreenTools.smallFontPointSize
                                        Layout.row:         index
                                        Layout.column:      2
                                        Layout.alignment:   Qt.AlignVCenter
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
