import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SetupPage {
    id:             serialPortsPage
    pageComponent:  pageComponent

    property real _margins:         ScreenTools.defaultFontPixelHeight * 0.5
    property real _cellPadding:     ScreenTools.defaultFontPixelWidth * 1.5
    property bool _showInUseOnly:   false

    readonly property int _rowsPerPort: 3   // separator, port settings, conflict warning
    // Reverting or rebooting before earlier writes finish could undo the revert or apply only part of a change
    readonly property bool _pendingWrites: controller.vehicle ? controller.vehicle.parameterManager.pendingWrites : false
    readonly property int _shownPortCount: {
        let count = 0
        for (let i = 0; i < controller.ports.count; i++) {
            if (_portShown(controller.ports.get(i))) {
                count++
            }
        }
        return count
    }

    // shared: several functions' options are shown together, so name the function instance
    function _optionLabel(paramName, shared) {
        const labels = {
            "MODE":         qsTr("Mode"),
            "RATE":         qsTr("Send rate"),
            "FORWARD":      qsTr("Forwarding"),
            "FLOW_CTRL":    qsTr("Flow control"),
            "UDP_PRT":      qsTr("UDP port"),
            "REMOTE_PRT":   qsTr("Remote port"),
            "BROADCAST":    qsTr("Broadcast"),
            "PROTOCOL":     qsTr("Protocol")
        }
        const suffix = paramName.replace(/^[A-Z]+_\d_/, "")
        const label = labels[suffix] !== undefined ? labels[suffix] : paramName
        const instance = paramName.match(/^[A-Z]+_(\d)_/)
        return (shared && instance) ? qsTr("%1 (instance %2)").arg(label).arg(instance[1]) : label
    }

    // Ethernet is the only shared port. PX4 has no baud rate there, so a rate of 0 is a fixed value.
    function _optionNote(paramName, ethernet) {
        const notes = {
            "RATE": ethernet ? qsTr("0 uses 100000 bytes/s.") : qsTr("0 uses half the link bandwidth.")
        }
        const note = notes[paramName.replace(/^[A-Z]+_\d_/, "")]
        return note !== undefined ? note : ""
    }

    // A port whose function was just removed stays shown until the change is reverted or applied
    function _portShown(port) {
        return !_showInUseOnly || port.assignments.length > 0 || port.rebootRequired
    }

    PX4SerialPortsController {
        id: controller
    }

    QGCPalette {
        id: qgcPal
    }

    Component {
        id: pageComponent

        ColumnLayout {
            width:   Math.max(availableWidth, implicitWidth)
            spacing: _margins

            Rectangle {
                Layout.fillWidth:   true
                implicitHeight:     rebootRow.implicitHeight + _margins * 2
                visible:            controller.changedPortCount > 0
                color:              "transparent"
                border.color:       qgcPal.warningText
                radius:             ScreenTools.defaultFontPixelHeight / 4

                RowLayout {
                    id:                 rebootRow
                    anchors.margins:    _margins
                    anchors.fill:       parent
                    spacing:            _margins

                    QGCLabel {
                        Layout.fillWidth:   true
                        wrapMode:           Text.WordWrap
                        color:              qgcPal.warningText
                        text:               controller.changedPortCount === 1 ?
                                                qsTr("1 port changed. Changes take effect after the vehicle reboots.") :
                                                qsTr("%1 ports changed. Changes take effect after the vehicle reboots.").arg(controller.changedPortCount)
                    }

                    QGCButton {
                        text:       qsTr("Revert Changes")
                        enabled:    !_pendingWrites
                        onClicked:  controller.revertChanges()
                    }

                    QGCButton {
                        text:       qsTr("Reboot Vehicle")
                        primary:    true
                        enabled:    controller.vehicle && !controller.vehicle.armed && !_pendingWrites
                        onClicked:  controller.vehicle.rebootVehicle()
                    }
                }
            }

            Rectangle {
                Layout.fillWidth:   true
                implicitWidth:      panelLayout.implicitWidth + _margins * 2
                implicitHeight:     panelLayout.implicitHeight + _margins * 2
                color:              qgcPal.windowPanel
                border.color:       qgcPal.borderSubtle
                radius:             ScreenTools.defaultBorderRadius * 2

                ColumnLayout {
                    id:                 panelLayout
                    anchors.fill:       parent
                    anchors.margins:    _margins
                    spacing:            _margins

                    RowLayout {
                        Layout.fillWidth:   true
                        spacing:            _margins

                        QGCRadioButtonBar {
                            model:          [ qsTr("All Ports"), qsTr("In Use") ]
                            currentIndex:   _showInUseOnly ? 1 : 0
                            onActivated:    (index) => { _showInUseOnly = index === 1 }
                        }

                        QGCLabel {
                            Layout.fillWidth:   true
                            color:              qgcPal.textSecondary
                            text:               _showInUseOnly ?
                                                    qsTr("Showing %1 of %2 ports.").arg(_shownPortCount).arg(controller.ports.count) :
                                                    qsTr("Showing all %1 ports.").arg(controller.ports.count)
                        }
                    }

                    GridLayout {
                        Layout.fillWidth:   true
                        columns:            4
                        columnSpacing:      _margins
                        rowSpacing:         _margins * 0.75

                        Repeater {
                            model: controller.ports

                            Rectangle {
                                Layout.row:         index * _rowsPerPort
                                Layout.column:      0
                                Layout.columnSpan:  4
                                Layout.fillWidth:   true
                                implicitHeight:     1
                                color:              qgcPal.borderSubtle
                                visible:            _portShown(object)

                                required property int index
                                required property var object
                            }
                        }

                        Repeater {
                            model: controller.ports

                            ColumnLayout {
                                Layout.row:         1 + index * _rowsPerPort
                                Layout.column:      0
                                Layout.alignment:   Qt.AlignTop
                                Layout.topMargin:   _cellPadding
                                spacing:            ScreenTools.defaultFontPixelHeight * 0.25
                                visible:            _portShown(object)

                                required property int index
                                required property var object

                                readonly property bool _inUse:      object.assignments.length > 0
                                readonly property bool _attention:  object.rebootRequired || object.conflictText !== ""

                                RowLayout {
                                    spacing: ScreenTools.defaultFontPixelWidth

                                    Rectangle {
                                        implicitWidth:  ScreenTools.defaultFontPixelHeight * 0.5
                                        implicitHeight: implicitWidth
                                        radius:         implicitWidth / 2
                                        color:          _attention ? qgcPal.warningText : (_inUse ? qgcPal.colorGreen : "transparent")
                                        border.color:   (_attention || _inUse) ? color : qgcPal.textSecondary
                                    }

                                    QGCLabel {
                                        text:                   object.label
                                        font.bold:              true
                                        font.capitalization:    Font.AllUppercase
                                        font.letterSpacing:     ScreenTools.defaultFontPixelWidth * 0.1
                                    }
                                }

                                Repeater {
                                    model: object.assignments

                                    QGCLabel {
                                        text:           modelData
                                        font.family:    ScreenTools.fixedFontFamily
                                        font.pointSize: ScreenTools.smallFontPointSize
                                        color:          qgcPal.textSecondary

                                        required property string modelData
                                    }
                                }

                                QGCLabel {
                                    text:           qsTr("No function assigned")
                                    font.pointSize: ScreenTools.smallFontPointSize
                                    color:          qgcPal.textSecondary
                                    visible:        !_inUse
                                }

                                RowLayout {
                                    spacing:    ScreenTools.defaultFontPixelWidth / 2
                                    visible:    object.shared || object.rebootRequired

                                    QGCChip {
                                        text:       qsTr("Shared")
                                        visible:    object.shared
                                    }

                                    QGCChip {
                                        text:       qsTr("Reboot required")
                                        textColor:  qgcPal.warningText
                                        visible:    object.rebootRequired
                                    }
                                }
                            }
                        }

                        Repeater {
                            model: controller.ports

                            QGCCaptionCell {
                                id:                 functionCell
                                Layout.row:         1 + index * _rowsPerPort
                                Layout.column:      1
                                Layout.alignment:   Qt.AlignTop
                                Layout.fillWidth:   true
                                Layout.horizontalStretchFactor: 0
                                caption:            object.shared ? qsTr("Functions") : qsTr("Function")
                                visible:            _portShown(object)

                                required property int index
                                required property var object

                                QGCComboBox {
                                    id:                 functionCombo
                                    Layout.fillWidth:   true
                                    model:              functionCell.object.functionLabels
                                    currentIndex:       functionCell.object.functionIndex
                                    sizeToContents:     true
                                    visible:            !functionCell.object.shared

                                    onActivated: (comboIndex) => {
                                        controller.setPortFunction(functionCell.object.portId, comboIndex)
                                        currentIndex = Qt.binding(function() { return functionCell.object.functionIndex })
                                    }

                                    // A model change resets the index, so restore the binding once the row has updated
                                    onModelChanged: Qt.callLater(function() {
                                        currentIndex = Qt.binding(function() { return functionCell.object.functionIndex })
                                    })
                                }

                                // Shown instead of the combo on ports several functions can use at once
                                Repeater {
                                    // Skip "None", which is index 0
                                    model: functionCell.object.shared ? functionCell.object.functionLabels.length - 1 : 0

                                    QGCCheckBoxSlider {
                                        Layout.fillWidth:   true
                                        text:               functionCell.object.functionLabels[functionIndex]
                                        checked:            functionCell.object.functionEnabled[functionIndex] === true

                                        required property int index
                                        readonly property int functionIndex: index + 1

                                        onClicked: {
                                            controller.setPortFunctionEnabled(functionCell.object.portId, functionIndex, checked)
                                            checked = Qt.binding(function() { return functionCell.object.functionEnabled[functionIndex] === true })
                                        }
                                    }
                                }

                                QGCLabel {
                                    Layout.fillWidth:       true
                                    Layout.preferredWidth:  functionCombo.implicitWidth
                                    wrapMode:               Text.WordWrap
                                    font.pointSize:         ScreenTools.smallFontPointSize
                                    color:                  qgcPal.textSecondary
                                    text:                   qsTr("%1 always runs on this port unless it is assigned to another port.")
                                                                .arg(functionCell.object.functionLabels[functionCell.object.functionIndex])
                                    visible:                functionCell.object.defaultFunctionOnPort && functionCell.object.conflictText === ""
                                }
                            }
                        }

                        Repeater {
                            model: controller.ports

                            QGCCaptionCell {
                                id:                 baudCell
                                Layout.row:         1 + index * _rowsPerPort
                                Layout.column:      2
                                Layout.alignment:   Qt.AlignTop
                                Layout.fillWidth:   true
                                Layout.horizontalStretchFactor: 0
                                caption:            qsTr("Baud")
                                visible:            _portShown(object)

                                required property int index
                                required property var object

                                FactComboBox {
                                    Layout.fillWidth:   true
                                    fact:               baudCell.object.baudFact
                                    indexModel:         false
                                    sizeToContents:     true
                                    visible:            baudCell.object.baudFact
                                }

                                QGCLabel {
                                    text:       baudCell.object.baudLoading ? qsTr("Loading...") : qsTr("N/A")
                                    color:      qgcPal.textSecondary
                                    visible:    !baudCell.object.baudFact
                                }
                            }
                        }

                        // Only the Options column takes spare width. The other columns size to their widest cell.
                        // Options wrap instead of widening the page, so the column only needs its widest single cell.
                        Repeater {
                            model: controller.ports

                            Flow {
                                id:                 optionsFlow
                                Layout.row:         1 + index * _rowsPerPort
                                Layout.column:      3
                                Layout.alignment:   Qt.AlignTop
                                Layout.fillWidth:   true
                                Layout.horizontalStretchFactor: 1
                                Layout.minimumWidth:    _widestChildWidth
                                Layout.preferredWidth:  _widestChildWidth
                                spacing:            _margins
                                visible:            _portShown(object)

                                required property int index
                                required property var object

                                // The message wraps, so only option cells set the column's minimum width
                                readonly property real _widestChildWidth: {
                                    let widest = optionsMessage.visible ? ScreenTools.defaultFontPixelWidth * 20 : 0
                                    for (let i = 0; i < children.length; i++) {
                                        if (children[i].visible && children[i] !== optionsMessage) {
                                            widest = Math.max(widest, children[i].implicitWidth)
                                        }
                                    }
                                    return widest
                                }

                                Repeater {
                                    model: optionsFlow.object.optionFacts

                                    QGCCaptionCell {
                                        caption: _optionLabel(modelData.name, optionsFlow.object.shared)

                                        required property var modelData

                                        FactComboBox {
                                            Layout.fillWidth:   true
                                            fact:               modelData
                                            indexModel:         false
                                            sizeToContents:     true
                                            visible:            modelData.enumStrings.length > 0
                                        }

                                        FactTextField {
                                            Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 12
                                            fact:                   modelData
                                            visible:                modelData.enumStrings.length === 0
                                        }

                                        QGCLabel {
                                            Layout.fillWidth:       true
                                            Layout.preferredWidth:  Math.min(implicitWidth, ScreenTools.defaultFontPixelWidth * 20)
                                            wrapMode:               Text.WordWrap
                                            font.pointSize:         ScreenTools.smallFontPointSize
                                            color:                  qgcPal.textSecondary
                                            text:                   _optionNote(modelData.name, optionsFlow.object.shared)
                                            visible:                text !== ""
                                        }
                                    }
                                }

                                QGCLabel {
                                    id:         optionsMessage
                                    width:      Math.min(implicitWidth, optionsFlow.width)
                                    topPadding: _cellPadding
                                    wrapMode:   Text.WordWrap
                                    color:      qgcPal.textSecondary
                                    text:       optionsFlow.object.optionsLoading ? qsTr("Loading...") :
                                                    (optionsFlow.object.conflictText !== "" ?
                                                         qsTr("Options are hidden while more than one function is assigned.") :
                                                         qsTr("No options for this port."))
                                    visible:    optionsFlow.object.optionsLoading || optionsFlow.object.optionFacts.length === 0
                                }
                            }
                        }

                        Repeater {
                            model: controller.ports

                            Rectangle {
                                id:                 conflictBox
                                Layout.row:         2 + index * _rowsPerPort
                                Layout.column:      0
                                Layout.columnSpan:  4
                                Layout.fillWidth:   true
                                implicitHeight:     conflictLabel.implicitHeight + _margins * 2
                                color:              "transparent"
                                border.color:       qgcPal.warningText
                                radius:             ScreenTools.defaultBorderRadius * 2
                                visible:            object.conflictText !== "" && _portShown(object)

                                required property int index
                                required property var object

                                QGCLabel {
                                    id:                 conflictLabel
                                    anchors.left:       parent.left
                                    anchors.right:      parent.right
                                    anchors.margins:    _cellPadding
                                    anchors.verticalCenter: parent.verticalCenter
                                    wrapMode:           Text.WordWrap
                                    color:              qgcPal.warningText
                                    text:               conflictBox.object.conflictText
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Opening on the ports in use keeps the page short on boards with many ports
    Component.onCompleted: {
        for (let i = 0; i < controller.ports.count; i++) {
            if (controller.ports.get(i).assignments.length > 0) {
                _showInUseOnly = true
                return
            }
        }
    }
}
