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
    property Fact _lowBattAction:   controller.getParameterFact(-1, "COM_LOW_BAT_ACT")
    property Fact _rcLossAction:    controller.getParameterFact(-1, "NAV_RCL_ACT")

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        QGCFlickable {
            clip:               true
            anchors.fill:       parent
            contentHeight:      mainCol.height
            contentWidth:       parent.width
            flickableDirection: Flickable.VerticalFlick

            Column {
                id:             mainCol
                spacing:        _margins

                QGCLabel {
                    id:                 rtlLabel
                    text:               qsTr("Return Home Settings")
                    font.weight:        Font.DemiBold
                }

                Rectangle {
                    id:                 rtlSettings
                    color:              palette.windowShade
                    width:              rtlRow.width  + _margins * 2
                    height:             rtlRow.height + _margins * 2
                    Row {
                        id:                     rtlRow
                        spacing:                _margins
                        anchors.verticalCenter: parent.verticalCenter
                        Item {
                            width:              _margins * 0.5
                            height:             1
                        }
                        QGCColoredImage {
                            id:                 icon
                            height:             ScreenTools.defaultFontPixelWidth * 10
                            width:              ScreenTools.defaultFontPixelWidth * 20
                            mipmap:             true
                            fillMode:           Image.PreserveAspectFit
                            source:             controller.fixedWing ? "/qmlimages/ReturnToHomeAltitude.svg" : "/qmlimages/ReturnToHomeAltitudeCopter.svg"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Item {
                            width:              _margins * 0.5
                            height:             1
                        }
                        Column {
                            spacing:            _margins * 0.5
                            Row {
                                QGCLabel {
                                    id:                 climbLabel
                                    anchors.baseline:   climbField.baseline
                                    width:              ScreenTools.defaultFontPixelWidth * 22
                                    text:               qsTr("Climb to altitude of:")
                                }
                                FactTextField {
                                    id:                 climbField
                                    fact:               controller.getParameterFact(-1, "RTL_RETURN_ALT")
                                    showUnits:          true
                                }
                            }
                            QGCLabel {
                                id:                 returnHomeLabel
                                text:               "Return home, then:"
                            }
                            Row {
                                Item {
                                    height:     1
                                    width:      _margins
                                }
                                Column {
                                    spacing:            _margins * 0.5
                                    ExclusiveGroup { id: homeLoiterGroup }
                                    QGCRadioButton {
                                        id:                 homeLandRadio
                                        checked:            _rtlLandDelay.value < 0
                                        exclusiveGroup:     homeLoiterGroup
                                        text:               "Land immediately"
                                        onClicked:          _rtlLandDelay.value = 0
                                    }
                                    QGCRadioButton {
                                        id:                 homeLoiterNoLandRadio
                                        checked:            _rtlLandDelay.value < 0
                                        exclusiveGroup:     homeLoiterGroup
                                        text:               "Loiter, do not land"
                                        onClicked:          _rtlLandDelay.value = -1
                                    }
                                    QGCRadioButton {
                                        id:                 homeLoiterLandRadio
                                        checked:            _rtlLandDelay.value >= 0
                                        exclusiveGroup:     homeLoiterGroup
                                        text:               qsTr("Loiter and land after specified time")
                                        onClicked:          _rtlLandDelay.value = 60
                                    }
                                }
                            }
                            Item {
                                width:  1
                                height: _margins
                            }
                            Row {
                                QGCLabel {
                                    text:               qsTr("Loiter Time")
                                    width:              ScreenTools.defaultFontPixelWidth * 22
                                    anchors.baseline:   landDelayField.baseline
                                    color:              palette.text
                                    enabled:            homeLoiterLandRadio.checked === true
                                }
                                FactTextField {
                                    id:                 landDelayField
                                    fact:               controller.getParameterFact(-1, "RTL_LAND_DELAY")
                                    showUnits:          true
                                    enabled:            homeLoiterLandRadio.checked === true
                                }
                            }
                            Row {
                                QGCLabel {
                                    text:               qsTr("Loiter Altitude")
                                    width:              ScreenTools.defaultFontPixelWidth * 22
                                    anchors.baseline:   descendField.baseline
                                    color:              palette.text
                                    enabled:            homeLoiterLandRadio.checked === true
                                }
                                FactTextField {
                                    id:                 descendField
                                    fact:               controller.getParameterFact(-1, "RTL_DESCEND_ALT")
                                    enabled:            homeLoiterLandRadio.checked === true
                                    showUnits:          true
                                }
                            }
                        }
                    }
                }

                QGCLabel {
                    text:               qsTr("Land Mode")
                    font.weight:        Font.DemiBold
                }

                Rectangle {
                    color:              palette.windowShade
                    width:              rtlSettings.width
                    height:             landModeRow.height + _margins * 2
                    Row {
                        id:                     landModeRow
                        spacing:                _margins
                        anchors.verticalCenter: parent.verticalCenter
                        Item {
                            width:              _margins * 0.5
                            height:             1
                        }
                        QGCColoredImage {
                            height:             ScreenTools.defaultFontPixelWidth * 10
                            width:              ScreenTools.defaultFontPixelWidth * 20
                            mipmap:             true
                            fillMode:           Image.PreserveAspectFit
                            source:             controller.fixedWing ? "/qmlimages/LandMode.svg" : "/qmlimages/LandModeCopter.svg"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Item {
                            width:              _margins * 0.5
                            height:             1
                        }
                        Column {
                            spacing:                    _margins * 0.5
                            anchors.verticalCenter:     parent.verticalCenter
                            Row {
                                visible:                !controller.fixedWing
                                QGCLabel {
                                    anchors.baseline:   landVelField.baseline
                                    width:              ScreenTools.defaultFontPixelWidth * 22
                                    text:               qsTr("Land Velocity:")
                                }
                                FactTextField {
                                    id:                 landVelField
                                    fact:               controller.getParameterFact(-1, "MPC_LAND_SPEED")
                                    showUnits:          true
                                }
                            }
                            Row {
                                QGCLabel {
                                    anchors.baseline:   disarmField.baseline
                                    width:              ScreenTools.defaultFontPixelWidth * 22
                                    text:               qsTr("Disarm After:")
                                }
                                FactTextField {
                                    id:                 disarmField
                                    fact:               controller.getParameterFact(-1, "COM_DISARM_LAND")
                                    showUnits:          true
                                }
                            }
                        }
                    }
                }

                QGCLabel {
                    text:               qsTr("Low Battery Trigger")
                    font.weight:        Font.DemiBold
                }

                Rectangle {
                    color:              palette.windowShade
                    width:              rtlSettings.width
                    height:             lowBattRow.height + _margins * 2
                    Row {
                        id:                     lowBattRow
                        spacing:                _margins
                        anchors.verticalCenter: parent.verticalCenter
                        Item {
                            width:              _margins * 0.5
                            height:             1
                        }
                        QGCColoredImage {
                            height:             ScreenTools.defaultFontPixelWidth * 10
                            width:              ScreenTools.defaultFontPixelWidth * 20
                            mipmap:             true
                            fillMode:           Image.PreserveAspectFit
                            source:             "/qmlimages/LowBattery.svg"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Item {
                            width:              _margins * 0.5
                            height:             1
                        }
                        Column {
                            spacing:                    _margins * 0.5
                            anchors.verticalCenter:     parent.verticalCenter
                            Row {
                                visible:                !controller.fixedWing
                                QGCLabel {
                                    anchors.baseline:   lowBattCombo.baseline
                                    width:              ScreenTools.defaultFontPixelWidth * 22
                                    text:               qsTr("Action:")
                                }
                                FactComboBox {
                                    id:                 lowBattCombo
                                    width:              climbField.width
                                    model:              [ qsTr("No Action"), qsTr("Return To Land") ]
                                    fact:               _lowBattAction
                                }
                            }
                            Row {
                                QGCLabel {
                                    anchors.baseline:   batLowLevelField.baseline
                                    width:              ScreenTools.defaultFontPixelWidth * 22
                                    text:               qsTr("Battery Low Level:")
                                }
                                FactTextField {
                                    id:                 batLowLevelField
                                    fact:               controller.getParameterFact(-1, "COM_DISARM_LAND")
                                    showUnits:          true
                                }
                            }
                        }
                    }
                }











                Rectangle {
                    color:              palette.windowShade
                    width:              rtlSettings.width
                    height:             _triggerCol.height + _margins * 2
                    Column {
                        id:                     _triggerCol
                        spacing:                _margins
                        anchors.verticalCenter: parent.verticalCenter
                        Row {
                            id:                     rcLossRow
                            spacing:                _margins
                            Item {
                                width:              _margins * 0.5
                                height:             1
                            }
                            QGCLabel {
                                text:               qsTr("RC Loss")
                                font.weight:        Font.DemiBold
                                width:              ScreenTools.defaultFontPixelWidth * 20
                            }
                            Item {
                                width:              _margins * 0.5
                                height:             1
                            }
                            Column {
                                spacing:                    _margins * 0.5
                                Row {
                                    QGCLabel {
                                        anchors.baseline:   rcLossCombo.baseline
                                        width:              ScreenTools.defaultFontPixelWidth * 22
                                        text:               qsTr("Action:")
                                    }
                                    FactComboBox {
                                        id:                 rcLossCombo
                                        width:              climbField.width
                                        model:              [ qsTr("Loiter"), qsTr("Return To Land"), qsTr("Land at current position") ]
                                        fact:               _rcLossAction
                                    }
                                }
                                Row {
                                    QGCLabel {
                                        anchors.baseline:   rcLossField.baseline
                                        width:              ScreenTools.defaultFontPixelWidth * 22
                                        text:               qsTr("RC Loss Timeout:")
                                    }
                                    FactTextField {
                                        id:                 rcLossField
                                        fact:               controller.getParameterFact(-1, "COM_RC_LOSS_T")
                                        showUnits:          true
                                    }
                                }
                                /*
                                    This is defined in the parameter specification but it is not clear how it is used.
                                    The actions above (RTL, Loiter, and Land At Current Position) makes its meaning
                                    ambiguous. Loiter before RTL and/or Land At Current Position? What if the action
                                    is set to "Loiter"? What happens after the timeout?
                                Row {
                                    QGCLabel {
                                        anchors.baseline:   rcLossLoiterField.baseline
                                        width:              ScreenTools.defaultFontPixelWidth * 22
                                        text:               qsTr("RC Loss Loiter Period:")
                                    }
                                    FactTextField {
                                        id:                 rcLossLoiterField
                                        fact:               controller.getParameterFact(-1, "NAV_RCL_LT")
                                        showUnits:          true
                                    }
                                }
                                */
                            }
                        }
                        Item {
                            width:  1
                            height: _margins
                        }
                        Row {
                            id:                     geofenceRow
                            spacing:                _margins
                            Item {
                                width:              _margins * 0.5
                                height:             1
                            }
                            QGCLabel {
                                text:               qsTr("GeoFence")
                                font.weight:        Font.DemiBold
                                width:              ScreenTools.defaultFontPixelWidth * 20
                            }
                            Item {
                                width:              _margins * 0.5
                                height:             1
                            }
                            Column {
                                spacing:                    _margins * 0.5
                                Row {
                                    QGCLabel {
                                        id:                 fenceActionLabel
                                        anchors.baseline:   fenceActionCombo.baseline
                                        text:               qsTr("Action on breach:")
                                        width:              ScreenTools.defaultFontPixelWidth * 22
                                    }
                                    FactComboBox {
                                        id:                 fenceActionCombo
                                        width:              fenceAltMaxField.width
                                        model:              [ qsTr("None"), qsTr("Warning"), qsTr("Loiter"), qsTr("Return Home"), qsTr("Flight termination") ]
                                        fact:               _fenceAction
                                    }
                                }
                                Row {
                                    QGCCheckBox {
                                        id:                 fenceRadiusCheckBox
                                        anchors.baseline:   fenceRadiusField.baseline
                                        text:               qsTr("Max radius:")
                                        checked:            _fenceRadius.value >= 0
                                        onClicked:          _fenceRadius.value = checked ? 100 : -1
                                        width:              ScreenTools.defaultFontPixelWidth * 22
                                    }
                                    FactTextField {
                                        id:                 fenceRadiusField
                                        showUnits:          true
                                        fact:               _fenceRadius
                                        enabled:            fenceRadiusCheckBox.checked
                                    }
                                }
                                Row {
                                    QGCCheckBox {
                                        id:                 fenceAltMaxCheckBox
                                        anchors.baseline:   fenceAltMaxField.baseline
                                        text:               qsTr("Max altitude:")
                                        checked:            _fenceAlt.value >= 0
                                        onClicked:          _fenceAlt.value = checked ? 100 : -1
                                        width:              ScreenTools.defaultFontPixelWidth * 22
                                    }
                                    FactTextField {
                                        id:                 fenceAltMaxField
                                        showUnits:          true
                                        fact:               _fenceAlt
                                        enabled:            fenceAltMaxCheckBox.checked
                                    }
                                }
                            }
                        }
                    }
                }
            }

            /*

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
*/


        } // QGCFlickable
    } // QGCViewPanel
} // QGCView

