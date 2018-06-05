/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick              2.3
import QtQuick.Controls     1.2
import QtGraphicalEffects   1.0
import QtQuick.Layouts      1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

SetupPage {
    id:             safetyPage
    pageComponent:  safetyPageComponent

    Component {
        id: safetyPageComponent

        Flow {
            id:         flowLayout
            width:      availableWidth
            spacing:    _margins

            FactPanelController { id: controller; factPanel: safetyPage.viewPanel }

            QGCPalette { id: ggcPal; colorGroupEnabled: true }

            property Fact _failsafeGCSEnable:               controller.getParameterFact(-1, "FS_GCS_ENABLE")
            property Fact _failsafeBattLowAct:              controller.getParameterFact(-1, "r.BATT_FS_LOW_ACT")
            property Fact _failsafeBattMah:                 controller.getParameterFact(-1, "r.BATT_LOW_MAH")
            property Fact _failsafeBattVoltage:             controller.getParameterFact(-1, "r.BATT_LOW_VOLT")
            property Fact _failsafeThrEnable:               controller.getParameterFact(-1, "FS_THR_ENABLE")
            property Fact _failsafeThrValue:                controller.getParameterFact(-1, "FS_THR_VALUE")

            property bool _failsafeBattCritActAvailable:    controller.parameterExists(-1, "BATT_FS_CRT_ACT")
            property bool _failsafeBatt2LowActAvailable:    controller.parameterExists(-1, "BATT2_FS_LOW_ACT")
            property bool _failsafeBatt2CritActAvailable:   controller.parameterExists(-1, "BATT2_FS_CRT_ACT")
            property bool _batt2MonitorAvailable:           controller.parameterExists(-1, "BATT2_MONITOR")
            property bool _batt2MonitorEnabled:             _batt2MonitorAvailable ? _batt2Monitor.rawValue !== 0 : false

            property Fact _failsafeBattCritAct:             controller.getParameterFact(-1, "BATT_FS_CRT_ACT", false /* reportMissing */)
            property Fact _batt2Monitor:                    controller.getParameterFact(-1, "BATT2_MONITOR", false /* reportMissing */)
            property Fact _failsafeBatt2LowAct:             controller.getParameterFact(-1, "BATT2_FS_LOW_ACT", false /* reportMissing */)
            property Fact _failsafeBatt2CritAct:            controller.getParameterFact(-1, "BATT2_FS_CRT_ACT", false /* reportMissing */)
            property Fact _failsafeBatt2Mah:                controller.getParameterFact(-1, "BATT2_LOW_MAH", false /* reportMissing */)
            property Fact _failsafeBatt2Voltage:            controller.getParameterFact(-1, "BATT2_LOW_VOLT", false /* reportMissing */)

            property Fact _fenceAction: controller.getParameterFact(-1, "FENCE_ACTION")
            property Fact _fenceAltMax: controller.getParameterFact(-1, "FENCE_ALT_MAX")
            property Fact _fenceEnable: controller.getParameterFact(-1, "FENCE_ENABLE")
            property Fact _fenceMargin: controller.getParameterFact(-1, "FENCE_MARGIN")
            property Fact _fenceRadius: controller.getParameterFact(-1, "FENCE_RADIUS")
            property Fact _fenceType:   controller.getParameterFact(-1, "FENCE_TYPE")

            property Fact _landSpeedFact:   controller.getParameterFact(-1, "LAND_SPEED")
            property Fact _rtlAltFact:      controller.getParameterFact(-1, "RTL_ALT")
            property Fact _rtlLoitTimeFact: controller.getParameterFact(-1, "RTL_LOIT_TIME")
            property Fact _rtlAltFinalFact: controller.getParameterFact(-1, "RTL_ALT_FINAL")

            property Fact _armingCheck: controller.getParameterFact(-1, "ARMING_CHECK")

            property real _margins:     ScreenTools.defaultFontPixelHeight
            property bool _showIcon:    !ScreenTools.isTinyScreen

            ExclusiveGroup { id: fenceActionRadioGroup }
            ExclusiveGroup { id: landLoiterRadioGroup }
            ExclusiveGroup { id: returnAltRadioGroup }

            Column {
                spacing: _margins / 2

                QGCLabel {
                    text:       qsTr("Servo Setup")
                    font.family: ScreenTools.demiboldFontFamily
                }

                Rectangle {
                    width:  servoGrid.x + servoGrid.width + _margins
                    height: servoGrid.y + servoGrid.height + _margins
                    color:  ggcPal.windowShade

                    GridLayout {
                        id:         servoGrid
                        columns:    6

                        QGCLabel { text: qsTr("Servo") }
                        QGCLabel { text: qsTr("Function") }
                        QGCLabel { text: qsTr("Min") }
                        QGCLabel { text: qsTr("Max") }
                        QGCLabel { text: qsTr("Trim") }
                        QGCLabel { text: qsTr("Reversed") }

                        QGCLabel { text: qsTr("1") }
                        FactComboBox {
                            fact:               controller.getParameterFact(-1, "SERVO1_FUNCTION")
                            indexModel:         false
                            Layout.fillWidth:   true
                        }
                        FactTextField {
                            fact:               controller.getParameterFact(-1, "SERVO1_MIN")
                            Layout.fillWidth:   true
                        }
                        FactTextField {
                            fact:               controller.getParameterFact(-1, "SERVO1_MAX")
                            Layout.fillWidth:   true
                        }
                        FactTextField {
                            fact:               controller.getParameterFact(-1, "SERVO1_TRIM")
                            Layout.fillWidth:   true
                        }
                        FactCheckBox {
                            fact:               controller.getParameterFact(-1, "SERVO1_REVERSED")
                            Layout.fillWidth:   true
                        }

                        QGCLabel { text: qsTr("2") }
                        FactComboBox {
                            fact:               controller.getParameterFact(-1, "SERVO2_FUNCTION")
                            indexModel:         false
                            Layout.fillWidth:   true
                        }
                        FactTextField {
                            fact:               controller.getParameterFact(-1, "SERVO2_MIN")
                            Layout.fillWidth:   true
                        }
                        FactTextField {
                            fact:               controller.getParameterFact(-1, "SERVO2_MAX")
                            Layout.fillWidth:   true
                        }
                        FactTextField {
                            fact:               controller.getParameterFact(-1, "SERVO2_TRIM")
                            Layout.fillWidth:   true
                        }
                        FactCheckBox {
                            fact:               controller.getParameterFact(-1, "SERVO2_REVERSED")
                            Layout.fillWidth:   true
                        }

                        QGCLabel { text: qsTr("3") }
                        FactComboBox {
                            fact:               controller.getParameterFact(-1, "SERVO3_FUNCTION")
                            indexModel:         false
                            Layout.fillWidth:   true
                        }
                        FactTextField {
                            fact:               controller.getParameterFact(-1, "SERVO3_MIN")
                            Layout.fillWidth:   true
                        }
                        FactTextField {
                            fact:               controller.getParameterFact(-1, "SERVO3_MAX")
                            Layout.fillWidth:   true
                        }
                        FactTextField {
                            fact:               controller.getParameterFact(-1, "SERVO3_TRIM")
                            Layout.fillWidth:   true
                        }
                        FactCheckBox {
                            fact:               controller.getParameterFact(-1, "SERVO3_REVERSED")
                            Layout.fillWidth:   true
                        }

                        QGCLabel { text: qsTr("4") }
                        FactComboBox {
                            fact:               controller.getParameterFact(-1, "SERVO4_FUNCTION")
                            indexModel:         false
                            Layout.fillWidth:   true
                        }
                        FactTextField {
                            fact:               controller.getParameterFact(-1, "SERVO4_MIN")
                            Layout.fillWidth:   true
                        }
                        FactTextField {
                            fact:               controller.getParameterFact(-1, "SERVO4_MAX")
                            Layout.fillWidth:   true
                        }
                        FactTextField {
                            fact:               controller.getParameterFact(-1, "SERVO4_TRIM")
                            Layout.fillWidth:   true
                        }
                        FactCheckBox {
                            fact:               controller.getParameterFact(-1, "SERVO4_REVERSED")
                            Layout.fillWidth:   true
                        }
                    } // GridLayout
                } // Rectangle
            } // Column
        } // Flow
    } // Component
} // SetupView
