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

    property real _margins:         ScreenTools.defaultFontPixelHeight
    property real _middleRowWidth:  ScreenTools.defaultFontPixelWidth * 22
    property real _editFieldWidth:  ScreenTools.defaultFontPixelWidth * 18

    property Fact _fenceAction:     controller.getParameterFact(-1, "GF_ACTION")
    property Fact _fenceRadius:     controller.getParameterFact(-1, "GF_MAX_HOR_DIST")
    property Fact _fenceAlt:        controller.getParameterFact(-1, "GF_MAX_VER_DIST")
    property Fact _rtlLandDelay:    controller.getParameterFact(-1, "RTL_LAND_DELAY")
    property Fact _lowBattAction:   controller.getParameterFact(-1, "COM_LOW_BAT_ACT")
    property Fact _rcLossAction:    controller.getParameterFact(-1, "NAV_RCL_ACT")
    property Fact _dlLossAction:    controller.getParameterFact(-1, "NAV_DLL_ACT")

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent
        QGCFlickable {
            clip:                                       true
            anchors.top:                                parent.top
            anchors.bottom:                             parent.bottom
            anchors.horizontalCenter:                   parent.horizontalCenter
            width:                                      mainCol.width
            contentHeight:                              mainCol.height
            contentWidth:                               mainCol.width
            flickableDirection:                         Flickable.VerticalFlick
            Column {
                id:                                     mainCol
                spacing:                                _margins
                /*
                   **** Low Battery ****
                */
                Item { width: 1; height: _margins * 0.5; }
                QGCLabel {
                    text:                               qsTr("Low Battery Trigger")
                    font.weight:                        Font.DemiBold
                }
                Rectangle {
                    color:                              palette.windowShade
                    width:                              rtlSettings.width
                    height:                             lowBattRow.height + _margins * 2
                    Row {
                        id:                             lowBattRow
                        spacing:                        _margins
                        anchors.verticalCenter:         parent.verticalCenter
                        Item { width: _margins * 0.5; height: 1; }
                        Image {
                            height:                     ScreenTools.defaultFontPixelWidth * 6
                            width:                      ScreenTools.defaultFontPixelWidth * 20
                            mipmap:                     true
                            fillMode:                   Image.PreserveAspectFit
                            source:                     qgcPal.globalTheme === QGCPalette.Light ? "/qmlimages/LowBatteryLight.svg" : "/qmlimages/LowBattery.svg"
                            anchors.verticalCenter:     parent.verticalCenter
                        }
                        Item { width: _margins * 0.5; height: 1; }
                        Column {
                            spacing:                    _margins * 0.5
                            anchors.verticalCenter:     parent.verticalCenter
                            Row {
                                visible:                !controller.fixedWing
                                QGCLabel {
                                    anchors.baseline:   lowBattCombo.baseline
                                    width:              _middleRowWidth
                                    text:               qsTr("Action:")
                                }
                                FactComboBox {
                                    id:                 lowBattCombo
                                    width:              _editFieldWidth
                                    fact:               _lowBattAction
                                    indexModel:         false
                                }
                            }
                            Row {
                                QGCLabel {
                                    anchors.baseline:   batLowLevelField.baseline
                                    width:              _middleRowWidth
                                    text:               qsTr("Battery Warn Level:")
                                }
                                FactTextField {
                                    id:                 batLowLevelField
                                    fact:               controller.getParameterFact(-1, "BAT_LOW_THR")
                                    showUnits:          true
                                    width:              _editFieldWidth
                                }
                            }
                            Row {
                                QGCLabel {
                                    anchors.baseline:   batLowLevelField.baseline
                                    width:              _middleRowWidth
                                    text:               qsTr("Battery Failsafe Level:")
                                }
                                FactTextField {
                                    id:                 batCritLevelField
                                    fact:               controller.getParameterFact(-1, "BAT_CRIT_THR")
                                    showUnits:          true
                                    width:              _editFieldWidth
                                }
                            }
                        }
                    }
                }
                /*
                   **** RC Loss ****
                */
                QGCLabel {
                    text:                               qsTr("RC Loss Trigger")
                    font.weight:                        Font.DemiBold
                }
                Rectangle {
                    color:                              palette.windowShade
                    width:                              rtlSettings.width
                    height:                             rcLossRow.height + _margins * 2
                    Row {
                        id:                             rcLossRow
                        spacing:                        _margins
                        anchors.verticalCenter:         parent.verticalCenter
                        Item { width: _margins * 0.5; height: 1; }
                        Image {
                            height:                     ScreenTools.defaultFontPixelWidth * 6
                            width:                      ScreenTools.defaultFontPixelWidth * 20
                            mipmap:                     true
                            fillMode:                   Image.PreserveAspectFit
                            source:                     qgcPal.globalTheme === QGCPalette.Light ? "/qmlimages/RCLossLight.svg" : "/qmlimages/RCLoss.svg"
                            anchors.verticalCenter:     parent.verticalCenter
                        }
                        Item { width: _margins * 0.5; height: 1; }
                        Column {
                            spacing:                    _margins * 0.5
                            anchors.verticalCenter:     parent.verticalCenter
                            Row {
                                QGCLabel {
                                    anchors.baseline:   rcLossCombo.baseline
                                    width:              _middleRowWidth
                                    text:               qsTr("Action:")
                                }
                                FactComboBox {
                                    id:                 rcLossCombo
                                    width:              _editFieldWidth
                                    fact:               _rcLossAction
                                    indexModel:         false
                                }
                            }
                            Row {
                                QGCLabel {
                                    anchors.baseline:   rcLossField.baseline
                                    width:              _middleRowWidth
                                    text:               qsTr("RC Loss Timeout:")
                                }
                                FactTextField {
                                    id:                 rcLossField
                                    fact:               controller.getParameterFact(-1, "COM_RC_LOSS_T")
                                    showUnits:          true
                                    width:              _editFieldWidth
                                }
                            }
                        }
                    }
                }
                /*
                   **** Data Link Loss ****
                */
                QGCLabel {
                    text:                               qsTr("Data Link Loss Trigger")
                    font.weight:                        Font.DemiBold
                }
                Rectangle {
                    color:                              palette.windowShade
                    width:                              rtlSettings.width
                    height:                             dlLossRow.height + _margins * 2
                    Row {
                        id:                             dlLossRow
                        spacing:                        _margins
                        anchors.verticalCenter:         parent.verticalCenter
                        Item { width: _margins * 0.5; height: 1; }
                        Image {
                            height:                     ScreenTools.defaultFontPixelWidth * 6
                            width:                      ScreenTools.defaultFontPixelWidth * 20
                            mipmap:                     true
                            fillMode:                   Image.PreserveAspectFit
                            source:                     qgcPal.globalTheme === QGCPalette.Light ? "/qmlimages/DatalinkLossLight.svg" : "/qmlimages/DatalinkLoss.svg"
                            anchors.verticalCenter:     parent.verticalCenter
                        }
                        Item { width: _margins * 0.5; height: 1; }
                        Column {
                            spacing:                    _margins * 0.5
                            anchors.verticalCenter:     parent.verticalCenter
                            Row {
                                QGCLabel {
                                    anchors.baseline:   dlLossCombo.baseline
                                    width:              _middleRowWidth
                                    text:               qsTr("Action:")
                                }
                                FactComboBox {
                                    id:                 dlLossCombo
                                    width:              _editFieldWidth
                                    fact:               _dlLossAction
                                    indexModel:         false
                                }
                            }
                            Row {
                                QGCLabel {
                                    anchors.baseline:   dlLossField.baseline
                                    width:              _middleRowWidth
                                    text:               qsTr("Data Link Loss Timeout:")
                                }
                                FactTextField {
                                    id:                 dlLossField
                                    fact:               controller.getParameterFact(-1, "COM_DL_LOSS_T")
                                    showUnits:          true
                                    width:              _editFieldWidth
                                }
                            }
                        }
                    }
                }
                /*
                   **** Geofence ****
                */
                QGCLabel {
                    text:                               qsTr("Geofence Trigger")
                    font.weight:                        Font.DemiBold
                }
                Rectangle {
                    color:                              palette.windowShade
                    width:                              rtlSettings.width
                    height:                             geofenceRow.height + _margins * 2
                    Row {
                        id:                             geofenceRow
                        spacing:                        _margins
                        anchors.verticalCenter:         parent.verticalCenter
                        Item { width: _margins * 0.5; height: 1; }
                        Image {
                            height:                     ScreenTools.defaultFontPixelWidth * 8
                            width:                      ScreenTools.defaultFontPixelWidth * 20
                            mipmap:                     true
                            fillMode:                   Image.PreserveAspectFit
                            source:                     qgcPal.globalTheme === QGCPalette.Light ? "/qmlimages/GeoFenceLight.svg" : "/qmlimages/GeoFence.svg"
                            anchors.verticalCenter:     parent.verticalCenter
                        }
                        Item { width: _margins * 0.5; height: 1; }
                        Column {
                            spacing:                    _margins * 0.5
                            anchors.verticalCenter:     parent.verticalCenter
                            Row {
                                QGCLabel {
                                    id:                 fenceActionLabel
                                    anchors.baseline:   fenceActionCombo.baseline
                                    text:               qsTr("Action on breach:")
                                    width:              _middleRowWidth
                                }
                                FactComboBox {
                                    id:                 fenceActionCombo
                                    width:              _editFieldWidth
                                    fact:               _fenceAction
                                    indexModel:         false
                                }
                            }
                            Row {
                                QGCCheckBox {
                                    id:                 fenceRadiusCheckBox
                                    anchors.baseline:   fenceRadiusField.baseline
                                    text:               qsTr("Max radius:")
                                    checked:            _fenceRadius.value >= 0
                                    onClicked:          _fenceRadius.value = checked ? 100 : -1
                                    width:              _middleRowWidth
                                }
                                FactTextField {
                                    id:                 fenceRadiusField
                                    showUnits:          true
                                    fact:               _fenceRadius
                                    enabled:            fenceRadiusCheckBox.checked
                                    width:              _editFieldWidth
                                }
                            }
                            Row {
                                QGCCheckBox {
                                    id:                 fenceAltMaxCheckBox
                                    anchors.baseline:   fenceAltMaxField.baseline
                                    text:               qsTr("Max altitude:")
                                    checked:            _fenceAlt.value >= 0
                                    onClicked:          _fenceAlt.value = checked ? 100 : -1
                                    width:              _middleRowWidth
                                }
                                FactTextField {
                                    id:                 fenceAltMaxField
                                    showUnits:          true
                                    fact:               _fenceAlt
                                    enabled:            fenceAltMaxCheckBox.checked
                                    width:              _editFieldWidth
                                }
                            }
                        }
                    }
                }
                /*
                   **** Return Home Settings ****
                */
                QGCLabel {
                    id:                                 rtlLabel
                    text:                               qsTr("Return Home Settings")
                    font.weight:                        Font.DemiBold
                }
                Rectangle {
                    id:                                 rtlSettings
                    color:                              palette.windowShade
                    width:                              rtlRow.width  + _margins * 2
                    height:                             rtlRow.height + _margins * 2
                    Row {
                        id:                             rtlRow
                        spacing:                        _margins
                        anchors.verticalCenter:         parent.verticalCenter
                        Item { width: _margins * 0.5; height: 1; }
                        QGCColoredImage {
                            id:                         icon
                            color:                      palette.text
                            height:                     ScreenTools.defaultFontPixelWidth * 10
                            width:                      ScreenTools.defaultFontPixelWidth * 20
                            mipmap:                     true
                            fillMode:                   Image.PreserveAspectFit
                            source:                     controller.fixedWing ? "/qmlimages/ReturnToHomeAltitude.svg" : "/qmlimages/ReturnToHomeAltitudeCopter.svg"
                            anchors.verticalCenter:     parent.verticalCenter
                        }
                        Item { width: _margins * 0.5; height: 1; }
                        Column {
                            spacing:                    _margins * 0.5
                            Row {
                                QGCLabel {
                                    id:                 climbLabel
                                    anchors.baseline:   climbField.baseline
                                    width:              _middleRowWidth
                                    text:               qsTr("Climb to altitude of:")
                                }
                                FactTextField {
                                    id:                 climbField
                                    fact:               controller.getParameterFact(-1, "RTL_RETURN_ALT")
                                    showUnits:          true
                                    width:              _editFieldWidth
                                }
                            }
                            Item { width: 1; height: _margins * 0.5; }
                            QGCLabel {
                                id:                     returnHomeLabel
                                text:                   "Return home, then:"
                            }
                            Row {
                                Item { height: 1; width: _margins; }
                                Column {
                                    spacing:            _margins * 0.5
                                    ExclusiveGroup { id: homeLoiterGroup }
                                    QGCRadioButton {
                                        id:             homeLandRadio
                                        checked:        _rtlLandDelay.value === 0
                                        exclusiveGroup: homeLoiterGroup
                                        text:           "Land immediately"
                                        onClicked:      _rtlLandDelay.value = 0
                                    }
                                    QGCRadioButton {
                                        id:             homeLoiterNoLandRadio
                                        checked:        _rtlLandDelay.value < 0
                                        exclusiveGroup: homeLoiterGroup
                                        text:           "Loiter and do not land"
                                        onClicked:      _rtlLandDelay.value = -1
                                    }
                                    QGCRadioButton {
                                        id:             homeLoiterLandRadio
                                        checked:        _rtlLandDelay.value > 0
                                        exclusiveGroup: homeLoiterGroup
                                        text:           qsTr("Loiter and land after specified time")
                                        onClicked:      _rtlLandDelay.value = 60
                                    }
                                }
                            }
                            Item { width: 1; height: _margins * 0.5; }
                            Row {
                                QGCLabel {
                                    text:               qsTr("Loiter Time")
                                    width:              _middleRowWidth
                                    anchors.baseline:   landDelayField.baseline
                                    color:              palette.text
                                    enabled:            homeLoiterLandRadio.checked === true
                                }
                                FactTextField {
                                    id:                 landDelayField
                                    fact:               controller.getParameterFact(-1, "RTL_LAND_DELAY")
                                    showUnits:          true
                                    enabled:            homeLoiterLandRadio.checked === true
                                    width:              _editFieldWidth
                                }
                            }
                            Row {
                                QGCLabel {
                                    text:               qsTr("Loiter Altitude")
                                    width:              _middleRowWidth
                                    anchors.baseline:   descendField.baseline
                                    color:              palette.text
                                    enabled:            homeLoiterLandRadio.checked === true || homeLoiterNoLandRadio.checked === true
                                }
                                FactTextField {
                                    id:                 descendField
                                    fact:               controller.getParameterFact(-1, "RTL_DESCEND_ALT")
                                    enabled:            homeLoiterLandRadio.checked === true || homeLoiterNoLandRadio.checked === true
                                    showUnits:          true
                                    width:              _editFieldWidth
                                }
                            }
                        }
                    }
                }
                /*
                   **** Land Mode Settings ****
                */
                QGCLabel {
                    text:                               qsTr("Land Mode Settings")
                    font.weight:                        Font.DemiBold
                }
                Rectangle {
                    color:                              palette.windowShade
                    width:                              rtlSettings.width
                    height:                             landModeRow.height + _margins * 2
                    Row {
                        id:                             landModeRow
                        spacing:                        _margins
                        anchors.verticalCenter:         parent.verticalCenter
                        Item {
                            width:                      _margins * 0.5
                            height:                     1
                        }
                        QGCColoredImage {
                            color:                      palette.text
                            height:                     ScreenTools.defaultFontPixelWidth * 10
                            width:                      ScreenTools.defaultFontPixelWidth * 20
                            mipmap:                     true
                            fillMode:                   Image.PreserveAspectFit
                            source:                     controller.fixedWing ? "/qmlimages/LandMode.svg" : "/qmlimages/LandModeCopter.svg"
                            anchors.verticalCenter:     parent.verticalCenter
                        }
                        Item {
                            width:                      _margins * 0.5
                            height:                     1
                        }
                        Column {
                            spacing:                    _margins * 0.5
                            anchors.verticalCenter:     parent.verticalCenter
                            Row {
                                visible:                !controller.fixedWing
                                QGCLabel {
                                    anchors.baseline:   landVelField.baseline
                                    width:              _middleRowWidth
                                    text:               qsTr("Landing Velocity:")
                                }
                                FactTextField {
                                    id:                 landVelField
                                    fact:               controller.getParameterFact(-1, "MPC_LAND_SPEED")
                                    showUnits:          true
                                    width:              _editFieldWidth
                                }
                            }
                            Row {
                                QGCLabel {
                                    anchors.baseline:   disarmField.baseline
                                    width:              _middleRowWidth
                                    text:               qsTr("Disarm After:")
                                }
                                FactTextField {
                                    id:                 disarmField
                                    fact:               controller.getParameterFact(-1, "COM_DISARM_LAND")
                                    showUnits:          true
                                    width:              _editFieldWidth
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

