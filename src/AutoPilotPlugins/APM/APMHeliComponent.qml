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

            property real _margins:     ScreenTools.defaultFontPixelHeight
            property bool _showIcon:    !ScreenTools.isTinyScreen

            property Fact _hSvMan:      controller.getParameterFact(-1, "H_SV_MAN")
            property Fact _hSv1Pos:     controller.getParameterFact(-1, "H_SV1_POS")
            property Fact _hSv2Pos:     controller.getParameterFact(-1, "H_SV2_POS")
            property Fact _hSv3Pos:     controller.getParameterFact(-1, "H_SV3_POS")
            property Fact _hSwashType:  controller.getParameterFact(-1, "H_SWASH_TYPE")
            property Fact _hColCtrlDir: controller.getParameterFact(-1, "H_COL_CTRL_DIR")
            property Fact _hFlybarMode: controller.getParameterFact(-1, "H_FLYBAR_MODE")
            property Fact _hColMax:     controller.getParameterFact(-1, "H_COL_MAX")
            property Fact _hColMid:     controller.getParameterFact(-1, "H_COL_MID")
            property Fact _hColMin:     controller.getParameterFact(-1, "H_COL_MIN")
            property Fact _hCycMax:     controller.getParameterFact(-1, "H_CYC_MAX")

            property Fact _hRscMode:        controller.getParameterFact(-1, "H_RSC_MODE")
            property Fact _hRscCritical:    controller.getParameterFact(-1, "H_RSC_CRITICAL")
            property Fact _hRscIdle:        controller.getParameterFact(-1, "H_RSC_IDLE")
            property Fact _hRscRampTime:    controller.getParameterFact(-1, "H_RSC_RAMP_TIME")
            property Fact _hRscRunupTime:   controller.getParameterFact(-1, "H_RSC_RUNUP_TIME")
            property Fact _hRscSetpoint:    controller.getParameterFact(-1, "H_RSC_SETPOINT")
            property Fact _hRscThrcrv0:     controller.getParameterFact(-1, "H_RSC_THRCRV_0")
            property Fact _hRscThrcrv25:    controller.getParameterFact(-1, "H_RSC_THRCRV_25")
            property Fact _hRscThrcrv50:    controller.getParameterFact(-1, "H_RSC_THRCRV_50")
            property Fact _hRscThrcrv75:    controller.getParameterFact(-1, "H_RSC_THRCRV_75")
            property Fact _hRscThrcrv100:   controller.getParameterFact(-1, "H_RSC_THRCRV_100")

            property Fact _hLandColMin:     controller.getParameterFact(-1, "H_LAND_COL_MIN")
            property Fact _imStabCol1:      controller.getParameterFact(-1, "IM_STAB_COL_1")
            property Fact _imStabCol2:      controller.getParameterFact(-1, "IM_STAB_COL_2")
            property Fact _imStabCol3:      controller.getParameterFact(-1, "IM_STAB_COL_3")
            property Fact _imStabCol4:      controller.getParameterFact(-1, "IM_STAB_COL_4")

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
                }
            }

            QGCGroupBox {
                title: qsTr("Swash Setup")

                GridLayout {
                    columns: 2

                    QGCLabel { text: _hSvMan.shortDescription }
                    FactComboBox {
                        fact:       _hSvMan
                        indexModel: false
                    }

                    QGCLabel { text: _hSv1Pos.shortDescription }
                    FactTextField { fact: _hSv1Pos }

                    QGCLabel { text: _hSv2Pos.shortDescription }
                    FactTextField { fact: _hSv2Pos }

                    QGCLabel { text: _hSv3Pos.shortDescription }
                    FactTextField { fact: _hSv3Pos }

                    QGCLabel { text: _hSwashType.shortDescription }
                    FactComboBox {
                        fact:       _hSwashType
                        indexModel: false
                    }

                    QGCLabel { text: _hColCtrlDir.shortDescription }
                    FactComboBox {
                        fact:       _hColCtrlDir
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
                title: qsTr("Throttle Setup")

                GridLayout {
                    columns: 2

                    QGCLabel { text: _hRscMode.shortDescription }
                    FactComboBox {
                        fact:       _hRscMode
                        indexModel: false
                    }

                    QGCLabel { text: _hRscCritical.shortDescription }
                    FactTextField { fact: _hRscCritical }

                    QGCLabel { text: _hRscIdle.shortDescription }
                    FactTextField { fact: _hRscIdle }

                    QGCLabel { text: _hRscRampTime.shortDescription }
                    FactTextField { fact: _hRscRampTime }

                    QGCLabel { text: _hRscRunupTime.shortDescription }
                    FactTextField { fact: _hRscRunupTime }

                    QGCLabel { text: _hRscSetpoint.shortDescription }
                    FactTextField { fact: _hRscSetpoint }

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
                title: qsTr("Collective Curve Setup")

                GridLayout {
                    columns: 2

                    QGCLabel { text: _hLandColMin.shortDescription }
                    FactTextField { fact: _hLandColMin }

                    QGCLabel { text: _imStabCol1.shortDescription }
                    FactTextField { fact: _imStabCol1 }

                    QGCLabel { text: _imStabCol2.shortDescription }
                    FactTextField { fact: _imStabCol2 }

                    QGCLabel { text: _imStabCol3.shortDescription }
                    FactTextField { fact: _imStabCol3 }

                    QGCLabel { text: _imStabCol4.shortDescription }
                    FactTextField { fact: _imStabCol4 }
                }
            }
        } // Flow
    } // Component
} // SetupView
