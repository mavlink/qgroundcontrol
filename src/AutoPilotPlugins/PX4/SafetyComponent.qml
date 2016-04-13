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

import QtQuick 2.5
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Layouts 1.2
import QtGraphicalEffects 1.0

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0
import QGroundControl.ScreenTools 1.0


QGCView {
    id:             _safetyView
    viewPanel:      panel
    anchors.fill:   parent

    FactPanelController { id: controller; factPanel: panel }

    QGCPalette { id: palette; colorGroupEnabled: enabled }

    property real _margins: ScreenTools.defaultFontPixelHeight

    property Fact _fenceAction:     controller.getParameterFact(-1, "GF_ACTION")
    property Fact _fenceRadius:     controller.getParameterFact(-1, "GF_MAX_HOR_DIST")
    property Fact _fenceAlt:        controller.getParameterFact(-1, "GF_MAX_VER_DIST")
    property Fact _rtlLandDelay:    controller.getParameterFact(-1, "RTL_LAND_DELAY")

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        QGCFlickable {
            clip:               true
            anchors.fill:       parent
            contentHeight:      screenBottom.y + screenBottom.height
            contentWidth:       parent.width
            flickableDirection: Flickable.VerticalFlick

            QGCLabel {
                id:             triggerLabel
                text:           qsTr("Triggers For Return Home")
                font.weight:    Font.DemiBold
            }

            Rectangle {
                id:                     triggerSettings
                anchors.topMargin:      _margins / 2
                anchors.rightMargin:    _margins
                anchors.left:           parent.left
                anchors.top:            triggerLabel.bottom
                anchors.bottom:         geoFenceSettings.bottom
                width:                  telemetryLossField.x + telemetryLossField.width + (_margins * 2)
                color:                  palette.windowShade

                QGCLabel {
                    text:               qsTr("RC Transmitter Signal Loss: Return Home after")
                    anchors.margins:    _margins
                    anchors.left:       parent.left
                    anchors.baseline:   rcLossField.baseline
                }

                FactTextField {
                    id:                 rcLossField
                    anchors.topMargin:  _margins
                    anchors.top:        parent.top
                    anchors.left:       telemetryLossField.left
                    fact:               controller.getParameterFact(-1, "COM_RC_LOSS_T")
                    showUnits:          true
                }

                FactCheckBox {
                    id:                 telemetryTimeoutCheckbox
                    anchors.margins:    _margins
                    anchors.left:       parent.left
                    anchors.baseline:   telemetryLossField.baseline
                    fact:               controller.getParameterFact(-1, "COM_DL_LOSS_EN")
                    checkedValue:       1
                    uncheckedValue:     0
                    text:               qsTr("Telemetry Signal Timeout: Return Home after")
                }

                FactTextField {
                    id:                 telemetryLossField
                    anchors.leftMargin: _margins
                    anchors.topMargin:  _margins / 2
                    anchors.left:       telemetryTimeoutCheckbox.right
                    anchors.top:        rcLossField.bottom
                    fact:               controller.getParameterFact(-1, "COM_DL_LOSS_T")
                    showUnits:          true
                    enabled:            telemetryTimeoutCheckbox.checked
                }
            } // Rectangle - Trigger settings

            QGCLabel {
                id:                 geoFenceLabel
                anchors.leftMargin: _margins
                anchors.left:       triggerSettings.right
                anchors.top:        parent.top
                text:               qsTr("GeoFence")
                font.weight:        Font.DemiBold
            }

            Rectangle {
                id:                 geoFenceSettings
                anchors.topMargin:  _margins / 2
                anchors.left:       geoFenceLabel.left
                anchors.top:        geoFenceLabel.bottom
                width:              fenceActionCombo.x + fenceActionCombo.width + _margins
                height:             fenceAltMaxField.y + fenceAltMaxField.height + _margins
                color:              palette.windowShade

                QGCLabel {
                    id:                 fenceActionLabel
                    anchors.margins:    _margins
                    anchors.left:       parent.left
                    anchors.baseline:   fenceActionCombo.baseline
                    text:               qsTr("Action on breach:")
                }

                FactComboBox {
                    id:                 fenceActionCombo
                    anchors.margins:    _margins
                    anchors.left:       fenceActionLabel.right
                    anchors.top:        parent.top
                    width:              fenceAltMaxField.width
                    model:              [ qsTr("None"), qsTr("Warning"), qsTr("Loiter"), qsTr("Return Home"), qsTr("Flight termination") ]
                    fact:               _fenceAction
                }

                QGCCheckBox {
                    id:                 fenceRadiusCheckBox
                    anchors.left:       fenceActionLabel.left
                    anchors.baseline:   fenceRadiusField.baseline
                    text:               qsTr("Max radius:")
                    checked:            _fenceRadius.value >= 0

                    onClicked: _fenceRadius.value = checked ? 100 : -1
                }

                FactTextField {
                    id:                 fenceRadiusField
                    anchors.topMargin:  _margins
                    anchors.left:       fenceActionCombo.left
                    anchors.top:        fenceActionCombo.bottom
                    showUnits:          true
                    fact:               _fenceRadius
                    enabled:            fenceRadiusCheckBox.checked
                }

                QGCCheckBox {
                    id:                 fenceAltMaxCheckBox
                    anchors.left:       fenceActionLabel.left
                    anchors.baseline:   fenceAltMaxField.baseline
                    text:               qsTr("Max altitude:")
                    checked:            _fenceAlt.value >= 0

                    onClicked: _fenceAlt.value = checked ? 100 : -1
                }

                FactTextField {
                    id:                 fenceAltMaxField
                    anchors.topMargin:  _margins / 2
                    anchors.left:       fenceActionCombo.left
                    anchors.top:        fenceRadiusField.bottom
                    showUnits:          true
                    fact:               _fenceAlt
                    enabled:            fenceAltMaxCheckBox.checked
                }
            } // Rectangle - GeoFence Settings

            QGCLabel {
                id:                 rtlLabel
                anchors.topMargin:  _margins
                anchors.top:        triggerSettings.bottom
                text:               qsTr("Return Home Settings")
                font.weight:        Font.DemiBold
            }

            Rectangle {
                id:                 rtlSettings
                anchors.topMargin:  _margins / 2
                anchors.left:       parent.left
                anchors.top:        rtlLabel.bottom
                width:              landDelayField.x + landDelayField.width + _margins
                height:             descendField.y + descendField.height + _margins
                color:              palette.windowShade

                Image {
                    id:                 icon
                    anchors.margins:    _margins
                    anchors.left:       parent.left
                    anchors.top:        parent.top
                    height:             ScreenTools.defaultFontPixelWidth * 10
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

                QGCLabel {
                    id:                 climbLabel
                    anchors.margins:    _margins
                    anchors.left:       icon.right
                    anchors.baseline:   climbField.baseline
                    text:               qsTr("Climb to altitude of")
                }

                FactTextField {
                    id:                 climbField
                    anchors.topMargin:  _margins
                    anchors.top:        parent.top
                    anchors.left:       landDelayField.left
                    fact:               controller.getParameterFact(-1, "RTL_RETURN_ALT")
                    showUnits:          true
                }

                QGCLabel {
                    id:                 returnHomeLabel
                    anchors.topMargin:  _margins
                    anchors.top:        climbField.bottom
                    anchors.left:       climbLabel.left
                    text:               "Return Home, then:"
                }

                ExclusiveGroup { id: homeLoiterGroup }

                QGCRadioButton {
                    id:                 homeLoiterNoLandRadio
                    anchors.topMargin:  _margins
                    anchors.top:        returnHomeLabel.bottom
                    anchors.left:       climbLabel.left
                    checked:            _rtlLandDelay.value < 0
                    exclusiveGroup:     homeLoiterGroup
                    text:               "Loiter at Home altitude, do not land"

                    onClicked: _rtlLandDelay.value = -1
                }

                QGCRadioButton {
                    id:                 homeLoiterLandRadio
                    anchors.baseline:   landDelayField.baseline
                    anchors.left:       climbLabel.left
                    checked:            _rtlLandDelay.value >= 0
                    exclusiveGroup:     homeLoiterGroup
                    text:               qsTr("Loiter at Home altitude for")
                    onClicked: _rtlLandDelay.value = 60
                }

                FactTextField {
                    id:                 landDelayField
                    anchors.margins:    _margins
                    anchors.left:       homeLoiterLandRadio.right
                    anchors.top:        homeLoiterNoLandRadio.bottom
                    fact:               controller.getParameterFact(-1, "RTL_LAND_DELAY")
                    showUnits:          true
                    enabled:            homeLoiterLandRadio.checked === true
                }

                QGCLabel {
                    text:               qsTr("Home loiter altitude")
                    anchors.baseline:   descendField.baseline
                    anchors.left:       climbLabel.left
                    color:              palette.text
                    enabled:            homeLoiterLandRadio.checked === true
                }

                FactTextField {
                    id:                 descendField
                    anchors.topMargin:  _margins
                    anchors.left:       landDelayField.left
                    anchors.top:        landDelayField.bottom
                    fact:               controller.getParameterFact(-1, "RTL_DESCEND_ALT")
                    enabled:            homeLoiterLandRadio.checked === true
                    showUnits:          true
                }
            }

            QGCLabel {
                id:                 navRclObc
                anchors.topMargin:  _margins
                anchors.top:        rtlSettings.bottom
                anchors.left:       parent.left
                anchors.right:      parent.right
                font.pixelSize:     ScreenTools.mediumFontPixelSize
                text:               qsTr("Warning: You have an advanced safety configuration set using the NAV_RCL_OBC parameter. The above settings may not apply.")
                visible:            fact.value !== 0
                wrapMode:           Text.Wrap

                property Fact fact: controller.getParameterFact(-1, "NAV_RCL_OBC")
            }

            QGCLabel {
                id:                 navDllObc
                anchors.topMargin:  _margins / 2
                anchors.top:        navRclObc.bottom
                anchors.left:       parent.left
                anchors.right:      parent.right
                font.pixelSize:     ScreenTools.mediumFontPixelSize
                text:               qsTr("Warning: You have an advanced safety configuration set using the NAV_DLL_OBC parameter. The above settings may not apply.")
                visible:            fact.value !== 0
                wrapMode:           Text.Wrap

                property Fact fact: controller.getParameterFact(-1, "NAV_DLL_OBC")
            }

            Item {
                id:             screenBottom
                anchors.top:    navDllObc.bottom
                width:          1
                height:         1
            }
        } // QGCFlickable
    } // QGCViewPanel
} // QGCView
