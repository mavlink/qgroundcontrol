/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Layouts          1.2
import QtGraphicalEffects       1.0

import QGroundControl               1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

SetupPage {
    id:             safetyPage
    pageComponent:  pageComponent
    Component {
        id: pageComponent

        Item {
            width:      Math.max(availableWidth, outerColumn.width)
            height:     outerColumn.height

            FactPanelController {
                id:         controller
            }

            readonly property string hitlParam: "SYS_HITL"

            property real _margins:         ScreenTools.defaultFontPixelHeight
            property real _labelWidth:      ScreenTools.defaultFontPixelWidth  * 30
            property real _editFieldWidth:  ScreenTools.defaultFontPixelWidth  * 24
            property real _imageHeight:     ScreenTools.defaultFontPixelHeight * 3
            property real _imageWidth:      _imageHeight * 2

            property Fact _enableLogging:       controller.getParameterFact(-1, "SDLOG_MODE")
            property Fact _fenceAction:         controller.getParameterFact(-1, "GF_ACTION")
            property Fact _fenceRadius:         controller.getParameterFact(-1, "GF_MAX_HOR_DIST")
            property Fact _fenceAlt:            controller.getParameterFact(-1, "GF_MAX_VER_DIST")
            property Fact _rtlLandDelay:        controller.getParameterFact(-1, "RTL_LAND_DELAY")
            property Fact _lowBattAction:       controller.getParameterFact(-1, "COM_LOW_BAT_ACT")
            property Fact _rcLossAction:        controller.getParameterFact(-1, "NAV_RCL_ACT")
            property Fact _dlLossAction:        controller.getParameterFact(-1, "NAV_DLL_ACT")
            property Fact _disarmLandDelay:     controller.getParameterFact(-1, "COM_DISARM_LAND")
            property Fact _collisionPrevention: controller.getParameterFact(-1, "CP_DIST")
            property Fact _objectAvoidance:     controller.getParameterFact(-1, "COM_OBS_AVOID")
            property Fact _landSpeedMC:         controller.getParameterFact(-1, "MPC_LAND_SPEED", false)
            property bool _hitlAvailable:       controller.parameterExists(-1, hitlParam)
            property Fact _hitlEnabled:         controller.getParameterFact(-1, hitlParam, false)

            ColumnLayout {
                id:         outerColumn
                spacing:    _margins
                anchors.horizontalCenter:   parent.horizontalCenter

                QGCLabel {
                    text:                   qsTr("Low Battery Failsafe Trigger")
                }

                Rectangle {
                    width:                  mainRow.width  + (_margins * 2)
                    height:                 mainRow.height + (_margins * 2)
                    color:                  qgcPal.windowShade
                    Row {
                        id:                 mainRow
                        spacing:            _margins
                        anchors.centerIn:   parent
                        Item {
                            width:                  _imageWidth
                            height:                 _imageHeight
                            anchors.verticalCenter: parent.verticalCenter
                            Image {
                                mipmap:             true
                                fillMode:           Image.PreserveAspectFit
                                source:             qgcPal.globalTheme === QGCPalette.Light ? "/qmlimages/LowBatteryLight.svg" : "/qmlimages/LowBattery.svg"
                                height:             _imageHeight
                                anchors.centerIn:   parent
                            }
                        }
                        GridLayout {
                            columns:                2
                            anchors.verticalCenter: parent.verticalCenter

                            QGCLabel {
                                text:               qsTr("Failsafe Action:")
                                Layout.minimumWidth:_labelWidth
                                Layout.fillWidth:   true
                            }
                            FactComboBox {
                                fact:               _lowBattAction
                                indexModel:         false
                                Layout.minimumWidth:_editFieldWidth
                                Layout.fillWidth:   true
                            }

                            QGCLabel {
                                text:               qsTr("Battery Warn Level:")
                                Layout.fillWidth:   true
                            }
                            FactTextField {
                                fact:               controller.getParameterFact(-1, "BAT_LOW_THR")
                                Layout.fillWidth:   true
                            }

                            QGCLabel {
                                text:               qsTr("Battery Failsafe Level:")
                                Layout.fillWidth:   true
                            }
                            FactTextField {
                                fact:               controller.getParameterFact(-1, "BAT_CRIT_THR")
                                Layout.fillWidth:   true
                            }

                            QGCLabel {
                                text:               qsTr("Battery Emergency Level:")
                                Layout.fillWidth:   true
                            }
                            FactTextField {
                                fact:               controller.getParameterFact(-1, "BAT_EMERGEN_THR")
                                Layout.fillWidth:   true
                            }
                        }
                    }
                }

                QGCLabel {
                    text:                   qsTr("Object Detection")
                }

                Rectangle {
                    width:                  mainRow.width + (_margins * 2)
                    height:                 odRow.height  + (_margins * 2)
                    color:                  qgcPal.windowShade
                    Row {
                        id:                 odRow
                        spacing:            _margins
                        anchors.centerIn:   parent
                        Item {
                            width:                  _imageWidth
                            height:                 _imageHeight
                            anchors.verticalCenter: parent.verticalCenter
                            QGCColoredImage {
                                color:              qgcPal.text
                                source:             "/qmlimages/ObjectAvoidance.svg"
                                height:             _imageHeight
                                width:              _imageHeight * 2
                                anchors.centerIn:   parent
                            }
                        }
                        GridLayout {
                            columns:                2
                            anchors.verticalCenter: parent.verticalCenter

                            QGCLabel {
                                text:               qsTr("Collision Prevention:")
                                Layout.minimumWidth:_labelWidth
                                Layout.fillWidth:   true
                            }
                            QGCComboBox {
                                model:              [qsTr("Disabled"), qsTr("Enabled")]
                                enabled:            _collisionPrevention
                                Layout.minimumWidth:_editFieldWidth
                                Layout.fillWidth:   true
                                currentIndex:       _collisionPrevention ? (_collisionPrevention.rawValue > 0 ? 1 : 0) : 0
                                onActivated: {
                                    if(_collisionPrevention) {
                                        _collisionPrevention.value = index > 0 ? 5 : -1
                                        console.log('Collision prevention enabled: ' + _collisionPrevention.value)
                                        showObstacleDistanceOverlayCheckBox.checked = _collisionPrevention.value > 0
                                    }
                                }
                            }

                            QGCLabel {
                                text:               qsTr("Obstacle Avoidance:")
                                Layout.fillWidth:   true
                            }
                            QGCComboBox {
                                model:              [qsTr("Disabled"), qsTr("Enabled")]
                                enabled:            _objectAvoidance && _collisionPrevention.rawValue > 0
                                Layout.minimumWidth:_editFieldWidth
                                Layout.fillWidth:   true
                                currentIndex:       _objectAvoidance ? (_objectAvoidance.value === 0 ? 0 : 1) : 0
                                onActivated: {
                                    if(_objectAvoidance) {
                                        _objectAvoidance.value = index > 0 ? 1 : 0
                                    }
                                }
                            }

                            QGCLabel {
                                text:               qsTr("Minimum Distance: (") + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString + ")"
                                Layout.fillWidth:   true
                                Layout.alignment:   Qt.AlignVCenter
                            }
                            QGCSlider {
                                width:              _editFieldWidth
                                enabled:            _collisionPrevention && _collisionPrevention.rawValue > 0
                                Layout.minimumWidth:_editFieldWidth
                                Layout.minimumHeight:   ScreenTools.defaultFontPixelHeight * 2
                                Layout.fillWidth:   true
                                Layout.fillHeight:  true
                                maximumValue:       QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(15)
                                minimumValue:       QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(1)
                                stepSize:           1
                                displayValue:       true
                                updateValueWhileDragging:   false
                                Layout.alignment:   Qt.AlignVCenter
                                value: {
                                    if (_collisionPrevention && _collisionPrevention.rawValue > 0) {
                                        return QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(_collisionPrevention.rawValue)
                                    } else {
                                        return 1;
                                    }
                                }
                                onValueChanged: {
                                    if(_collisionPrevention) {
                                        //-- Negative means disabled
                                        if(_collisionPrevention.rawValue >= 0) {
                                            _collisionPrevention.rawValue = QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsToMeters(value)
                                        }
                                    }
                                }
                            }

                            FactCheckBox {
                                id:         showObstacleDistanceOverlayCheckBox
                                text:       qsTr("Show obstacle distance overlay")
                                visible:    _showObstacleDistanceOverlay.visible
                                fact:       _showObstacleDistanceOverlay

                                property Fact _showObstacleDistanceOverlay: QGroundControl.settingsManager.flyViewSettings.showObstacleDistanceOverlay
                            }
                        }
                    }
                }

                QGCLabel {
                    text:                   qsTr("RC Loss Failsafe Trigger")
                }

                Rectangle {
                    width:                  mainRow.width     + (_margins * 2)
                    height:                 rcLossGrid.height + (_margins * 2)
                    color:                  qgcPal.windowShade
                    Row {
                        id:                 rcLossGrid
                        spacing:            _margins
                        anchors.centerIn:   parent
                        Item {
                            width:                  _imageWidth
                            height:                 _imageHeight
                            anchors.verticalCenter: parent.verticalCenter
                            Image {
                                mipmap:             true
                                fillMode:           Image.PreserveAspectFit
                                source:             qgcPal.globalTheme === QGCPalette.Light ? "/qmlimages/RCLossLight.svg" : "/qmlimages/RCLoss.svg"
                                height:             _imageHeight
                                anchors.centerIn:   parent
                            }
                        }
                        GridLayout {
                            columns:                2
                            anchors.verticalCenter: parent.verticalCenter

                            QGCLabel {
                                text:               qsTr("Failsafe Action:")
                                Layout.minimumWidth:_labelWidth
                                Layout.fillWidth:   true
                            }
                            FactComboBox {
                                fact:               _rcLossAction
                                indexModel:         false
                                Layout.minimumWidth:_editFieldWidth
                                Layout.fillWidth:   true
                            }

                            QGCLabel {
                                text:               qsTr("RC Loss Timeout:")
                                Layout.fillWidth:   true
                            }
                            FactTextField {
                                fact:               controller.getParameterFact(-1, "COM_RC_LOSS_T")
                                Layout.fillWidth:   true
                            }
                        }
                    }
                }

                QGCLabel {
                    text:                   qsTr("Data Link Loss Failsafe Trigger")
                }

                Rectangle {
                    width:                  mainRow.width           + (_margins * 2)
                    height:                 dataLinkLossGrid.height + (_margins * 2)
                    color:                  qgcPal.windowShade
                    Row {
                        id:                 dataLinkLossGrid
                        spacing:            _margins
                        anchors.centerIn:   parent
                        Item {
                            width:                  _imageWidth
                            height:                 _imageHeight
                            anchors.verticalCenter: parent.verticalCenter
                            Image {
                                mipmap:             true
                                fillMode:           Image.PreserveAspectFit
                                source:             qgcPal.globalTheme === QGCPalette.Light ? "/qmlimages/DatalinkLossLight.svg" : "/qmlimages/DatalinkLoss.svg"
                                height:             _imageHeight
                                anchors.centerIn:   parent
                            }
                        }
                        GridLayout {
                            columns:                2
                            anchors.verticalCenter: parent.verticalCenter

                            QGCLabel {
                                text:               qsTr("Failsafe Action:")
                                Layout.minimumWidth:_labelWidth
                                Layout.fillWidth:   true
                            }
                            FactComboBox {
                                fact:               _dlLossAction
                                indexModel:         false
                                Layout.minimumWidth:_editFieldWidth
                                Layout.fillWidth:   true
                            }

                            QGCLabel {
                                text:               qsTr("Data Link Loss Timeout:")
                                Layout.fillWidth:   true
                            }
                            FactTextField {
                                fact:               controller.getParameterFact(-1, "COM_DL_LOSS_T")
                                Layout.fillWidth:   true
                            }
                        }
                    }
                }

                QGCLabel {
                    text:                   qsTr("Geofence Failsafe Trigger")
                }

                Rectangle {
                    width:                  mainRow.width       + (_margins * 2)
                    height:                 geoFenceGrid.height + (_margins * 2)
                    color:                  qgcPal.windowShade
                    Row {
                        id:                 geoFenceGrid
                        spacing:            _margins
                        anchors.centerIn:   parent
                        Item {
                            width:                  _imageWidth
                            height:                 _imageHeight
                            anchors.verticalCenter: parent.verticalCenter
                            Image {
                                mipmap:             true
                                fillMode:           Image.PreserveAspectFit
                                source:             qgcPal.globalTheme === QGCPalette.Light ? "/qmlimages/GeoFenceLight.svg" : "/qmlimages/GeoFence.svg"
                                height:             _imageHeight
                                anchors.centerIn:   parent
                            }
                        }

                        GridLayout {
                            columns:                2
                            anchors.verticalCenter: parent.verticalCenter

                            QGCLabel {
                                text:               qsTr("Action on breach:")
                                Layout.minimumWidth:_labelWidth
                                Layout.fillWidth:   true
                            }
                            FactComboBox {
                                fact:               _fenceAction
                                indexModel:         false
                                Layout.minimumWidth:_editFieldWidth
                                Layout.fillWidth:   true
                            }

                            QGCCheckBox {
                                id:                 fenceRadiusCheckBox
                                text:               qsTr("Max Radius:")
                                checked:            _fenceRadius.value > 0
                                onClicked:          _fenceRadius.value = checked ? 100 : 0
                                Layout.fillWidth:   true
                            }
                            FactTextField {
                                fact:               _fenceRadius
                                enabled:            fenceRadiusCheckBox.checked
                                Layout.fillWidth:   true
                            }

                            QGCCheckBox {
                                id:                 fenceAltMaxCheckBox
                                text:               qsTr("Max Altitude:")
                                checked:            _fenceAlt ? _fenceAlt.value > 0 : false
                                onClicked:          _fenceAlt.value = checked ? 100 : 0
                                Layout.fillWidth:   true
                            }
                            FactTextField {
                                fact:               _fenceAlt
                                enabled:            fenceAltMaxCheckBox.checked
                                Layout.fillWidth:   true
                            }
                        }
                    }
                }

                QGCLabel {
                    text:               qsTr("Return To Launch Settings")
                }

                Rectangle {
                    width:              mainRow.width         + (_margins * 2)
                    height:             returnHomeGrid.height + (_margins * 2)
                    color:              qgcPal.windowShade
                    Row {
                        id:                 returnHomeGrid
                        spacing:            _margins
                        anchors.centerIn:   parent
                        Item {
                            width:                  _imageWidth
                            height:                 _imageHeight
                            anchors.verticalCenter: parent.verticalCenter
                            QGCColoredImage {
                                color:              qgcPal.text
                                source:             controller.vehicle.fixedWing ? "/qmlimages/ReturnToHomeAltitude.svg" : "/qmlimages/ReturnToHomeAltitudeCopter.svg"
                                height:             _imageHeight
                                width:              _imageHeight * 2
                                anchors.centerIn:   parent
                            }
                        }
                        GridLayout {
                            columns:                    2
                            anchors.verticalCenter:     parent.verticalCenter

                            QGCLabel {
                                text:                   qsTr("Climb to altitude of:")
                                Layout.minimumWidth:    _labelWidth
                                Layout.fillWidth:       true
                            }
                            FactTextField {
                                fact:                   controller.getParameterFact(-1, "RTL_RETURN_ALT")
                                Layout.minimumWidth:    _editFieldWidth
                                Layout.fillWidth:       true
                            }

                            QGCLabel {
                                text:                   qsTr("Return to launch, then:")
                                Layout.columnSpan:      2
                            }
                            Row {
                                Layout.columnSpan:      2
                                Item { width: ScreenTools.defaultFontPixelWidth; height: 1 }
                                QGCRadioButton {
                                    id:                 homeLandRadio
                                    checked:            _rtlLandDelay ? _rtlLandDelay.value === 0 : false
                                    text:               qsTr("Land immediately")
                                    onClicked:          _rtlLandDelay.value = 0
                                }
                            }
                            Row {
                                Layout.columnSpan:      2
                                Item { width: ScreenTools.defaultFontPixelWidth; height: 1 }
                                QGCRadioButton {
                                    id:                 homeLoiterNoLandRadio
                                    checked:            _rtlLandDelay ? _rtlLandDelay.value < 0 : false
                                    text:               qsTr("Loiter and do not land")
                                    onClicked:          _rtlLandDelay.value = -1
                                }
                            }
                            Row {
                                Layout.columnSpan:      2
                                Item { width: ScreenTools.defaultFontPixelWidth; height: 1 }
                                QGCRadioButton {
                                    id:                 homeLoiterLandRadio
                                    checked:            _rtlLandDelay ? _rtlLandDelay.value > 0 : false
                                    text:               qsTr("Loiter and land after specified time")
                                    onClicked:          _rtlLandDelay.value = 60
                                }
                            }

                            QGCLabel {
                                text:                   qsTr("Loiter Time")
                                Layout.fillWidth:       true
                            }
                            FactTextField {
                                fact:                   controller.getParameterFact(-1, "RTL_LAND_DELAY")
                                enabled:                homeLoiterLandRadio.checked === true
                                Layout.fillWidth:       true
                            }

                            QGCLabel {
                                text:                   qsTr("Loiter Altitude")
                                Layout.fillWidth:       true
                            }
                            FactTextField {
                                fact:                   controller.getParameterFact(-1, "RTL_DESCEND_ALT")
                                enabled:                homeLoiterLandRadio.checked === true || homeLoiterNoLandRadio.checked === true
                                Layout.fillWidth:       true
                            }
                        }
                    }
                }

                QGCLabel {
                    text:               qsTr("Land Mode Settings")
                }

                Rectangle {
                    width:              mainRow.width       + (_margins * 2)
                    height:             landModeGrid.height + (_margins * 2)
                    color:              qgcPal.windowShade
                    Row {
                        id:                 landModeGrid
                        spacing:            _margins
                        anchors.centerIn:   parent
                        Item {
                            width:                  _imageWidth
                            height:                 _imageHeight
                            anchors.verticalCenter: parent.verticalCenter
                            QGCColoredImage {
                                color:              qgcPal.text
                                source:             controller.vehicle.fixedWing ? "/qmlimages/LandMode.svg" : "/qmlimages/LandModeCopter.svg"
                                height:             _imageHeight
                                width:              _imageHeight
                                anchors.centerIn:   parent
                            }
                        }
                        GridLayout {
                            columns:                2
                            anchors.verticalCenter: parent.verticalCenter

                            QGCLabel {
                                id:                 landVelocityLabel
                                text:               qsTr("Landing Descent Rate:")
                                visible:            controller.vehicle && !controller.vehicle.fixedWing
                                Layout.minimumWidth:_labelWidth
                                Layout.fillWidth:   true
                            }
                            FactTextField {
                                fact:               _landSpeedMC
                                visible:            controller.vehicle && !controller.vehicle.fixedWing
                                Layout.minimumWidth:_editFieldWidth
                                Layout.fillWidth:   true
                            }

                            QGCCheckBox {
                                id:                 disarmDelayCheckBox
                                text:               qsTr("Disarm After:")
                                checked:            _disarmLandDelay.value > 0
                                onClicked:          _disarmLandDelay.value = checked ? 2 : 0
                                Layout.fillWidth:   true
                            }
                            FactTextField {
                                fact:               _disarmLandDelay
                                enabled:            disarmDelayCheckBox.checked
                                Layout.fillWidth:   true
                            }
                        }
                    }
                }

                QGCLabel {
                    text:               qsTr("Vehicle Telemetry Logging")
                }

                Rectangle {
                    width:              mainRow.width      + (_margins * 2)
                    height:             loggingGrid.height + (_margins * 2)
                    color:              qgcPal.windowShade
                    Row {
                        id:                 loggingGrid
                        spacing:            _margins
                        anchors.centerIn:   parent
                        Item {
                            width:                  _imageWidth
                            height:                 _imageHeight
                            anchors.verticalCenter: parent.verticalCenter
                            Image {
                                mipmap:             true
                                fillMode:           Image.PreserveAspectFit
                                source:             qgcPal.globalTheme === QGCPalette.Light ? "/qmlimages/no-logging-light.svg" : "/qmlimages/no-logging.svg"
                                height:             _imageHeight
                                anchors.centerIn:   parent
                            }
                        }
                        GridLayout {
                            columns:                2
                            anchors.verticalCenter: parent.verticalCenter
                            QGCLabel {
                                text:               qsTr("Telemetry logging to vehicle storage:")
                                Layout.minimumWidth:_labelWidth
                                Layout.fillWidth:   true
                            }
                            QGCComboBox {
                                model:              [qsTr("Disabled"), qsTr("Enabled")]
                                enabled:            _enableLogging
                                Layout.minimumWidth:_editFieldWidth
                                Layout.fillWidth:   true
                                Component.onCompleted: {
                                    currentIndex = _enableLogging ? (_enableLogging.value >= 0 ? 1 : 0) : 0
                                }
                                onActivated: {
                                    if(_enableLogging) {
                                        _enableLogging.value = index > 0 ? 0 : -1
                                    }
                                }
                            }
                        }
                    }
                }

                QGCLabel {
                    text:               qsTr("Hardware in the Loop Simulation")
                    visible:            _hitlAvailable
                }

                Rectangle {
                    width:              mainRow.width   + (_margins * 2)
                    height:             hitlGrid.height + (_margins * 2)
                    color:              qgcPal.windowShade
                    visible:            _hitlAvailable
                    Row {
                        id:                 hitlGrid
                        spacing:            _margins
                        anchors.centerIn:   parent
                        Item {
                            width:                  _imageWidth
                            height:                 _imageHeight
                            anchors.verticalCenter: parent.verticalCenter
                            QGCColoredImage {
                                color:              qgcPal.text
                                source:             "/qmlimages/HITL.svg"
                                height:             _imageHeight
                                width:              _imageHeight
                                anchors.centerIn:   parent
                            }
                        }
                        GridLayout {
                            columns:                2
                            anchors.verticalCenter: parent.verticalCenter
                            QGCLabel {
                                text:               qsTr("HITL Enabled:")
                                Layout.minimumWidth:_labelWidth
                                Layout.fillWidth:   true
                            }
                            FactComboBox {
                                fact:               _hitlEnabled
                                indexModel:         false
                                Layout.minimumWidth:_editFieldWidth
                                Layout.fillWidth:   true
                            }
                        }
                    }
                }
            }
        }
    }
}
