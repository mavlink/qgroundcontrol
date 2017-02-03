/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick              2.5
import QtQuick.Dialogs      1.2

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0

SetupPage {
    id:             videoPage
    pageComponent:  videoPageComponent

    property real _margins:       ScreenTools.defaultFontPixelHeight
    property int  _firstColumn:   ScreenTools.defaultFontPixelWidth * 10
    property int  _secondColumn:  ScreenTools.defaultFontPixelWidth * 15

    APMVideoStreamingComponentController {
        id:         controller
        factPanel:  videoPage.viewPanel
    }

    Component {
        id: videoPageComponent

        Column {
            id:         mainCol
            width:      availableWidth
            spacing:    _margins

            QGCLabel {
                text:         qsTr("Сonnection settings")
                font.family:  ScreenTools.demiboldFontFamily
            }

            Rectangle {
                width:              addressCol.width  + _margins * 2
                height:             addressCol.height + _margins * 2
                anchors.topMargin:  _margins / 2
                anchors.left:       parent.left
                color:              qgcPal.windowShade

                Column {
                    id:               addressCol
                    spacing:          _margins
                    anchors.top:      parent.top
                    anchors.left:     parent.left
                    anchors.margins:  _margins

                    Row {
                        spacing: ScreenTools.defaultFontPixelWidth

                        QGCLabel {
                            width:             _firstColumn
                            text:              qsTr("IP adress:")
                            anchors.baseline:  ipField.baseline

                        }

                        QGCTextField {
                            id:                ipField
                            text:              controller.targetIp
                            width:             _secondColumn
                        }
                    }

                    Row {
                        spacing: ScreenTools.defaultFontPixelWidth

                        QGCLabel {
                            width:             _firstColumn
                            text:              qsTr("Port:")
                            anchors.baseline:  portField.baseline
                        }

                        QGCTextField {
                            id:                portField
                            text:              controller.targetPort
                            width:             _secondColumn
                            validator:         IntValidator { bottom: 0; top: 65535}
                        }
                    }

                    QGCButton {
                        text:      "Use port of GCS"
                        onClicked: {
                            portField.text = QGroundControl.videoManager.udpPort.toString()
                        }
                    }
                }
            }

            QGCButton {
                text:       "Save"
                onClicked:  {
                    saveDialog.visible = true
                }
                MessageDialog {
                    id:         saveDialog
                    visible:    false
                    icon:       StandardIcon.Warning
                    title:      qsTr("Save Settings")
                    text:       qsTr("Do you want to change connection settings?")
                    standardButtons: StandardButton.Yes | StandardButton.No
                    onYes: {
                        controller.saveAddress(ipField.text,portField.text)
                        saveDialog.visible = false
                    }
                    onNo: {
                        saveDialog.visible = false
                    }
                }
            }

            QGCLabel {
                text:           qsTr("Quality settings")
                font.family:    ScreenTools.demiboldFontFamily
            }

            Rectangle {
                color:   qgcPal.windowShade
                width:   qualityCol.width  + _margins * 2
                height:  qualityCol.height + _margins * 2

                Column {
                    id:               qualityCol
                    spacing:          _margins
                    anchors.top:      parent.top
                    anchors.left:     parent.left
                    anchors.margins:  _margins

                    Row {
                        spacing: ScreenTools.defaultFontPixelWidth

                        QGCLabel {
                            id:                resolutionLabel
                            width:             _firstColumn
                            text:              qsTr("Resolution:")
                        }

                        FactComboBox {
                            fact:              controller.resolutionFact
                            width:             _secondColumn
                            anchors.baseline:  resolutionLabel.baseline
                            indexModel:        true
                        }
                    }

                    Row {
                        spacing: ScreenTools.defaultFontPixelWidth

                        QGCLabel {
                            id:                bitRateLabel
                            width:             _firstColumn
                            text:              qsTr("Bit rate:")
                        }

                        FactComboBox {
                            fact:              controller.bitRateFact
                            width:             _secondColumn
                            anchors.baseline:  bitRateLabel.baseline
                            indexModel:        false
                        }
                    }

                    Row {
                        spacing: ScreenTools.defaultFontPixelWidth

                        QGCLabel {
                            id:                fpsLabel
                            width:             _firstColumn
                            text:              qsTr("FPS:")
                        }

                        FactComboBox {
                            fact:              controller.fpsFact
                            width:             _secondColumn
                            anchors.baseline:  fpsLabel.baseline
                            indexModel:        false
                        }
                    }

                    Row {
                        spacing: ScreenTools.defaultFontPixelWidth

                        QGCLabel {
                            id:                rotationLabel
                            width:             _firstColumn
                            text:              qsTr("Rotation:")
                        }

                        FactComboBox {
                            fact:              controller.rotationFact
                            width:             _secondColumn
                            anchors.baseline:  rotationLabel.baseline
                            indexModel:        false
                        }
                    }
                }
            }

            Row {
                spacing: ScreenTools.defaultFontPixelWidth

                QGCButton {
                    text:       "Start Streaming"
                    onClicked:  {
                        controller.startVideo()
                    }
                }

                QGCButton {
                    text:       "Stop Streaming"
                    onClicked:  {
                        controller.stopVideo()
                    }
                }
            } // Row
        } // Column
    } // Component
} // SetupPage
