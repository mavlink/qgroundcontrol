/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick
import QtQuick.Controls

import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.Palette
import QGroundControl.Controls
import QGroundControl.ScreenTools

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

            QGCPalette { id: qgcPal; colorGroupEnabled: true }

            property Fact _failsafeBattMah:     controller.getParameterFact(-1, "r.BATT_LOW_MAH")
            property Fact _failsafeBattVoltage: controller.getParameterFact(-1, "r.BATT_LOW_VOLT")
            property Fact _failsafeThrEnable:   controller.getParameterFact(-1, "THR_FAILSAFE")
            property Fact _failsafeThrValue:    controller.getParameterFact(-1, "THR_FS_VALUE")
            property Fact _failsafeGCSEnable:   controller.getParameterFact(-1, "FS_GCS_ENABL")

            property Fact _rtlAltFact: {
                if (controller.firmwareMajorVersion < 4 || (controller.firmwareMajorVersion === 4 && controller.firmwareMinorVersion < 5)) {
                    return controller.getParameterFact(-1, "ALT_HOLD_RTL")
                } else {
                    return controller.getParameterFact(-1, "RTL_ALTITUDE")
                }
            }

            property real _margins: ScreenTools.defaultFontPixelHeight

            ExclusiveGroup { id: returnAltRadioGroup }

            Column {
                spacing: _margins / 2

                QGCLabel {
                    text:       qsTr("Failsafe Triggers")
                    font.bold:   true
                }

                Rectangle {
                    width:  throttlePWMField.x + throttlePWMField.width + _margins
                    height: gcsCheckbox.y + gcsCheckbox.height + _margins
                    color:  qgcPal.windowShade

                    QGCCheckBox {
                        id:                 throttleEnableCheckBox
                        anchors.margins:    _margins
                        anchors.left:       parent.left
                        anchors.baseline:   throttlePWMField.baseline
                        text:               qsTr("Throttle PWM threshold:")
                        checked:            _failsafeThrEnable.value == 1

                        onClicked: _failsafeThrEnable.value = (checked ? 1 : 0)
                    }

                    FactTextField {
                        id:                 throttlePWMField
                        anchors.margins:    _margins
                        anchors.left:       throttleEnableCheckBox.right
                        anchors.top:        parent.top
                        fact:               _failsafeThrValue
                        showUnits:          true
                        enabled:            throttleEnableCheckBox.checked
                    }

                    QGCCheckBox {
                        id:                 voltageCheckBox
                        anchors.margins:    _margins
                        anchors.left:       parent.left
                        anchors.baseline:   voltageField.baseline
                        text:               qsTr("Voltage threshold:")
                        checked:            _failsafeBattVoltage.value != 0

                        onClicked: _failsafeBattVoltage.value = checked ? 10.5 : 0
                    }

                    FactTextField {
                        id:                 voltageField
                        anchors.topMargin:  _margins
                        anchors.left:       throttlePWMField.left
                        anchors.top:        throttlePWMField.bottom
                        fact:               _failsafeBattVoltage
                        showUnits:          true
                        enabled:            voltageCheckBox.checked
                    }

                    QGCCheckBox {
                        id:                 mahCheckBox
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
                        anchors.left:       throttlePWMField.left
                        anchors.top:        voltageField.bottom
                        fact:               _failsafeBattMah
                        showUnits:          true
                        enabled:            mahCheckBox.checked
                    }

                    QGCCheckBox {
                        id:                 gcsCheckbox
                        anchors.margins:    _margins
                        anchors.left:       parent.left
                        anchors.top:        mahField.bottom
                        text:               qsTr("GCS failsafe")
                        checked:            _failsafeGCSEnable.value != 0

                        onClicked: _failsafeGCSEnable.value = checked ? 1 : 0
                    }
                } // Rectangle - Failsafe trigger settings
            } // Column - Failsafe trigger settings

            Column {
                spacing: _margins / 2

                QGCLabel {
                    text:           qsTr("Return to Launch")
                    font.bold:      true
                }

                Rectangle {
                    width:  rltAltField.x + rltAltField.width + _margins
                    height: rltAltField.y + rltAltField.height + _margins
                    color:  qgcPal.windowShade

                    QGCRadioButton {
                        id:                 returnAtCurrentRadio
                        anchors.margins:    _margins
                        anchors.left:       parent.left
                        anchors.top:        parent.top
                        text:               qsTr("Return at current altitude")
                        checked:            _rtlAltFact.value < 0
                        exclusiveGroup:     returnAltRadioGroup

                        onClicked: _rtlAltFact.value = -1
                    }

                    QGCRadioButton {
                        id:                 returnAltRadio
                        anchors.topMargin:  _margins / 2
                        anchors.left:       returnAtCurrentRadio.left
                        anchors.top:        returnAtCurrentRadio.bottom
                        text:               qsTr("Return at specified altitude:")
                        exclusiveGroup:     returnAltRadioGroup
                        checked:            _rtlAltFact.value >= 0

                        onClicked: _rtlAltFact.value = 10000
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
                } // Rectangle - RTL Settings
            } // Column - RTL Settings
        } // Flow
    } // Component
} // SetupView
