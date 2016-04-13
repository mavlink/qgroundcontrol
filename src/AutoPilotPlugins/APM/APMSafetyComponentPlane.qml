/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

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

    property Fact _failsafeBattMah:     controller.getParameterFact(-1, "FS_BATT_MAH")
    property Fact _failsafeBattVoltage: controller.getParameterFact(-1, "FS_BATT_VOLTAGE")
    property Fact _failsafeThrEnable:   controller.getParameterFact(-1, "THR_FAILSAFE")
    property Fact _failsafeThrValue:    controller.getParameterFact(-1, "THR_FS_VALUE")
    property Fact _failsafeGCSEnable:   controller.getParameterFact(-1, "FS_GCS_ENABL")

    property Fact _rtlAltFact: controller.getParameterFact(-1, "ALT_HOLD_RTL")

    property real _margins: ScreenTools.defaultFontPixelHeight

    ExclusiveGroup { id: returnAltRadioGroup }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        QGCFlickable {
            clip:               true
            anchors.fill:       parent
            contentHeight:      rtlSettings.y + rtlSettings.height
            flickableDirection: Flickable.VerticalFlick

            QGCLabel {
                id:         failsafeTriggersLabel
                text:       qsTr("Failsafe Triggers")
                font.weight: Font.DemiBold
            }

            Rectangle {
                id:                     failsafeTriggerSettings
                anchors.topMargin:      _margins / 2
                anchors.rightMargin:    _margins
                anchors.left:           parent.left
                anchors.top:            failsafeTriggersLabel.bottom
                width:                  throttlePWMField.x + throttlePWMField.width + _margins
                height:                 gcsCheckbox.y + gcsCheckbox.height + _margins
                color:                  palette.windowShade

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

            QGCLabel {
                id:                 rtlLabel
                anchors.leftMargin: _margins
                anchors.left:        failsafeTriggerSettings.right
                text:               qsTr("Return to Launch")
                font.weight:        Font.DemiBold
            }

            Rectangle {
                id:                 rtlSettings
                anchors.topMargin:  _margins / 2
                anchors.left:       rtlLabel.left
                anchors.top:        rtlLabel.bottom
                anchors.bottom:     failsafeTriggerSettings.bottom
                width:              rltAltField.x + rltAltField.width + _margins
                color:              palette.windowShade

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
        } // QGCFlickable
    } // QGCViewPanel
} // QGCView
