/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
    allowPopout:        true

    property var    curSystem:          controller ? controller.activeSystem : null
    property var    curMessage:         curSystem && curSystem.messages.count ? curSystem.messages.get(curSystem.selected) : null
    property int    curCompID:          0
    property real   maxButtonWidth:     0

    MAVLinkInspectorController {
        id: controller
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
                visible:            curSystem ? controller.systemNames.length > 1 || curSystem.compIDsStr.length > 2 : false
                QGCComboBox {
                    id:             systemCombo
                    model:          controller.systemNames
                    sizeToContents: true
                    visible:        controller.systemNames.length > 1
                    onActivated:    controller.setActiveSystem(controller.systems.get(index).id);

                    Connections {
                        target: controller
                        onActiveSystemChanged: {
                            for (var systemIndex=0; systemIndex<controller.systems.count; systemIndex++) {
                                if (controller.systems.get(systemIndex) == curSystem) {
                                    systemCombo.currentIndex = systemIndex
                                    curCompID = 0
                                    cidCombo.currentIndex = 0
                                    break
                                }
                            }
                        }
                    }
                }
                QGCComboBox {
                    id:             cidCombo
                    model:          curSystem ? curSystem.compIDsStr : []
                    sizeToContents: true
                    visible:        curSystem ? curSystem.compIDsStr.length > 2 : false
                    onActivated: {
                        if(curSystem && curSystem.compIDsStr.length > 1) {
                            if(index < 1)
                                curCompID = 0
                            else
                                curCompID = curSystem.compIDs[index - 1]
                        }
                    }
                }
            }
        }
    }

    Component {
        id:                         pageComponent
        Row {
            width:                  availableWidth
            height:                 availableHeight
            spacing:                ScreenTools.defaultFontPixelWidth
            //-- Messages (Buttons)
            QGCFlickable {
                id:                 buttonGrid
                flickableDirection: Flickable.VerticalFlick
                width:              maxButtonWidth
                height:             parent.height
                contentWidth:       width
                contentHeight:      buttonCol.height
                ColumnLayout {
                    id:             buttonCol
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    spacing:        ScreenTools.defaultFontPixelHeight * 0.25
                    Repeater {
                        model:      curSystem ? curSystem.messages : []
                        delegate:   MAVLinkMessageButton {
                            text:       object.name + (object.fieldSelected ?  " *" : "")
                            compID:     object.cid
                            checked:    curSystem ? (curSystem.selected === index) : false
                            messageHz:  object.messageHz
                            visible:    curCompID === 0 || curCompID === compID
                            onClicked: {
                                curSystem.selected = index
                            }
                            Layout.fillWidth: true
                        }
                    }
                }
            }
            //-- Message Data
            QGCFlickable {
                id:                 messageGrid
                visible:            curMessage !== null && (curCompID === 0 || curCompID === curMessage.cid)
                flickableDirection: Flickable.VerticalFlick
                width:              parent.width - buttonGrid.width - ScreenTools.defaultFontPixelWidth
                height:             parent.height
                contentWidth:       width
                contentHeight:      messageCol.height
                Column {
                    id:                 messageCol
                    width:              parent.width
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
                    //---------------------------------------------------------
                    GridLayout {
                        id:                 msgInfoGrid
                        columns:            5
                        columnSpacing:      ScreenTools.defaultFontPixelWidth  * 0.25
                        rowSpacing:         ScreenTools.defaultFontPixelHeight * 0.25
                        width:              parent.width
                        QGCLabel {
                            text:       qsTr("Name")
                        }
                        QGCLabel {
                            text:       qsTr("Value")
                        }
                        QGCLabel {
                            text:       qsTr("Type")
                        }
                        QGCLabel {
                            text:       qsTr("Plot 1")
                        }
                        QGCLabel {
                            text:       qsTr("Plot 2")
                        }

                        //---------------------------------------------------------
                        Rectangle {
                            Layout.columnSpan:  5
                            Layout.fillWidth:   true
                            height:             1
                            color:              qgcPal.text
                        }
                        //---------------------------------------------------------

                        Repeater {
                            model:      curMessage ? curMessage.fields : []
                            delegate:   QGCLabel {
                                Layout.row:         index + 2
                                Layout.column:      0
                                Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 20
                                text:               object.name
                            }
                        }
                        Repeater {
                            model:      curMessage ? curMessage.fields : []
                            delegate:   QGCLabel {
                                Layout.row:         index + 2
                                Layout.column:      1
                                Layout.minimumWidth: msgInfoGrid.width * 0.25
                                Layout.maximumWidth: msgInfoGrid.width * 0.25
                                text:               object.value
                                elide:              Text.ElideRight
                            }
                        }
                        Repeater {
                            model:      curMessage ? curMessage.fields : []
                            delegate:   QGCLabel {
                                Layout.row:         index + 2
                                Layout.column:      2
                                Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 10
                                text:               object.type
                                elide:              Text.ElideRight
                            }
                        }
                        Repeater {
                            model:      curMessage ? curMessage.fields : []
                            delegate:   QGCCheckBox {
                                Layout.row:         index + 2
                                Layout.column:      3
                                Layout.alignment:   Qt.AlignHCenter
                                enabled: {
                                    if(checked)
                                        return true
                                    if(!object.selectable)
                                        return false
                                    if(object.series !== null)
                                        return false
                                    if(chart1.chartController !== null) {
                                        if(chart1.chartController.chartFields.length >= chart1.seriesColors.length)
                                            return false
                                    }
                                    return true;
                                }
                                checked:            object.series !== null && object.chartIndex === 0
                                onClicked: {
                                    if(checked) {
                                        chart1.addDimension(object)
                                    } else {
                                        chart1.delDimension(object)
                                    }
                                }
                            }
                        }
                        Repeater {
                            model:      curMessage ? curMessage.fields : []
                            delegate:   QGCCheckBox {
                                Layout.row:         index + 2
                                Layout.column:      4
                                Layout.alignment:   Qt.AlignHCenter
                                enabled: {
                                    if(checked)
                                        return true
                                    if(!object.selectable)
                                        return false
                                    if(object.series !== null)
                                        return false
                                    if(chart2.chartController !== null) {
                                        if(chart2.chartController.chartFields.length >= chart2.seriesColors.length)
                                            return false
                                    }
                                    return true;
                                }
                                checked:            object.series !== null && object.chartIndex === 1
                                onClicked: {
                                    if(checked) {
                                        chart2.addDimension(object)
                                    } else {
                                        chart2.delDimension(object)
                                    }
                                }
                            }
                        }
                    }
                    Item { height: ScreenTools.defaultFontPixelHeight * 0.25; width: 1 }
                    MAVLinkChart {
                        id:         chart1
                        height:     ScreenTools.defaultFontPixelHeight * 20
                        width:      parent.width
                    }
                    MAVLinkChart {
                        id:         chart2
                        height:     ScreenTools.defaultFontPixelHeight * 20
                        width:      parent.width
                    }
                }
            }
        }
    }
}
