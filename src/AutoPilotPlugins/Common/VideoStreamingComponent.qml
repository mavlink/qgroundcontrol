/****************************************************************************
 *
 *   (c) 2009-2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
    pageComponent:  _cameraAvailable ? videoPageComponent : noCameraPageComponent

    property real _margins:         ScreenTools.defaultFontPixelHeight
    property int  _firstColumn:     ScreenTools.defaultFontPixelWidth * 10
    property int  _secondColumn:    ScreenTools.defaultFontPixelWidth * 20
    property bool _cameraAvailable: controller.cameraNames.length > 0

    VideoStreamingComponentController {
        id:         controller
        factPanel:  videoPage.viewPanel
    }

    Component {
        id: noCameraPageComponent

        QGCLabel {
            text:         qsTr("No available camera")
            font.family:  ScreenTools.demiboldFontFamily
        }
    }

    Component {
        id: videoPageComponent

        Column {
            id:         mainCol
            width:      availableWidth
            spacing:    _margins

            QGCLabel {
                        text:         qsTr("Cameras")
                        font.family:  ScreenTools.demiboldFontFamily
            }

            QGCComboBox {
                id:    cameraNumber
                width: _secondColumn
                model: controller.cameraNames
            }

            QGCLabel {
                text:         qsTr("Video streaming settings")
                font.family:  ScreenTools.demiboldFontFamily
            }

            Rectangle {
                width:              settingsCol.width  + _margins * 2
                height:             settingsCol.height + _margins * 2
                anchors.topMargin:  _margins / 2
                anchors.left:       parent.left
                color:              qgcPal.windowShade

                Column {
                    id:               settingsCol
                    spacing:          _margins
                    anchors.top:      parent.top
                    anchors.left:     parent.left
                    anchors.margins:  _margins

                    Row {
                        spacing: ScreenTools.defaultFontPixelWidth

                        QGCLabel {
                            id:                uriLabel
                            width:             _firstColumn
                            text:              qsTr("URI:")
                        }

                        FactTextField {
                            fact:              controller.cameras[cameraNumber.currentIndex].uriFact
                            width:             _secondColumn
                            anchors.baseline:  uriLabel.baseline
                        }
                    }

                    Row {
                        spacing: ScreenTools.defaultFontPixelWidth

                        QGCLabel {
                            id:                resolutionLabel
                            width:             _firstColumn
                            text:              qsTr("Resolution:")
                        }

                        FactComboBox {
                            fact:              controller.cameras[cameraNumber.currentIndex].resolutionFact
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
                            fact:              controller.cameras[cameraNumber.currentIndex].bitRateFact
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
                            fact:              controller.cameras[cameraNumber.currentIndex].fpsFact
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
                            fact:              controller.cameras[cameraNumber.currentIndex].rotationFact
                            width:             _secondColumn
                            anchors.baseline:  rotationLabel.baseline
                            indexModel:        false
                        }
                    }

                    QGCButton {
                        text:       "Save  settings"
                        onClicked:  {
                            saveDialog.visible = true
                        }
                        MessageDialog {
                            id:         saveDialog
                            visible:    false
                            icon:       StandardIcon.Warning
                            title:      qsTr("Save Settings")
                            text:       qsTr("Do you want to change settings? Video Stream will restart.")
                            standardButtons: StandardButton.Yes | StandardButton.No
                            onYes: {
                                controller.saveSettings(cameraNumber.currentIndex)
                                saveDialog.visible = false
                            }
                            onNo: {
                                saveDialog.visible = false
                            }
                        }
                    }
                }
            }

            Row {
                spacing: ScreenTools.defaultFontPixelWidth

                QGCButton {
                    text:       "Start Streaming"
                    onClicked:  {
                        controller.startVideo(cameraNumber.currentIndex)
                    }
                }

                QGCButton {
                    text:       "Stop Streaming"
                    onClicked:  {
                        controller.stopVideo(cameraNumber.currentIndex)
                    }
                }
            } // Row
        } // Column
    } // Component
} // SetupPage
