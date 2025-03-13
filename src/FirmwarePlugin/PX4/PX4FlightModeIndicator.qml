/****************************************************************************
 *
 * (c) 2009-2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.MultiVehicleManager
import QGroundControl.ScreenTools
import QGroundControl.Palette
import QGroundControl.FactSystem
import QGroundControl.FactControls

FlightModeIndicator {
    waitForParameters: true

    expandedPageComponent: Component {
        ColumnLayout {
            Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 60
            spacing:                margins / 2

            property Fact mpcLandSpeedFact:         controller.getParameterFact(-1, "MPC_LAND_SPEED", false)
            property Fact precisionLandingFact:     controller.getParameterFact(-1, "RTL_PLD_MD", false)
            property Fact sys_vehicle_resp:         controller.getParameterFact(-1, "SYS_VEHICLE_RESP", false)
            property Fact mpc_xy_vel_all:           controller.getParameterFact(-1, "MPC_XY_VEL_ALL", false)
            property Fact mpc_z_vel_all:            controller.getParameterFact(-1, "MPC_Z_VEL_ALL", false)
            property var  qgcPal:                   QGroundControl.globalPalette
            property real margins:                  ScreenTools.defaultFontPixelHeight
            property real sliderWidth:              ScreenTools.defaultFontPixelWidth * 40
            property var  flyViewSettings:          QGroundControl.settingsManager.flyViewSettings

            FactPanelController { id: controller }

            SettingsGroupLayout {
                Layout.fillWidth: true

                FactSlider {
                    Layout.fillWidth:       true
                    Layout.preferredWidth:  sliderWidth
                    label:                  qsTr("RTL Altitude")
                    fact:                   controller.getParameterFact(-1, "RTL_RETURN_ALT")
                    to:                     fact.maxIsDefaultForType ? QGroundControl.unitsConversion.metersToAppSettingsVerticalDistanceUnits(121.92) : fact.max
                    majorTickStepSize:      10
                }
            }

            SettingsGroupLayout {
                Layout.fillWidth:   true
                heading:            qsTr("GeoFence")

                LabelledFactComboBox {
                    Layout.fillWidth:       true
                    label:                  qsTr("Breach Action")
                    fact:                   controller.getParameterFact(-1, "GF_ACTION")
                }

                ColumnLayout {
                    QGCCheckBoxSlider {
                        Layout.fillWidth:   true
                        text:               qsTr("Max Distance")
                        checked:            maxDistanceSlider.value > 0

                        onClicked: {
                            if (checked) {
                                maxDistanceSlider.setValue(prevValue != 0 ? prevValue : maxDistanceSlider.to)
                            } else {
                                prevValue = maxDistanceSlider.value
                                maxDistanceSlider.setValue(0)
                            }
                        }

                        property real prevValue: 0
                    }

                    FactSlider {
                        id:                 maxDistanceSlider
                        Layout.fillWidth:   true
                        fact:               controller.getParameterFact(-1, "GF_MAX_HOR_DIST")
                        to:                 flyViewSettings.maxGoToLocationDistance.value
                        majorTickStepSize:  500
                        enabled:            fact.value > 0
                    }
                }

                ColumnLayout {
                    QGCCheckBoxSlider {
                        Layout.fillWidth:   true
                        text:               qsTr("Max Altitude")
                        checked:            maxAltitudeSlider.value > 0

                        onClicked: {
                            if (checked) {
                                maxAltitudeSlider.setValue(prevValue != 0 ? prevValue : maxAltitudeSlider.to)
                            } else {
                                prevValue = maxAltitudeSlider.value
                                maxAltitudeSlider.setValue(0)
                            }
                        }

                        property real prevValue: 0
                    }

                    FactSlider {
                        id:                 maxAltitudeSlider
                        Layout.fillWidth:   true
                        fact:               controller.getParameterFact(-1, "GF_MAX_VER_DIST")
                        to:                 flyViewSettings.guidedMaximumAltitude.value
                        majorTickStepSize:  10
                        enabled:            fact.value > 0
                    }
                }
            }
        }
    }
}
