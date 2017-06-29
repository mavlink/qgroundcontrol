/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.7
import QtQuick.Layouts  1.3
import QtQuick.Controls 1.4

import QGroundControl               1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

import TyphoonHQuickInterface       1.0

SetupPage {
    id:             gimbalPage
    pageComponent:  pageComponent
    Component {
        id: pageComponent

        Item {
            width:  availableWidth
            height: availableHeight

            FactPanelController { id: controller; factPanel: gimbalPage.viewPanel }

            property var _activeVehicle:  QGroundControl.multiVehicleManager.activeVehicle

            ColumnLayout {
                id:                 settingsColumn
                spacing:            ScreenTools.defaultFontPixelHeight
                anchors.centerIn:   parent
                QGCGroupBox {
                    title:              qsTr("Gimbal Calibration")
                    Column {
                        spacing:            ScreenTools.defaultFontPixelHeight
                        padding:            ScreenTools.defaultFontPixelWidth * 4
                        Row {
                            id:         importRow
                            spacing:    ScreenTools.defaultFontPixelWidth * 4
                            Image {
                                width:              ScreenTools.defaultFontPixelHeight * 25
                                source:             "/typhoonh/img/VehicleWithCamera.svg"
                                fillMode:           Image.PreserveAspectFit
                                sourceSize.width:   width
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            QGCLabel {
                                width:                  ScreenTools.defaultFontPixelHeight * 25
                                wrapMode:               Text.WordWrap
                                horizontalAlignment:    Text.AlignJustify
                                anchors.verticalCenter: parent.verticalCenter
                                text:       qsTr("Place vehicle on a stable, horizontal surface and touch the <b>Start Gimbal Calibration</b> button to initiate the gimbal calibration. Do not move the vehicle while the calibration is in progress.<br><br>This procedure will take a few minutes to complete.")
                            }
                        }
                        ProgressBar {
                            width:          parent.width * 0.75
                            orientation:    Qt.Horizontal
                            minimumValue:   0
                            maximumValue:   100
                            value:          TyphoonHQuickInterface.cameraControl.gimbalProgress
                            visible:        TyphoonHQuickInterface.cameraControl.gimbalCalOn
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }
                Item {
                    width:  1
                    height: ScreenTools.defaultFontPixelHeight
                }
                QGCButton {
                    text:       "Start Gimbal Calibration"
                    enabled:    _activeVehicle && !TyphoonHQuickInterface.cameraControl.gimbalCalOn
                    onClicked:  TyphoonHQuickInterface.cameraControl.calibrateGimbal()
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }
    }
}
