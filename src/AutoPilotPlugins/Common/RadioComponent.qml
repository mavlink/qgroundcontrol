import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls
import QGroundControl.VehicleSetup

SetupPage {
    id: radioPage
    pageComponent: pageComponent

    Component {
        id: pageComponent

        RemoteControlCalibration {
            additionalSetupComponent: ColumnLayout {
                spacing: ScreenTools.defaultFontPixelHeight / 2

                ColumnLayout {
                    id: switchSettings
                    Layout.fillWidth: true

                    Repeater {
                        model: QGroundControl.multiVehicleManager.activeVehicle.px4Firmware ?
                                    (QGroundControl.multiVehicleManager.activeVehicle.multiRotor ?
                                        [ "RC_MAP_AUX1", "RC_MAP_AUX2", "RC_MAP_PARAM1", "RC_MAP_PARAM2", "RC_MAP_PARAM3"] :
                                        [ "RC_MAP_FLAPS", "RC_MAP_AUX1", "RC_MAP_AUX2", "RC_MAP_PARAM1", "RC_MAP_PARAM2", "RC_MAP_PARAM3"]) :
                                    0

                        LabelledFactComboBox {
                            label: fact.shortDescription
                            fact: _controller.getParameterFact(-1, modelData)
                            indexModel: false
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 1
                    color: qgcPal.text
                }

                RowLayout {
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCButton {
                        id: bindButton
                        text: qsTr("Spektrum Bind")
                        onClicked: spektrumBindDialogComponent.createObject(mainWindow).open()
                    }

                    QGCButton {
                        text: qsTr("CRSF Bind")
                        onClicked: mainWindow.showMessageDialog(qsTr("CRSF Bind"),
                                                                qsTr("Click Ok to place your CRSF receiver in the bind mode."),
                                                                Dialog.Ok | Dialog.Cancel,
                                                                function() { _controller.crsfBindMode() })
                    }

                    QGCButton {
                        text: qsTr("Copy Trims")
                        onClicked: mainWindow.showMessageDialog(qsTr("Copy Trims"),
                                                                qsTr("Center your sticks and move throttle all the way down, then press Ok to copy trims. After pressing Ok, reset the trims on your radio back to zero."),
                                                                Dialog.Ok | Dialog.Cancel,
                                                                function() { _controller.copyTrims() })
                    }
                }

                Component {
                    id: spektrumBindDialogComponent

                    QGCPopupDialog {
                        title: qsTr("Spektrum Bind")
                        buttons: Dialog.Ok | Dialog.Cancel

                        onAccepted: { _controller.spektrumBindMode(radioGroup.checkedButton.bindMode) }

                        ButtonGroup { id: radioGroup }

                        ColumnLayout {
                            spacing: ScreenTools.defaultFontPixelHeight / 2

                            QGCLabel {
                                wrapMode: Text.WordWrap
                                text: qsTr("Click Ok to place your Spektrum receiver in the bind mode.")
                            }

                            QGCLabel {
                                wrapMode: Text.WordWrap
                                text: qsTr("Select the specific receiver type below:")
                            }

                            QGCRadioButton {
                                text: qsTr("DSM2 Mode")
                                ButtonGroup.group: radioGroup
                                property int bindMode: RadioComponentController.DSM2
                            }

                            QGCRadioButton {
                                text: qsTr("DSMX (7 channels or less)")
                                ButtonGroup.group: radioGroup
                                property int bindMode: RadioComponentController.DSMX7
                            }

                            QGCRadioButton {
                                checked: true
                                text: qsTr("DSMX (8 channels or more)")
                                ButtonGroup.group: radioGroup
                                property int bindMode: RadioComponentController.DSMX8
                            }
                        }
                    }
                }
            }
        }
    }
}
