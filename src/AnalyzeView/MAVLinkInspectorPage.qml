/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Window
import QtCharts

import QGroundControl
import QGroundControl.Palette
import QGroundControl.Controls
import QGroundControl.Controllers
import QGroundControl.ScreenTools

AnalyzePage {
    id: root
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

    function updateEnabledStatus(repeater, message, chart) {
        if(!message) {
            return;
        }

        for (let i = 0; i < repeater.count; i++) {
            let checkBox = repeater.itemAt(i)
            if(!checkBox) {
                continue
            }
            const object = message.fields.get(i)
            checkBox.enabled = isCheckboxEnabled(checkBox, object, chart)
        }
    }

    function isCheckboxEnabled(checkBox, object, chart) {
        if(checkBox.checkState === Qt.Checked) {
            return true
        }
        if(!object.selectable) {
            return false
        }
        if(object.series !== null) {
            return false
        }
        if(chart.chartController !== null) {
            if (chart.chartController.chartFields.length >= chart.seriesColors.length) {
                return false
            }
        }
        return true
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
                    onActivated: (index) =>  { controller.setActiveSystem(controller.systems.get(index).id) }

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
                    onActivated: (index) => {
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
                            compID:     object.compId
                            checked:    curSystem ? (curSystem.selected === index) : false
                            messageHz:  object.actualRateHz
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
                visible:            curMessage !== null && (curCompID === 0 || curCompID === curMessage.compId)
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
                            text: qsTr("Message:")
                            Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 20
                        }
                        QGCLabel {
                            color: qgcPal.buttonHighlight
                            text: curMessage ? curMessage.name + ' (' + curMessage.id + ')' : ""
                        }

                        QGCLabel { text: qsTr("Component:") }
                        QGCLabel { text: curMessage ? curMessage.compId : "" }

                        QGCLabel { text: qsTr("Count:") }
                        QGCLabel { text: curMessage ? curMessage.count : "" }

                        QGCLabel { text: qsTr("Actual Rate:") }
                        QGCLabel { text: curMessage ? curMessage.actualRateHz.toFixed(1) + qsTr("Hz") : "" }

                        QGCLabel { text: qsTr("Set Rate:") }
                        QGCComboBox {
                            id: msgRateCombo
                            textRole: "text"
                            valueRole: "value"
                            model: [
                                { value: -1, text: qsTr("Disabled") },
                                { value: 0, text: qsTr("Default") },
                                { value: 1, text: qsTr("1Hz") },
                                { value: 2, text: qsTr("2Hz") },
                                { value: 3, text: qsTr("3Hz") },
                                { value: 4, text: qsTr("4Hz") },
                                { value: 5, text: qsTr("5Hz") },
                                { value: 6, text: qsTr("6Hz") },
                                { value: 7, text: qsTr("7Hz") },
                                { value: 8, text: qsTr("8Hz") },
                                { value: 9, text: qsTr("9Hz") },
                                { value: 10, text: qsTr("10Hz") },
                                { value: 25, text: qsTr("25Hz") },
                                { value: 50, text: qsTr("50Hz") },
                                { value: 100, text: qsTr("100Hz") }
                            ]
                            Layout.alignment: Qt.AlignLeft
                            sizeToContents: true
                            Component.onCompleted: reset()
                            onActivated: (index) => controller.setMessageInterval(currentValue)
                            function reset() { currentIndex = indexOfValue(0) }
                            Connections {
                                target: root
                                function onCurMessageChanged() { msgRateCombo.reset() }
                            }
                            Connections {
                                target: curMessage
                                function onTargetRateHzChanged() {
                                    const target_index = indexOfValue(curMessage.targetRateHz)
                                    if(target_index != -1) {
                                        currentIndex = target_index
                                    }
                                }
                            }
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
                            id: chart1Repeater
                            model:      curMessage ? curMessage.fields : []
                            delegate:   QGCCheckBox {
                                Layout.row:         index + 2
                                Layout.column:      3
                                Layout.alignment:   Qt.AlignHCenter
                                checked:            object.series !== null && object.chartIndex === 0
                                onClicked: {
                                    if(checked) {
                                        chart1.addDimension(object)
                                    } else {
                                        chart1.delDimension(object)
                                    }
                                    updateEnabledStatus(chart1Repeater, curMessage, chart1)
                                    updateEnabledStatus(chart2Repeater, curMessage, chart2)
                                }
                                Component.onCompleted: updateEnabledStatus(chart1Repeater, curMessage, chart1)
                            }
                        }
                        Repeater {
                            id: chart2Repeater
                            model:      curMessage ? curMessage.fields : []
                            delegate:   QGCCheckBox {
                                Layout.row:         index + 2
                                Layout.column:      4
                                Layout.alignment:   Qt.AlignHCenter
                                checked:            object.series !== null && object.chartIndex === 1
                                onClicked: {
                                    if(checked) {
                                        chart2.addDimension(object)
                                    } else {
                                        chart2.delDimension(object)
                                    }
                                    updateEnabledStatus(chart2Repeater, curMessage, chart2)
                                    updateEnabledStatus(chart1Repeater, curMessage, chart1)
                                }
                                Component.onCompleted: updateEnabledStatus(chart2Repeater, curMessage, chart2)
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
