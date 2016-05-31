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

    QGCPalette { id: palette; colorGroupEnabled: enabled }

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
    property Fact _fenceRadius: controller.getParameterFact(-1, "FENCE_RADIUS")
    property Fact _fenceType:   controller.getParameterFact(-1, "FENCE_TYPE")

    property Fact _landSpeedFact:   controller.getParameterFact(-1, "LAND_SPEED")
    property Fact _rtlAltFact:      controller.getParameterFact(-1, "RTL_ALT")
    property Fact _rtlLoitTimeFact: controller.getParameterFact(-1, "RTL_LOIT_TIME")
    property Fact _rtlAltFinalFact: controller.getParameterFact(-1, "RTL_ALT_FINAL")

    property Fact _armingCheck: controller.getParameterFact(-1, "ARMING_CHECK")

    property real _margins: ScreenTools.defaultFontPixelHeight

    ExclusiveGroup { id: fenceActionRadioGroup }
    ExclusiveGroup { id: landLoiterRadioGroup }
    ExclusiveGroup { id: returnAltRadioGroup }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        QGCFlickable {
            clip:               true
            anchors.fill:       parent
            contentHeight:      armingCheckSettings.y + armingCheckSettings.height
            contentWidth:       armingCheckSettings.x + armingCheckSettings.width

            QGCLabel {
                id:         failsafeLabel
                text:       qsTr("Failsafe Triggers")
                font.family: ScreenTools.demiboldFontFamily
            }

            Rectangle {
                id:                     failsafeSettings
                anchors.topMargin:      _margins / 2
                anchors.rightMargin:    _margins
                anchors.left:           parent.left
                anchors.top:            failsafeLabel.bottom
                width:                  throttleEnableCombo.x + throttleEnableCombo.width + _margins
                height:                 mahField.y + mahField.height + _margins
                color:                  palette.windowShade

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

            QGCLabel {
                id:                 geoFenceLabel
                anchors.leftMargin: _margins
                anchors.left:       failsafeSettings.right
                anchors.top:        parent.top
                text:               qsTr("GeoFence")
                font.family:        ScreenTools.demiboldFontFamily
            }

            Rectangle {
                id:                 geoFenceSettings
                anchors.topMargin:  _margins / 2
                anchors.left:       geoFenceLabel.left
                anchors.top:        geoFenceLabel.bottom
                anchors.bottom:     failsafeSettings.bottom
                width:              fenceAltMaxField.x + fenceAltMaxField.width + _margins
                color:              palette.windowShade

                QGCCheckBox {
                    id:                 circleGeo
                    anchors.margins:    _margins
                    anchors.left:       parent.left
                    anchors.top:        parent.top
                    text:               qsTr("Circle GeoFence enabled")
                    checked:            _fenceEnable.value != 0 && _fenceType.value & 2

                    onClicked: {
                        if (checked) {
                            if (_fenceEnable.value == 1) {
                                _fenceType.value |= 2
                            } else {
                                _fenceEnable.value = 1
                                _fenceType.value = 2
                            }
                        } else if (altitudeGeo.checked) {
                            _fenceType.value &= ~2
                        } else {
                            _fenceEnable.value = 0
                            _fenceType.value = 0
                        }
                    }
                }

                QGCCheckBox {
                    id:                 altitudeGeo
                    anchors.topMargin:  _margins / 2
                    anchors.left:       circleGeo.left
                    anchors.top:        circleGeo.bottom
                    text:               qsTr("Altitude GeoFence enabled")
                    checked:            _fenceEnable.value != 0 && _fenceType.value & 1

                    onClicked: {
                        if (checked) {
                            if (_fenceEnable.value == 1) {
                                _fenceType.value |= 1
                            } else {
                                _fenceEnable.value = 1
                                _fenceType.value = 1
                            }
                        } else if (circleGeo.checked) {
                            _fenceType.value &= ~1
                        } else {
                            _fenceEnable.value = 0
                            _fenceType.value = 0
                        }
                    }
                }

                QGCRadioButton {
                    id:                 geoReportRadio
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
                    anchors.topMargin:  _margins / 2
                    anchors.left:       circleGeo.left
                    anchors.top:        geoReportRadio.bottom
                    text:               qsTr("RTL or Land")
                    exclusiveGroup:     fenceActionRadioGroup
                    checked:            _fenceAction.value == 1

                    onClicked: _fenceAction.value = 1
                }

                QGCLabel {
                    id:                 fenceRadiusLabel
                    anchors.left:       circleGeo.left
                    anchors.baseline:   fenceRadiusField.baseline
                    text:               qsTr("Max radius:")
                }

                FactTextField {
                    id:                 fenceRadiusField
                    anchors.topMargin:  _margins
                    anchors.left:       fenceAltMaxField.left
                    anchors.top:        geoRTLRadio.bottom
                    fact:               _fenceRadius
                    showUnits:          true
                }

                QGCLabel {
                    id:                 fenceAltMaxLabel
                    anchors.left:       circleGeo.left
                    anchors.baseline:   fenceAltMaxField.baseline
                    text:               qsTr("Max altitude:")
                }

                FactTextField {
                    id:                 fenceAltMaxField
                    anchors.topMargin:  _margins / 2
                    anchors.leftMargin: _margin
                    anchors.left:       fenceAltMaxLabel.right
                    anchors.top:        fenceRadiusField.bottom
                    fact:               _fenceAltMax
                    showUnits:          true
                }
            } // Rectangle - GeoFence Settings

            QGCLabel {
                id:                 rtlLabel
                anchors.topMargin:  _margins
                anchors.top:        geoFenceSettings.bottom
                text:               qsTr("Return to Launch")
                font.family:        ScreenTools.demiboldFontFamily
            }

            Rectangle {
                id:                 rtlSettings
                anchors.topMargin:  _margins / 2
                anchors.left:       parent.left
                anchors.top:        rtlLabel.bottom
                width:              rltAltFinalField.x + rltAltFinalField.width + _margins
                height:             rltAltFinalField.y + rltAltFinalField.height + _margins
                color:              palette.windowShade

                Image {
                    id:                 icon
                    anchors.margins:    _margins
                    anchors.left:       parent.left
                    anchors.top:        parent.top
                    height:             ScreenTools.defaultFontPixelWidth * 20
                    width:              ScreenTools.defaultFontPixelWidth * 20
                    sourceSize.width:   width
                    mipmap:             true
                    fillMode:           Image.PreserveAspectFit
                    visible:            false
                    source:             "/qmlimages/ReturnToHomeAltitude.svg"
                }

                ColorOverlay {
                    anchors.fill:   icon
                    source:         icon
                    color:          palette.text
                }

                QGCRadioButton {
                    id:                 returnAtCurrentRadio
                    anchors.leftMargin: _margins
                    anchors.left:       icon.right
                    anchors.top:        icon.top
                    text:               qsTr("Return at current altitude")
                    checked:            _rtlAltFact.value == 0
                    exclusiveGroup:     returnAltRadioGroup

                    onClicked: _rtlAltFact.value = 0
                }

                QGCRadioButton {
                    id:                 returnAltRadio
                    anchors.topMargin:  _margins
                    anchors.left:       returnAtCurrentRadio.left
                    anchors.top:        returnAtCurrentRadio.bottom
                    text:               qsTr("Return at specified altitude:")
                    exclusiveGroup:     returnAltRadioGroup
                    checked:            _rtlAltFact.value != 0

                    onClicked: _rtlAltFact.value = 1500
                }

                FactTextField {
                    id:                 rltAltField
                    anchors.leftMargin: _margins
                    anchors.left:       returnAltRadio.right
                    anchors.baseline:   returnAltRadio.baseline
                    fact:               _rtlAltFact
                    showUnits:          true
                    enabled:            returnAltRadio.checked
                }

                QGCCheckBox {
                    id:                 homeLoiterCheckbox
                    anchors.left:       returnAtCurrentRadio.left
                    anchors.baseline:   landDelayField.baseline
                    checked:            _rtlLoitTimeFact.value > 0
                    text:               qsTr("Loiter above Home for:")

                    onClicked: _rtlLoitTimeFact.value = (checked ? 60 : 0)
                }

                FactTextField {
                    id:                 landDelayField
                    anchors.topMargin:  _margins * 1.5
                    anchors.left:       rltAltField.left
                    anchors.top:        rltAltField.bottom
                    fact:               _rtlLoitTimeFact
                    showUnits:          true
                    enabled:            homeLoiterCheckbox.checked === true
                }

                QGCRadioButton {
                    id:                 landRadio
                    anchors.left:       returnAtCurrentRadio.left
                    anchors.baseline:   landSpeedField.baseline
                    text:               qsTr("Land with descent speed:")
                    checked:            _rtlAltFinalFact.value == 0
                    exclusiveGroup:     landLoiterRadioGroup

                    onClicked: _rtlAltFinalFact.value = 0
                }

                FactTextField {
                    id:                 landSpeedField
                    anchors.topMargin:  _margins * 1.5
                    anchors.top:        landDelayField.bottom
                    anchors.left:       rltAltField.left
                    fact:               _landSpeedFact
                    showUnits:          true
                    enabled:            landRadio.checked
                }

                QGCRadioButton {
                    id:                 finalLoiterRadio
                    anchors.left:       returnAtCurrentRadio.left
                    anchors.baseline:   rltAltFinalField.baseline
                    text:               qsTr("Final loiter altitude:")
                    exclusiveGroup:     landLoiterRadioGroup

                    onClicked: _rtlAltFinalFact.value = _rtlAltFact.value
                }

                FactTextField {
                    id:                 rltAltFinalField
                    anchors.topMargin:  _margins / 2
                    anchors.left:       rltAltField.left
                    anchors.top:        landSpeedField.bottom
                    fact:               _rtlAltFinalFact
                    enabled:            finalLoiterRadio.checked
                    showUnits:          true
                }
            } // Rectangle - RTL Settings

            QGCLabel {
                id:                 armingCheckLabel
                anchors.topMargin:  _margins
                anchors.left:       parent.left
                anchors.top:        rtlSettings.bottom
                text:               qsTr("Arming Checks")
                font.family:        ScreenTools.demiboldFontFamily
            }

            Rectangle {
                id:                 armingCheckSettings
                anchors.topMargin:  _margins / 2
                anchors.left:       parent.left
                anchors.top:        armingCheckLabel.bottom
                width:              armingCheckColumn.x + armingCheckColumn.width + _margins
                height:             armingCheckColumn.y + armingCheckColumn.height + _margins
                color:              palette.windowShade

                Column {
                    id:         armingCheckColumn
                    spacing:    _margins

                    QGCLabel { text: qsTr("Be very careful when turning off arming checks. Could lead to loss of Vehicle control.") }
                    FactBitmask { fact: _armingCheck }
                }
            }
        } // QGCFlickable
    } // QGCViewPanel
} // QGCView
