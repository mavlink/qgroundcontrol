import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

AnalyzePage {
    id:                 exclusionZoneReviewPage
    pageComponent:      pageComponent
    pageDescription:    qsTr("Import perception-derived hazard polygons, review and approve them, " +
                              "then push the approved zones as exclusion fence polygons to a vehicle. " +
                              "Nothing is pushed without an explicit confirmation.")

    property var    _controller:      ExclusionZoneController
    property var    _targetVehicle:   _controller.targetVehicle

    KMLOrSHPFileDialog {
        id: importDialog
        onAcceptedForLoad: (file) => {
            if (!_controller.importFromFile(file)) {
                importFailedLabel.visible = true
            }
            close()
        }
    }

    QGCPopupDialogFactory {
        id: pushConfirmDialogFactory
        dialogComponent: pushConfirmDialogComponent
    }

    Component {
        id: pushConfirmDialogComponent

        QGCPopupDialog {
            title:                  qsTr("Push Exclusion Zones")
            buttons:                Dialog.Cancel | Dialog.Ok
            acceptButtonEnabled:    true

            onAccepted: _controller.pushApproved()

            ColumnLayout {
                spacing: ScreenTools.defaultDialogControlSpacing

                QGCLabel {
                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 40
                    wrapMode:               Text.WordWrap
                    text: qsTr("Push %1 approved exclusion zone(s) to Vehicle %2's geofence? This cannot be undone automatically.")
                              .arg(_controller.approvedCount)
                              .arg(_targetVehicle ? _targetVehicle.id : "?")
                }
            }
        }
    }

    Connections {
        target: _controller
        function onPushFinished(success, message) {
            pushResultLabel.text = success ? qsTr("Push succeeded.") : qsTr("Push failed: %1").arg(message)
            pushResultLabel.visible = true
        }
    }

    Component {
        id: pageComponent

        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight

            RowLayout {
                spacing: ScreenTools.defaultFontPixelWidth

                QGCButton {
                    text:       qsTr("Import Zones from File…")
                    onClicked:  importDialog.openForLoad()
                }

                QGCLabel {
                    id:         importFailedLabel
                    visible:    false
                    color:      qgcPal.colorRed
                    text:       qsTr("Import failed — see log for details.")
                }
            }

            QGCLabel {
                text:       qsTr("Target vehicle:")
                visible:    QGroundControl.multiVehicleManager.vehicles.count > 0
            }

            RowLayout {
                spacing: ScreenTools.defaultFontPixelWidth
                visible: QGroundControl.multiVehicleManager.vehicles.count > 0

                Repeater {
                    model: QGroundControl.multiVehicleManager.vehicles

                    QGCRadioButton {
                        text:       qsTr("Vehicle %1").arg(object.id)
                        checked:    _targetVehicle === object
                        onClicked:  _controller.targetVehicle = object
                    }
                }
            }

            QGCLabel {
                visible:    QGroundControl.multiVehicleManager.vehicles.count === 0
                text:       qsTr("No vehicles connected.")
            }

            QGCLabel {
                text:       qsTr("Staged zones (%1 approved of %2):").arg(_controller.approvedCount).arg(_controller.stagedZones.count)
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0

                Repeater {
                    model: _controller.stagedZones

                    RowLayout {
                        Layout.fillWidth: true

                        QGCCheckBox {
                            checked:    object.approved
                            onClicked:  _controller.setApproved(index, checked)
                        }

                        QGCLabel {
                            Layout.fillWidth:   true
                            text:               qsTr("Exclusion zone %1 — %2 vertices").arg(index + 1).arg(object.polygon.path.length)
                        }
                    }
                }
            }

            QGCButton {
                text:       qsTr("Push Approved Zones…")
                enabled:    _controller.approvedCount > 0 && _targetVehicle !== null
                onClicked:  pushConfirmDialogFactory.open()
            }

            QGCLabel {
                id:         pushResultLabel
                visible:    false
                wrapMode:   Text.WordWrap
                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 40
            }
        }
    }
}
