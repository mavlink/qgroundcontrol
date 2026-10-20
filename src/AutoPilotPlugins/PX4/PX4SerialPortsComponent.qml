import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SetupPage {
    id:             serialPortsPage
    pageComponent:  pageComponent

    property real _margins: ScreenTools.defaultFontPixelHeight * 0.5

    readonly property int _rowsPerPort: 3   // separator, port settings, conflict warning
    // Reverting or rebooting before earlier writes finish could undo the revert or apply only part of a change
    readonly property bool _pendingWrites: controller.vehicle ? controller.vehicle.parameterManager.pendingWrites : false

    // shared: several functions' options are shown together, so name the function instance
    function _optionLabel(paramName, shared) {
        const labels = {
            "MODE":         qsTr("Mode"),
            "RATE":         qsTr("Rate"),
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

    PX4SerialPortsController {
        id: controller
    }

    QGCPalette {
        id: qgcPal
    }

    Component {
        id: pageComponent

        ColumnLayout {
            width:   Math.max(availableWidth, portGrid.implicitWidth)
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
                        enabled:    controller.vehicle && !controller.vehicle.armed && !_pendingWrites
                        onClicked:  controller.vehicle.rebootVehicle()
                    }
                }
            }

            GridLayout {
                id:             portGrid
                columns:        4
                columnSpacing:  _margins
                rowSpacing:     _margins / 2

                QGCLabel {
                    Layout.row:     0
                    Layout.column:  0
                    text:           qsTr("Port")
                    font.bold:      true
                }

                QGCLabel {
                    Layout.row:     0
                    Layout.column:  1
                    text:           qsTr("Function")
                    font.bold:      true
                }

                QGCLabel {
                    Layout.row:     0
                    Layout.column:  2
                    text:           qsTr("Baud")
                    font.bold:      true
                }

                // Only the Options column takes spare width. The combo columns size to their widest combo.
                QGCLabel {
                    Layout.row:         0
                    Layout.column:      3
                    Layout.fillWidth:   true
                    Layout.horizontalStretchFactor: 1
                    text:               qsTr("Options")
                    font.bold:          true
                }

                Repeater {
                    model: controller.ports

                    Rectangle {
                        Layout.row:         1 + index * _rowsPerPort
                        Layout.column:      0
                        Layout.columnSpan:  4
                        Layout.fillWidth:   true
                        implicitHeight:     1
                        color:              qgcPal.groupBorder

                        required property int index
                    }
                }

                Repeater {
                    model: controller.ports

                    ColumnLayout {
                        Layout.row:         2 + index * _rowsPerPort
                        Layout.column:      0
                        Layout.alignment:   Qt.AlignTop
                        spacing:            0

                        required property int index
                        required property var object

                        QGCLabel {
                            text:       object.label
                            font.bold:  true
                        }

                        QGCLabel {
                            text:       qsTr("Reboot required")
                            color:      qgcPal.warningText
                            visible:    object.rebootRequired
                        }
                    }
                }

                Repeater {
                    model: controller.ports

                    QGCComboBox {
                        Layout.row:         2 + index * _rowsPerPort
                        Layout.column:      1
                        Layout.alignment:   Qt.AlignBottom
                        Layout.fillWidth:   true
                        Layout.horizontalStretchFactor: 0
                        model:              object.functionLabels
                        currentIndex:       object.functionIndex
                        sizeToContents:     true
                        visible:            !object.shared

                        required property int index
                        required property var object

                        onActivated: (comboIndex) => {
                            controller.setPortFunction(object.portId, comboIndex)
                            currentIndex = Qt.binding(function() { return object.functionIndex })
                        }

                        // A model change resets the index, so restore the binding once the row has updated
                        onModelChanged: Qt.callLater(function() {
                            currentIndex = Qt.binding(function() { return object.functionIndex })
                        })
                    }
                }

                // Shares the function cell, shown instead of the combo on ports several functions can use at once
                Repeater {
                    model: controller.ports

                    ColumnLayout {
                        Layout.row:         2 + index * _rowsPerPort
                        Layout.column:      1
                        Layout.alignment:   Qt.AlignTop
                        spacing:            0
                        visible:            object.shared

                        required property int index
                        required property var object

                        Repeater {
                            // Skip "None", which is index 0
                            model: object.functionLabels.length - 1

                            QGCCheckBox {
                                text:       object.functionLabels[functionIndex]
                                checked:    object.functionEnabled[functionIndex] === true

                                required property int index
                                readonly property int functionIndex: index + 1

                                onClicked: {
                                    controller.setPortFunctionEnabled(object.portId, functionIndex, checked)
                                    checked = Qt.binding(function() { return object.functionEnabled[functionIndex] === true })
                                }
                            }
                        }
                    }
                }

                Repeater {
                    model: controller.ports

                    FactComboBox {
                        Layout.row:         2 + index * _rowsPerPort
                        Layout.column:      2
                        Layout.alignment:   Qt.AlignBottom
                        Layout.fillWidth:   true
                        Layout.horizontalStretchFactor: 0
                        fact:               object.baudFact
                        indexModel:         false
                        sizeToContents:     true
                        visible:            object.baudFact

                        required property int index
                        required property var object
                    }
                }

                // Shares the baud cell, shown instead of the combo while loading or when the port has no baud rate
                Repeater {
                    model: controller.ports

                    QGCLabel {
                        Layout.row:         2 + index * _rowsPerPort
                        Layout.column:      2
                        Layout.alignment:   object.shared ? Qt.AlignTop : Qt.AlignBottom
                        text:               object.baudLoading ? qsTr("Loading...") : "-"
                        visible:            !object.baudFact

                        required property int index
                        required property var object
                    }
                }

                Repeater {
                    model: controller.ports

                    RowLayout {
                        Layout.row:         2 + index * _rowsPerPort
                        Layout.column:      3
                        Layout.alignment:   Qt.AlignTop
                        spacing:            _margins

                        required property int index
                        required property var object

                        Repeater {
                            model: object.optionFacts

                            RowLayout {
                                required property var modelData

                                LabelledFactComboBox {
                                    label:      _optionLabel(modelData.name, object.shared)
                                    fact:       modelData
                                    indexModel: false
                                    vertical:   true
                                    visible:    modelData.enumStrings.length > 0
                                }

                                LabelledFactTextField {
                                    label:                      _optionLabel(modelData.name, object.shared)
                                    fact:                       modelData
                                    textFieldPreferredWidth:    ScreenTools.defaultFontPixelWidth * 12
                                    vertical:                   true
                                    visible:                    modelData.enumStrings.length === 0
                                }
                            }
                        }

                        QGCLabel {
                            Layout.alignment:   Qt.AlignBottom
                            text:               qsTr("Loading...")
                            visible:            object.optionsLoading
                        }
                    }
                }

                Repeater {
                    model: controller.ports

                    QGCLabel {
                        Layout.row:         3 + index * _rowsPerPort
                        Layout.column:      0
                        Layout.columnSpan:  4
                        Layout.fillWidth:   true
                        wrapMode:           Text.WordWrap
                        color:              qgcPal.warningText
                        text:               object.conflictText
                        visible:            object.conflictText !== ""

                        required property int index
                        required property var object
                    }
                }
            }
        }
    }
}
