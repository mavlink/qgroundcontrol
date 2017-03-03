/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick              2.7
import QtQuick.Controls     1.4
import QtGraphicalEffects   1.0

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

SetupPage {
    id:                 safetyPage
    pageComponent:      safetyPageComponent

    Component {
        id: safetyPageComponent

        Flow {
            id:         flowLayout
            width:      availableWidth
            spacing:    _margins

            FactPanelController { id: controller; factPanel: safetyPage.viewPanel }

            QGCPalette { id: ggcPal; colorGroupEnabled: true }

            property Fact _failsafeGCSEnable:   controller.getParameterFact(-1, "FS_GCS_ENABLE")
            property Fact _failsafeLeakEnable:  controller.getParameterFact(-1, "FS_LEAK_ENABLE")
            property Fact _failsafePressureEnable: controller.getParameterFact(-1, "FS_PRESS_ENABLE")
            property Fact _failsafePressureValue:  controller.getParameterFact(-1, "FS_PRESS_MAX")
            property Fact _failsafeTempEnable:  controller.getParameterFact(-1, "FS_TEMP_ENABLE")
            property Fact _failsafeTempValue:   controller.getParameterFact(-1, "FS_TEMP_MAX")

            property Fact _fenceAction: controller.getParameterFact(-1, "FENCE_ACTION")
            property Fact _fenceAltMax: controller.getParameterFact(-1, "FENCE_ALT_MAX")
            property Fact _fenceEnable: controller.getParameterFact(-1, "FENCE_ENABLE")
            property Fact _fenceMargin: controller.getParameterFact(-1, "FENCE_MARGIN")
            property Fact _fenceType:   controller.getParameterFact(-1, "FENCE_TYPE")

            property Fact _leakPin:            controller.getParameterFact(-1, "LEAK1_PIN")
            property Fact _leakLogic:          controller.getParameterFact(-1, "LEAK1_LOGIC")

            property Fact _armingCheck: controller.getParameterFact(-1, "ARMING_CHECK")

            property real _margins:     ScreenTools.defaultFontPixelHeight
            property bool _showIcon:    !ScreenTools.isTinyScreen

            ExclusiveGroup { id: fenceActionRadioGroup }

            Column {
                spacing: _margins / 2

                QGCLabel {
                    id:         failsafeLabel
                    text:       qsTr("Failsafe Actions")
                    font.family: ScreenTools.demiboldFontFamily
                }

                Rectangle {
                    id:     failsafeSettings
                    width:  leakEnableCombo.x + leakEnableCombo.width + _margins
                    height: leakEnableCombo.y + leakEnableCombo.height + _margins
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
                        anchors.margins:    _margins
                        anchors.left:       gcsEnableLabel.right
                        anchors.top:        parent.top
                        width:              ScreenTools.defaultFontPixelWidth*15
                        fact:               _failsafeGCSEnable
                        indexModel:         false
                    }

                    QGCLabel {
                        id:                 leakEnableLabel
                        anchors.margins:    _margins
                        anchors.left:       parent.left
                        anchors.baseline:   leakEnableCombo.baseline
                        text:               qsTr("Leak failsafe:")
                    }

                    FactComboBox {
                        id:                 leakEnableCombo
                        anchors.topMargin:  _margins
                        anchors.left:       gcsEnableCombo.left
                        anchors.top:        gcsEnableCombo.bottom
                        width:              ScreenTools.defaultFontPixelWidth*15
                        fact:               _failsafeLeakEnable
                        indexModel:         false
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
                        anchors.margins:    _margins
                        anchors.left:       parent.left
                        anchors.top:        parent.top
                        text:               qsTr("Depth GeoFence enabled\n(report only)")
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

                    QGCLabel {
                        id:                 fenceAltMaxLabel
                        anchors.left:       altitudeGeo.left
                        anchors.baseline:   fenceAltMaxField.baseline
                        text:               qsTr("Max depth:")
                    }

                    FactTextField {
                        id:                 fenceAltMaxField
                        anchors.topMargin:  _margins / 2
                        anchors.leftMargin: _margins
                        anchors.left:       fenceAltMaxLabel.right
                        anchors.top:        altitudeGeo.bottom
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
                        width:              ScreenTools.defaultFontPixelWidth*15
                        fact:               _leakPin
                        indexModel:         false
                    }

                    QGCLabel {
                        id:                 leakLogicLabel
                        anchors.margins:    _margins
                        anchors.left:       parent.left
                        anchors.top:        leakPinLabel.bottom
                        text:               qsTr("Logic (when dry):")
                    }

                    FactComboBox {
                        id:                 leakLogicCombo
                        anchors.margins:    _margins
                        anchors.left:       leakLogicLabel.right
                        anchors.baseline:   leakLogicLabel.baseline
                        width:              ScreenTools.defaultFontPixelWidth*15
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
    } // Component - safetyPageComponent
} // SetupView
