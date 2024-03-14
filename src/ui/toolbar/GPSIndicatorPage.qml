/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.MultiVehicleManager
import QGroundControl.ScreenTools
import QGroundControl.Palette
import QGroundControl.FactSystem
import QGroundControl.FactControls

ToolIndicatorPage {
    showExpand: true

    property var    activeVehicle:      QGroundControl.multiVehicleManager.activeVehicle
    property string na:                 qsTr("N/A", "No data to display")
    property string valueNA:            qsTr("--.--", "No data to display")
    property var    rtkSettings:        QGroundControl.settingsManager.rtkSettings
    property bool   useFixedPosition:   rtkSettings.useFixedBasePosition.rawValue

    contentComponent: Component {
        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight / 2

            SettingsGroupLayout {
                heading: qsTr("Vehicle GPS Status")

                LabelledLabel {
                    label:      qsTr("Satellites")
                    labelText:  activeVehicle ? activeVehicle.gps.count.valueString : na
                }

                LabelledLabel {
                    label:      qsTr("GPS Lock")
                    labelText:  activeVehicle ? activeVehicle.gps.lock.enumStringValue : na
                }

                LabelledLabel {
                    label:      qsTr("HDOP")
                    labelText:  activeVehicle ? activeVehicle.gps.hdop.valueString : valueNA
                }

                LabelledLabel {
                    label:      qsTr("VDOP")
                    labelText:  activeVehicle ? activeVehicle.gps.vdop.valueString : valueNA
                }

                LabelledLabel {
                    label:      qsTr("Course Over Ground")
                    labelText:  activeVehicle ? activeVehicle.gps.courseOverGround.valueString : valueNA
                }
            }

            SettingsGroupLayout {
                heading:    qsTr("RTK GPS Status")
                visible:    QGroundControl.gpsRtk.connected.value

                QGCLabel {
                    text: (QGroundControl.gpsRtk.active.value) ? qsTr("Survey-in Active") : qsTr("RTK Streaming")
                }

                LabelledLabel {
                    label:      qsTr("Satellites")
                    labelText:  QGroundControl.gpsRtk.numSatellites.value
                }

                LabelledLabel {
                    label:      qsTr("Duration")
                    labelText:  QGroundControl.gpsRtk.currentDuration.value + ' s'
                }

                LabelledLabel {
                    label:      QGroundControl.gpsRtk.valid.value ? qsTr("Accuracy") : qsTr("Current Accuracy")
                    labelText:  QGroundControl.gpsRtk.currentAccuracy.valueString + " " + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString
                    visible:    QGroundControl.gpsRtk.currentAccuracy.value > 0
                }
            }
        }
    }

    expandedComponent: Component {
        SettingsGroupLayout {
            heading:        qsTr("RTK GPS Settings")

            property real sliderWidth: ScreenTools.defaultFontPixelWidth * 32 // Size is tuned so expanded page fits Herelink screen without horiz scrolling

            FactCheckBoxSlider {
                Layout.fillWidth:   true
                text:               qsTr("AutoConnect")
                fact:               QGroundControl.settingsManager.autoConnectSettings.autoConnectRTKGPS
                visible:            fact.visible
            }

            FactCheckBoxSlider {
                Layout.fillWidth:   true
                text:               qsTr("Perform Survey-In")
                fact:               rtkSettings.useFixedBasePosition
                checkedValue:       false
                uncheckedValue:     true
                visible:            rtkSettings.useFixedBasePosition.visible
            }

            LabelledFactSlider {
                sliderPreferredWidth:   sliderWidth
                label:                  rtkSettings.surveyInAccuracyLimit.shortDescription
                fact:                   QGroundControl.settingsManager.rtkSettings.surveyInAccuracyLimit
                visible:                rtkSettings.surveyInAccuracyLimit.visible
                enabled:                !useFixedPosition

                Component.onCompleted: console.log("increment", fact.increment)
            }

            LabelledFactSlider {
                sliderPreferredWidth:   sliderWidth
                label:                  rtkSettings.surveyInMinObservationDuration.shortDescription
                fact:                   rtkSettings.surveyInMinObservationDuration
                visible:                rtkSettings.surveyInMinObservationDuration.visible
                enabled:                !useFixedPosition
            }

            FactCheckBoxSlider {
                Layout.columnSpan:  3
                Layout.fillWidth:   true
                text:               qsTr("Use Specified Base Position")
                fact:               rtkSettings.useFixedBasePosition
                visible:            rtkSettings.useFixedBasePosition.visible
            }

            LabelledFactTextField {
                label:                  rtkSettings.fixedBasePositionLatitude.shortDescription
                fact:                   rtkSettings.fixedBasePositionLatitude
                visible:                rtkSettings.fixedBasePositionLatitude.visible
                enabled:                useFixedPosition
            }

            LabelledFactTextField {
                label:              rtkSettings.fixedBasePositionLongitude.shortDescription
                fact:               rtkSettings.fixedBasePositionLongitude
                visible:            rtkSettings.fixedBasePositionLongitude.visible
                enabled:            useFixedPosition
            }

            LabelledFactTextField {
                label:              rtkSettings.fixedBasePositionAltitude.shortDescription
                fact:               rtkSettings.fixedBasePositionAltitude
                visible:            rtkSettings.fixedBasePositionAltitude.visible
                enabled:            useFixedPosition
            }

            LabelledFactTextField {
                label:              rtkSettings.fixedBasePositionAccuracy.shortDescription
                fact:               rtkSettings.fixedBasePositionAccuracy
                visible:            rtkSettings.fixedBasePositionAccuracy.visible
                enabled:            useFixedPosition
            }

            RowLayout {
                spacing: ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    Layout.fillWidth:   true;
                    text:               qsTr("Current Base Position")
                    enabled:            saveBasePositionButton.enabled
                }

                QGCButton {
                    id:         saveBasePositionButton
                    text:       enabled ? qsTr("Save") : qsTr("Not Yet Valid")
                    enabled:    QGroundControl.gpsRtk.valid.value

                    onClicked: {
                        rtkSettings.fixedBasePositionLatitude.rawValue  = QGroundControl.gpsRtk.currentLatitude.rawValue
                        rtkSettings.fixedBasePositionLongitude.rawValue = QGroundControl.gpsRtk.currentLongitude.rawValue
                        rtkSettings.fixedBasePositionAltitude.rawValue  = QGroundControl.gpsRtk.currentAltitude.rawValue
                        rtkSettings.fixedBasePositionAccuracy.rawValue  = QGroundControl.gpsRtk.currentAccuracy.rawValue
                    }
                }
            }
        }
    }
}
