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
import QtCharts                     2.1

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

    Window {
        id:             chartWindow
        width:          mainWindow.width  * 0.5
        height:         mainWindow.height * 0.5
        visible:        true
        title:          "Chart"
        Rectangle {
            color:          qgcPal.window
            anchors.fill:   parent
            ChartView {
                id:             chartView
                anchors.fill:   parent
                theme:          ChartView.ChartThemeDark
                antialiasing:   true
                animationOptions: ChartView.NoAnimation

                ValueAxis {
                    id:         axisY1
                    min:        visible ? controller.chartFields[0].rangeMin : 0
                    max:        visible ? controller.chartFields[0].rangeMax : 0
                    visible:    controller.chartFieldCount > 0
                }

                ValueAxis {
                    id:         axisY2
                    min:        visible ? controller.chartFields[1].rangeMin : 0
                    max:        visible ? controller.chartFields[1].rangeMax : 0
                    visible:    controller.chartFieldCount > 1
                }

                DateTimeAxis {
                    id:         axisX
                    min:        visible ? controller.rangeXMin : new Date()
                    max:        visible ? controller.rangeXMax : new Date()
                    format:     "hh:mm:ss.zzz"
                    tickCount:  6
                    visible:    controller.chartFieldCount > 0
                }

                LineSeries {
                    id:         lineSeries1
                    name:       controller.chartFieldCount ? controller.chartFields[0].name : ""
                    axisX:      axisX
                    axisY:      axisY1
                    useOpenGL:  true
                }

                LineSeries {
                    id:         lineSeries2
                    name:       controller.chartFieldCount > 1 ? controller.chartFields[1].name : ""
                    axisX:      axisX
                    axisYRight: axisY2
                    useOpenGL:  true
                }

            }
            Timer {
                id:         refreshTimer
                interval:   1 / 30 * 1000 // 30 Hz
                running:    controller.chartFieldCount > 0
                repeat:     true
                onTriggered: {
                    if(controller.chartFieldCount > 0) {
                        controller.updateSeries(0, lineSeries1)
                    }
                    if(controller.chartFieldCount > 1) {
                        controller.updateSeries(1, lineSeries2)
                    }
                }
            }
        }
    }

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
                                wrapMode:           Text.WordWrap
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
                }
            }
        }
    }
}
