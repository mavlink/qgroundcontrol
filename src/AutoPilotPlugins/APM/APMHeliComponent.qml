/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

            FactPanelController { id: controller; }

            QGCPalette { id: ggcPal; colorGroupEnabled: true }

            property real _margins:     ScreenTools.defaultFontPixelHeight
            property bool _showIcon:    !ScreenTools.isTinyScreen

            property Fact _hSvMan:          controller.getParameterFact(-1, "H_SV_MAN")
            property Fact _hSwType:         controller.getParameterFact(-1, "H_SW_TYPE")
            property Fact _hSwColDir:       controller.getParameterFact(-1, "H_SW_COL_DIR")
            property Fact _hSwLinSvo:       controller.getParameterFact(-1, "H_SW_LIN_SVO")
            property Fact _hFlybarMode:     controller.getParameterFact(-1, "H_FLYBAR_MODE")
            property Fact _hColMax:         controller.getParameterFact(-1, "H_COL_MAX")
            property Fact _hColMid:         controller.getParameterFact(-1, "H_COL_MID")
            property Fact _hColMin:         controller.getParameterFact(-1, "H_COL_MIN")
            property Fact _hCycMax:         controller.getParameterFact(-1, "H_CYC_MAX")

            property Fact _hRscMode:        controller.getParameterFact(-1, "H_RSC_MODE")
            property Fact _hRscCritical:    controller.getParameterFact(-1, "H_RSC_CRITICAL")
            property Fact _hRscRampTime:    controller.getParameterFact(-1, "H_RSC_RAMP_TIME")
            property Fact _hRscRunupTime:   controller.getParameterFact(-1, "H_RSC_RUNUP_TIME")
            property Fact _hRscSetpoint:    controller.getParameterFact(-1, "H_RSC_SETPOINT")
            property Fact _hRscIdle:        controller.getParameterFact(-1, "H_RSC_IDLE")
            property Fact _hRscThrcrv0:     controller.getParameterFact(-1, "H_RSC_THRCRV_0")
            property Fact _hRscThrcrv25:    controller.getParameterFact(-1, "H_RSC_THRCRV_25")
            property Fact _hRscThrcrv50:    controller.getParameterFact(-1, "H_RSC_THRCRV_50")
            property Fact _hRscThrcrv75:    controller.getParameterFact(-1, "H_RSC_THRCRV_75")
            property Fact _hRscThrcrv100:   controller.getParameterFact(-1, "H_RSC_THRCRV_100")

            property Fact _hRscGovSetpnt:   controller.getParameterFact(-1, "H_RSC_GOV_SETPNT")
            property Fact _hRscGovDisgag:   controller.getParameterFact(-1, "H_RSC_GOV_DISGAG")
            property Fact _hRscGovDroop:    controller.getParameterFact(-1, "H_RSC_GOV_DROOP")
            property Fact _hRscGovTcgain:   controller.getParameterFact(-1, "H_RSC_GOV_TCGAIN")
            property Fact _hRscGovRange:    controller.getParameterFact(-1, "H_RSC_GOV_RANGE")

            property Fact _imStbCol1:      controller.getParameterFact(-1, "IM_STB_COL_1")
            property Fact _imStbCol2:      controller.getParameterFact(-1, "IM_STB_COL_2")
            property Fact _imStbCol3:      controller.getParameterFact(-1, "IM_STB_COL_3")
            property Fact _imStbCol4:      controller.getParameterFact(-1, "IM_STB_COL_4")
            property Fact _hTailType:       controller.getParameterFact(-1, "H_TAIL_TYPE")
            property Fact _hTailSpeed:      controller.getParameterFact(-1, "H_TAIL_SPEED")
            property Fact _hGyrGain:        controller.getParameterFact(-1, "H_GYR_GAIN")
            property Fact _hGyrGainAcro:    controller.getParameterFact(-1, "H_GYR_GAIN_ACRO")
            property Fact _hColYaw:         controller.getParameterFact(-1, "H_COLYAW")

            QGCGroupBox {
                title: qsTr("Servo Setup")

                GridLayout {
                    columns: 6

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

                    QGCLabel { text: qsTr("5") }
                    FactComboBox {
                        fact:               controller.getParameterFact(-1, "SERVO5_FUNCTION")
                        indexModel:         false
                        Layout.fillWidth:   true
                    }
                    FactTextField {
                        fact:               controller.getParameterFact(-1, "SERVO5_MIN")
                        Layout.fillWidth:   true
                    }
                    FactTextField {
                        fact:               controller.getParameterFact(-1, "SERVO5_MAX")
                        Layout.fillWidth:   true
                    }
                    FactTextField {
                        fact:               controller.getParameterFact(-1, "SERVO5_TRIM")
                        Layout.fillWidth:   true
                    }
                    FactCheckBox {
                        fact:               controller.getParameterFact(-1, "SERVO5_REVERSED")
                        Layout.fillWidth:   true
                    }

                    QGCLabel { text: qsTr("6") }
                    FactComboBox {
                        fact:               controller.getParameterFact(-1, "SERVO6_FUNCTION")
                        indexModel:         false
                        Layout.fillWidth:   true
                    }
                    FactTextField {
                        fact:               controller.getParameterFact(-1, "SERVO6_MIN")
                        Layout.fillWidth:   true
                    }
                    FactTextField {
                        fact:               controller.getParameterFact(-1, "SERVO6_MAX")
                        Layout.fillWidth:   true
                    }
                    FactTextField {
                        fact:               controller.getParameterFact(-1, "SERVO6_TRIM")
                        Layout.fillWidth:   true
                    }
                    FactCheckBox {
                        fact:               controller.getParameterFact(-1, "SERVO6_REVERSED")
                        Layout.fillWidth:   true
                    }

                    QGCLabel { text: qsTr("7") }
                    FactComboBox {
                        fact:               controller.getParameterFact(-1, "SERVO7_FUNCTION")
                        indexModel:         false
                        Layout.fillWidth:   true
                    }
                    FactTextField {
                        fact:               controller.getParameterFact(-1, "SERVO7_MIN")
                        Layout.fillWidth:   true
                    }
                    FactTextField {
                        fact:               controller.getParameterFact(-1, "SERVO7_MAX")
                        Layout.fillWidth:   true
                    }
                    FactTextField {
                        fact:               controller.getParameterFact(-1, "SERVO7_TRIM")
                        Layout.fillWidth:   true
                    }
                    FactCheckBox {
                        fact:               controller.getParameterFact(-1, "SERVO7_REVERSED")
                        Layout.fillWidth:   true
                    }

                    QGCLabel { text: qsTr("8") }
                    FactComboBox {
                        fact:               controller.getParameterFact(-1, "SERVO8_FUNCTION")
                        indexModel:         false
                        Layout.fillWidth:   true
                    }
                    FactTextField {
                        fact:               controller.getParameterFact(-1, "SERVO8_MIN")
                        Layout.fillWidth:   true
                    }
                    FactTextField {
                        fact:               controller.getParameterFact(-1, "SERVO8_MAX")
                        Layout.fillWidth:   true
                    }
                    FactTextField {
                        fact:               controller.getParameterFact(-1, "SERVO8_TRIM")
                        Layout.fillWidth:   true
                    }
                    FactCheckBox {
                        fact:               controller.getParameterFact(-1, "SERVO8_REVERSED")
                        Layout.fillWidth:   true
                    }
                }
            }

            QGCGroupBox {
                title: qsTr("Swashplate Setup")

                GridLayout {
                    columns: 2

                    QGCLabel { text: _hSvMan.shortDescription }
                    FactComboBox {
                        fact:       _hSvMan
                        indexModel: false
                    }

                    QGCLabel { text: _hSwType.shortDescription }
                    FactComboBox {
                        fact:       _hSwType
                        indexModel: false
                    }

                    QGCLabel { text: _hSwColDir.shortDescription }
                    FactComboBox {
                        fact:       _hSwColDir
                        indexModel: false
                    }

                    QGCLabel { text: _hSwLinSvo.shortDescription }
                    FactComboBox {
                        fact:       _hSwLinSvo
                        indexModel: false
                    }

                    QGCLabel { text: _hFlybarMode.shortDescription }
                    FactComboBox {
                        fact:       _hFlybarMode
                        indexModel: false
                    }

                    QGCLabel { text: _hColMax.shortDescription }
                    FactTextField { fact: _hColMax }

                    QGCLabel { text: _hColMid.shortDescription }
                    FactTextField { fact: _hColMid }

                    QGCLabel { text: _hColMin.shortDescription }
                    FactTextField { fact: _hColMin }

                    QGCLabel { text: _hCycMax.shortDescription }
                    FactTextField { fact: _hCycMax }
                }
            }

            QGCGroupBox {
                title: qsTr("Throttle Settings")

                GridLayout {
                    columns: 2

                    QGCLabel { text: _hRscMode.shortDescription }
                    FactComboBox {
                        fact:       _hRscMode
                        indexModel: false
                    }

                    QGCLabel { text: _hRscCritical.shortDescription }
                    FactTextField { fact: _hRscCritical }

                    QGCLabel { text: _hRscRampTime.shortDescription }
                    FactTextField { fact: _hRscRampTime }

                    QGCLabel { text: _hRscRunupTime.shortDescription }
                    FactTextField { fact: _hRscRunupTime }

                    QGCLabel { text: _hRscSetpoint.shortDescription }
                    FactTextField { fact: _hRscSetpoint }

                    QGCLabel { text: _hRscIdle.shortDescription }
                    FactTextField { fact: _hRscIdle }

                    QGCLabel { text: _hRscThrcrv0.shortDescription }
                    FactTextField { fact: _hRscThrcrv0 }

                    QGCLabel { text: _hRscThrcrv25.shortDescription }
                    FactTextField { fact: _hRscThrcrv25 }

                    QGCLabel { text: _hRscThrcrv50.shortDescription }
                    FactTextField { fact: _hRscThrcrv50 }

                    QGCLabel { text: _hRscThrcrv75.shortDescription }
                    FactTextField { fact: _hRscThrcrv75 }

                    QGCLabel { text: _hRscThrcrv100.shortDescription }
                    FactTextField { fact: _hRscThrcrv100 }
                }
            }

            QGCGroupBox {
                title: qsTr("Governor Settings")

                GridLayout {
                    columns: 2

                    QGCLabel { text: _hRscGovSetpnt.shortDescription }
                    FactTextField { fact: _hRscGovSetpnt }

                    QGCLabel { text: _hRscGovDisgag.shortDescription }
                    FactTextField { fact: _hRscGovDisgag }

                    QGCLabel { text: _hRscGovDroop.shortDescription }
                    FactTextField { fact: _hRscGovDroop }

                    QGCLabel { text: _hRscGovTcgain.shortDescription }
                    FactTextField { fact: _hRscGovTcgain }

                    QGCLabel { text: _hRscGovRange.shortDescription }
                    FactTextField { fact: _hRscGovRange }
                }
            }

            QGCGroupBox {
                title: qsTr("Miscellaneous Settings")

                GridLayout {
                    columns: 2

                    QGCLabel { text: qsTr("* Stabilize Collective Curve *") }
                    QGCLabel { text: qsTr("") }

                    QGCLabel { text: _imStbCol1.shortDescription }
                    FactTextField { fact: _imStbCol1 }

                    QGCLabel { text: _imStbCol2.shortDescription }
                    FactTextField { fact: _imStbCol2 }

                    QGCLabel { text: _imStbCol3.shortDescription }
                    FactTextField { fact: _imStbCol3 }

                    QGCLabel { text: _imStbCol4.shortDescription }
                    FactTextField { fact: _imStbCol4 }

                    QGCLabel { text: qsTr("* Tail & Gyros *") }
                    QGCLabel { text: qsTr("") }

                    QGCLabel { text: _hTailType.shortDescription }
                    FactComboBox {
                        fact:       _hTailType
                        indexModel: false
                    }

                    QGCLabel { text: _hTailSpeed.shortDescription }
                    FactTextField { fact: _hTailSpeed }

                    QGCLabel { text: _hGyrGain.shortDescription }
                    FactTextField { fact: _hGyrGain }

                    QGCLabel { text: _hGyrGainAcro.shortDescription }
                    FactTextField { fact: _hGyrGainAcro }

                    QGCLabel { text: _hColYaw.shortDescription }
                    FactTextField { fact: _hColYaw }
                }
            }
        } // Flow
    } // Component
} // SetupView
