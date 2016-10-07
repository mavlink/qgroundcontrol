/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick              2.5
import QtQuick.Controls     1.2
import QtGraphicalEffects   1.0

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

QGCView {
    id:                 _safetyView
    viewPanel:          panel
    anchors.fill:       parent

    FactPanelController { id: controller; factPanel: panel }

    QGCPalette { id: ggcPal; colorGroupEnabled: enabled }

    property Fact _failsafeGCSEnable:   controller.getParameterFact(-1, "FS_GCS_ENABLE")
    property Fact _failsafeBattEnable:  controller.getParameterFact(-1, "FS_BATT_ENABLE")
    property Fact _failsafeBattMah:     controller.getParameterFact(-1, "FS_BATT_MAH")
    property Fact _failsafeBattVoltage: controller.getParameterFact(-1, "FS_BATT_VOLTAGE")
    property Fact _failsafeThrEnable:   controller.getParameterFact(-1, "FS_THR_ENABLE")
    property Fact _failsafeThrValue:    controller.getParameterFact(-1, "FS_THR_VALUE")

    property Fact _fenceAction: controller.getParameterFact(-1, "FENCE_ACTION")
    property Fact _fenceAltMax: controller.getParameterFact(-1, "FENCE_ALT_MAX")
    property Fact _fenceEnable: controller.getParameterFact(-1, "FENCE_ENABLE")
    property Fact _fenceMargin: controller.getParameterFact(-1, "FENCE_MARGIN")
    property Fact _fenceType:   controller.getParameterFact(-1, "FENCE_TYPE")

    property Fact _leakPin:            controller.getParameterFact(-1, "WD_1_PIN")
    property Fact _leakLogic:          controller.getParameterFact(-1, "WD_1_DEFAULT")

    property Fact _armingCheck: controller.getParameterFact(-1, "ARMING_CHECK")

    property real _margins:     ScreenTools.defaultFontPixelHeight
    property bool _showIcon:    !ScreenTools.isTinyScreen

    ExclusiveGroup { id: fenceActionRadioGroup }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        QGCFlickable {
            clip:               true
            anchors.fill:       parent
            contentHeight:      flowLayout.height
            contentWidth:       flowLayout.width

            Flow {
                id:         flowLayout
                width:      panel.width // parent.width doesn't work for some reason
                spacing:    _margins

                Column {
                    spacing: _margins / 2

                    QGCLabel {
                        id:         failsafeLabel
                        text:       qsTr("Failsafe Triggers")
                        font.family: ScreenTools.demiboldFontFamily
                    }

                    Rectangle {
                        id:     failsafeSettings
                        width:  throttleEnableCombo.x + throttleEnableCombo.width + _margins
                        height: mahField.y + mahField.height + _margins
                        color:  ggcPal.windowShade

                        QGCLabel {
                            id:                 gcsEnableLabel
                            anchors.margins:    _margins
                            anchors.left:       parent.left
                            anchors.baseline:   gcsEnableCombo.baseline
                            text:               qsTr("Ground Station failsafe:")
                        }

                        FactComboBox {
                            id:                 gcsEnableCombo
                            anchors.topMargin:  _margins
                            anchors.leftMargin: _margins
                            anchors.left:       gcsEnableLabel.right
                            anchors.top:        parent.top
                            width:              voltageField.width
                            fact:               _failsafeGCSEnable
                            indexModel:         false
                        }

                        QGCLabel {
                            id:                 throttleEnableLabel
                            anchors.margins:    _margins
                            anchors.left:       parent.left
                            anchors.baseline:   throttleEnableCombo.baseline
                            text:               qsTr("Throttle failsafe:")
                        }

                        QGCComboBox {
                            id:                 throttleEnableCombo
                            anchors.topMargin:  _margins
                            anchors.left:       gcsEnableCombo.left
                            anchors.top:        gcsEnableCombo.bottom
                            width:              voltageField.width
                            model:              [qsTr("Disabled"), qsTr("Always RTL"),
                                qsTr("Continue with Mission in Auto Mode"), qsTr("Always Land")]
                            currentIndex:       _failsafeThrEnable.value

                            onActivated: _failsafeThrEnable.value = index
                        }

                        QGCLabel {
                            id:                 throttlePWMLabel
                            anchors.margins:    _margins
                            anchors.left:       parent.left
                            anchors.baseline:   throttlePWMField.baseline
                            text:               qsTr("PWM threshold:")
                        }

                        FactTextField {
                            id:                 throttlePWMField
                            anchors.topMargin:  _margins / 2
                            anchors.left:       gcsEnableCombo.left
                            anchors.top:        throttleEnableCombo.bottom
                            fact:               _failsafeThrValue
                            showUnits:          true
                        }

                        QGCLabel {
                            id:                 batteryEnableLabel
                            anchors.margins:    _margins
                            anchors.left:       parent.left
                            anchors.baseline:   batteryEnableCombo.baseline
                            text:               qsTr("Battery failsafe:")
                        }

                        QGCComboBox {
                            id:                 batteryEnableCombo
                            anchors.topMargin:  _margins
                            anchors.left:       gcsEnableCombo.left
                            anchors.top:        throttlePWMField.bottom
                            width:              voltageField.width
                            model:              [qsTr("Disabled"), qsTr("Land"), qsTr("Return to Launch")]
                            currentIndex:       _failsafeBattEnable.value

                            onActivated: _failsafeBattEnable.value = index
                        }

                        QGCCheckBox {
                            id:                 voltageLabel
                            anchors.margins:    _margins
                            anchors.left:       parent.left
                            anchors.baseline:   voltageField.baseline
                            text:               qsTr("Voltage threshold:")
                            checked:            _failsafeBattVoltage.value != 0

                            onClicked: _failsafeBattVoltage.value = checked ? 10.5 : 0
                        }

                        FactTextField {
                            id:                 voltageField
                            anchors.topMargin:  _margins / 2
                            anchors.left:       gcsEnableCombo.left
                            anchors.top:        batteryEnableCombo.bottom
                            fact:               _failsafeBattVoltage
                            showUnits:          true
                        }

                        QGCCheckBox {
                            id:                 mahLabel
                            anchors.margins:    _margins
                            anchors.left:       parent.left
                            anchors.baseline:   mahField.baseline
                            text:               qsTr("MAH threshold:")
                            checked:            _failsafeBattMah.value != 0

                            onClicked: _failsafeBattMah.value = checked ? 600 : 0
                        }

                        FactTextField {
                            id:                 mahField
                            anchors.topMargin:  _margins / 2
                            anchors.left:       gcsEnableCombo.left
                            anchors.top:        voltageField.bottom
                            fact:               _failsafeBattMah
                            showUnits:          true
                        }
                    } // Rectangle - Failsafe Settings
                } // Column - Failsafe Settings

                Column {
                    spacing: _margins / 2

                    QGCLabel {
                        id:             geoFenceLabel
                        text:           qsTr("GeoFence")
                        font.family:    ScreenTools.demiboldFontFamily
                    }

                    Rectangle {
                        id:     geoFenceSettings
                        width:  fenceAltMaxField.x + fenceAltMaxField.width + _margins
                        height: fenceAltMaxField.y + fenceAltMaxField.height + _margins
                        color:  ggcPal.windowShade

                        QGCCheckBox {
                            id:                 altitudeGeo
                            enabled:            false
                            anchors.margins:    _margins
                            anchors.left:       parent.left
                            anchors.top:        parent.top
                            text:               qsTr("Depth GeoFence enabled")
                            checked:            _fenceEnable.value != 0 && _fenceType.value & 1

                            onClicked: {
                                if (checked) {
                                    if (_fenceEnable.value == 1) {
                                        _fenceType.value |= 1
                                    } else {
                                        _fenceEnable.value = 1
                                        _fenceType.value = 1
                                    }
                                } else {
                                    _fenceEnable.value = 0
                                    _fenceType.value = 0
                                }
                            }
                        }

                        QGCRadioButton {
                            id:                 geoReportRadio
                            enabled:            false
                            anchors.margins:    _margins
                            anchors.left:       parent.left
                            anchors.top:        altitudeGeo.bottom
                            text:               qsTr("Report only")
                            exclusiveGroup:     fenceActionRadioGroup
                            checked:            _fenceAction.value == 0

                            onClicked: _fenceAction.value = 0
                        }

                        QGCRadioButton {
                            id:                 geoRTLRadio
                            enabled:            false
                            anchors.topMargin:  _margins / 2
                            anchors.left:       altitudeGeo.left
                            anchors.top:        geoReportRadio.bottom
                            text:               qsTr("RTL or Land")
                            exclusiveGroup:     fenceActionRadioGroup
                            checked:            _fenceAction.value == 1

                            onClicked: _fenceAction.value = 1
                        }

                        QGCLabel {
                            id:                 fenceAltMaxLabel
                            enabled:            false
                            anchors.left:       altitudeGeo.left
                            anchors.baseline:   fenceAltMaxField.baseline
                            text:               qsTr("Max depth:")
                        }

                        FactTextField {
                            id:                 fenceAltMaxField
                            enabled:            false
                            anchors.topMargin:  _margins / 2
                            anchors.leftMargin: _margins
                            anchors.left:       fenceAltMaxLabel.right
                            anchors.top:        geoRTLRadio.bottom
                            fact:               _fenceAltMax
                            showUnits:          true
                        }
                    } // Rectangle - GeoFence Settings
                } // Column - GeoFence Settings

                Column {
                    spacing: _margins / 2

                    QGCLabel {
                        id:             leakDetectorLabel
                        text:           qsTr("Leak Detector")
                        font.family:    ScreenTools.demiboldFontFamily
                    }

                    Rectangle {
                        id:     leakDetectorSettings
                        width:  leakLogicCombo.x + leakLogicCombo.width + _margins
                        height: leakLogicCombo.y + leakLogicCombo.height + _margins
                        color:  ggcPal.windowShade

                        QGCLabel {
                            id:                 leakPinLabel
                            anchors.margins:    _margins
                            anchors.left:       parent.left
                            anchors.top:        parent.top
                            text:               qsTr("Pin:")
                        }

                        FactComboBox {
                            id:                 leakPinCombo
                            anchors.margins:    _margins
                            anchors.left:       leakLogicLabel.right
                            anchors.baseline:   leakPinLabel.baseline
                            width:              voltageField.width
                            fact:               _leakPin
                            indexModel:         false
                        }

                        QGCLabel {
                            id:                 leakLogicLabel
                            anchors.margins:    _margins
                            anchors.left:       parent.left
                            anchors.top:        leakPinLabel.bottom
                            text:               qsTr("Logic:")
                        }

                        FactComboBox {
                            id:                 leakLogicCombo
                            anchors.margins:    _margins
                            anchors.left:       leakLogicLabel.right
                            anchors.baseline:   leakLogicLabel.baseline
                            width:              voltageField.width
                            fact:               _leakLogic
                            indexModel:         false
                        }
                    } // Rectangle - Leak Detector Settings
                } // Column - Leak Detector Settings

                Column {
                    spacing: _margins / 2

                    QGCLabel {
                        text:           qsTr("Arming Checks")
                        font.family:    ScreenTools.demiboldFontFamily
                    }

                    Rectangle {
                        width:  flowLayout.width
                        height: armingCheckInnerColumn.height + (_margins * 2)
                        color:  ggcPal.windowShade

                        Column {
                            id:                 armingCheckInnerColumn
                            anchors.margins:    _margins
                            anchors.top:        parent.top
                            anchors.left:       parent.left
                            anchors.right:      parent.right
                            spacing: _margins

                            FactBitmask {
                                id:                 armingCheckBitmask
                                anchors.left:       parent.left
                                anchors.right:      parent.right
                                firstEntryIsAll:    true
                                fact:               _armingCheck
                            }

                            QGCLabel {
                                id:             armingCheckWarning
                                anchors.left:   parent.left
                                anchors.right:  parent.right
                                wrapMode:       Text.WordWrap
                                color:          qgcPal.warningText
                                text:            qsTr("Warning: Turning off arming checks can lead to loss of Vehicle control.")
                                visible:        _armingCheck.value != 1
                            }
                        }
                    } // Rectangle - Arming checks
                } // Column - Arming Checks
            } // Flow
        } // QGCFlickable
    } // QGCViewPanel
} // QGCView
