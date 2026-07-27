import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

QGCPopupDialog {
    title:                  qsTr("Load Parameters")
    buttons:                Dialog.Cancel | (_sendableCount > 0 ? Dialog.Ok : 0)
    acceptButtonEnabled:    paramController.diffSelectedCount > 0

    required property var paramController

    property int _sendableCount: paramController.diffSendableCount

    onAccepted: paramController.sendDiff()

    Component.onDestruction: paramController.clearDiff();

    ColumnLayout {
        spacing: ScreenTools.defaultDialogControlSpacing

        QGCLabel {
            id:                     summaryLabel
            objectName:             "diffSummaryLabel"
            Layout.preferredWidth:  mainGrid.visible ? mainGrid.width : ScreenTools.defaultFontPixelWidth * 40
            wrapMode:               Text.WordWrap
            text: {
                var clauses = []
                if (_sendableCount > 0) {
                    if (paramController.diffNoVehicleCount > 0) {
                        clauses.push(qsTr("%1 will be changed (including %2 not currently on the Vehicle)").arg(_sendableCount).arg(paramController.diffNoVehicleCount))
                    } else {
                        clauses.push(qsTr("%1 will be changed").arg(_sendableCount))
                    }
                }
                if (paramController.diffUnchangedCount === 1) {
                    clauses.push(qsTr("1 already matches the Vehicle"))
                } else if (paramController.diffUnchangedCount > 1) {
                    clauses.push(qsTr("%1 already match the Vehicle").arg(paramController.diffUnchangedCount))
                }
                if (paramController.diffReadOnlyCount === 1) {
                    clauses.push(qsTr("1 read-only parameter will not be sent"))
                } else if (paramController.diffReadOnlyCount > 1) {
                    clauses.push(qsTr("%1 read-only parameters will not be sent").arg(paramController.diffReadOnlyCount))
                }
                if (paramController.diffMissingParams.length > 0) {
                    clauses.push(qsTr("%1 not found on the Vehicle and cannot be sent").arg(paramController.diffMissingParams.length))
                }
                if (paramController.diffParsedCount === 1) {
                    return clauses.length === 0 ? qsTr("Loaded 1 parameter from file.")
                                                : qsTr("Loaded 1 parameter from file: %1.").arg(clauses.join(", "))
                }
                return clauses.length === 0 ? qsTr("Loaded %1 parameters from file.").arg(paramController.diffParsedCount)
                                            : qsTr("Loaded %1 parameters from file: %2.").arg(paramController.diffParsedCount).arg(clauses.join(", "))
            }
        }

        QGCLabel {
            objectName:             "diffOkHintLabel"
            Layout.preferredWidth:  summaryLabel.Layout.preferredWidth
            wrapMode:               Text.WordWrap
            visible:                _sendableCount > 0
            text:                   qsTr("Click 'Ok' to update the parameters below on the Vehicle.")
        }

        GridLayout {
            id:             mainGrid
            objectName:     "diffGrid"
            rows:           paramController.diffList.count + 1
            columns:        paramController.diffMultipleComponents ? 5 : 4
            flow:           GridLayout.TopToBottom
            rowSpacing:     0
            columnSpacing:  ScreenTools.defaultFontPixelWidth
            visible:        paramController.diffList.count

            QGCCheckBox {
                objectName: "diffCheckAllCheckbox"
                checked: true
                onClicked: {
                    for (var i=0; i<paramController.diffList.count; i++) {
                        var paramDiff = paramController.diffList.get(i)
                        if (!paramDiff.cannotSend) {
                            paramDiff.load = checked
                        }
                    }
                }
            }
            Repeater {
                model: paramController.diffList
                QGCCheckBox {
                    objectName: "diffRowCheckbox_" + (paramController.diffMultipleComponents ? object.componentId + "_" : "") + object.name
                    enabled:    !object.cannotSend
                    checked:    object.load
                    onClicked:  object.load = checked
                }
            }

            Repeater {
                model: paramController.diffMultipleComponents ? 1 : 0
                QGCLabel { text: qsTr("Comp ID") }
            }
            Repeater {
                model: paramController.diffMultipleComponents ? paramController.diffList : 0
                QGCLabel { text: object.componentId }
            }

            QGCLabel { text: qsTr("Name") }
            Repeater {
                model: paramController.diffList
                QGCLabel { text: object.name }
            }

            QGCLabel { text: qsTr("File") }
            Repeater {
                model: paramController.diffList
                QGCLabel { text: object.fileValue + " " + object.units }
            }

            QGCLabel { text: qsTr("Vehicle") }
            Repeater {
                model: paramController.diffList
                QGCLabel {
                    text: object.cannotSend ? qsTr("N/A — not on Vehicle") :
                          object.noVehicleValue ? qsTr("N/A — new to Vehicle") :
                          object.vehicleValue + " " + object.units
                }
            }
        }

        QGCLabel {
            objectName:             "diffNoVehicleHintLabel"
            Layout.preferredWidth:  summaryLabel.Layout.preferredWidth
            wrapMode:               Text.WordWrap
            visible:                paramController.diffNoVehicleCount > 0
            text:                   qsTr("Parameters marked 'new to Vehicle' have not been reported by the Vehicle. They may only become visible after they are sent and the Vehicle is rebooted.")
        }

        QGCLabel {
            objectName:             "diffCannotSendHintLabel"
            Layout.preferredWidth:  summaryLabel.Layout.preferredWidth
            wrapMode:               Text.WordWrap
            visible:                paramController.diffMissingParams.length > 0
            text:                   qsTr("Parameters marked 'not on Vehicle' cannot be sent since the file does not include type information. They may only exist after another parameter is changed and the Vehicle is rebooted, after which you can load this file again.")
        }
    }
}
