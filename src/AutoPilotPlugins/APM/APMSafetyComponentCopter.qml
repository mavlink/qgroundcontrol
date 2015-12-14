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

    property Fact _landSpeedFact:   controller.getParameterFact(-1, "LAND_SPEED")
    property Fact _rtlAltFact:      controller.getParameterFact(-1, "RTL_ALT")
    property Fact _rtlLoitTimeFact: controller.getParameterFact(-1, "RTL_LOIT_TIME")
    property Fact _rtlAltFinalFact: controller.getParameterFact(-1, "RTL_ALT_FINAL")

    property real _margins: ScreenTools.defaultFontPixelHeight

    ExclusiveGroup { id: landLoiterRadioGroup }
    ExclusiveGroup { id: returnAltRadioGroup }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        Flickable {
            clip:               true
            anchors.fill:       parent
            boundsBehavior:     Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick

            QGCLabel {
                id:         rtlLabel
                text:       "Return Home Settings"
                font.weight: Font.DemiBold
            }

            Rectangle {
                anchors.topMargin:  _margins / 2
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.top:        rtlLabel.bottom
                height:             rltAltFinalField.y + rltAltFinalField.height + _margins
                color:              palette.windowShade

                Image {
                    id:                 icon
                    anchors.margins:    _margins
                    anchors.left:       parent.left
                    anchors.top:        parent.top
                    height:             ScreenTools.defaultFontPixelWidth * 20
                    width:              ScreenTools.defaultFontPixelWidth * 20
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
                    text:               "Return at current altitude"
                    checked:            _rtlAltFact.value == 0
                    exclusiveGroup:     returnAltRadioGroup

                    onClicked: _rtlAltFact.value = 0
                }

                QGCRadioButton {
                    id:                 returnAltRadio
                    anchors.topMargin:  _margins
                    anchors.left:       returnAtCurrentRadio.left
                    anchors.top:        returnAtCurrentRadio.bottom
                    text:               "Return at specified altitude:"
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
                    text:               "Loiter above Home for:"

                    onClicked: _rtlLoitTimeFact.value = checked ? 60 : 0
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
                    text:               "Land with descent speed:"
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
                    text:               "Final loiter altitude:"
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
        } // Flickable
    } // QGCViewPanel
} // QGCView
