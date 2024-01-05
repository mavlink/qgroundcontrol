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

    property real   margins:            ScreenTools.defaultFontPixelHeight
    property var    activeVehicle:      QGroundControl.multiVehicleManager.activeVehicle
    property string na:                 qsTr("N/A", "No data to display")
    property string valueNA:            qsTr("--.--", "No data to display")
    property var    rtkSettings:        QGroundControl.settingsManager.rtkSettings
    property bool   useFixedPosition:   rtkSettings.useFixedBasePosition.rawValue

    contentComponent: Component {
        ColumnLayout {
            spacing: margins

            QGCLabel {
                Layout.alignment:   Qt.AlignHCenter
                text:               qsTr("Vehicle GPS Status")
                font.family:        ScreenTools.demiboldFontFamily
            }

            GridLayout {
                Layout.fillWidth:   true
                columnSpacing:      margins
                columns:            2

                QGCLabel { Layout.fillWidth: true; text: qsTr("Satellites") }
                QGCLabel { text: activeVehicle ? activeVehicle.gps.count.valueString : na }

                QGCLabel { Layout.fillWidth: true; text: qsTr("GPS Lock") }
                QGCLabel { text: activeVehicle ? activeVehicle.gps.lock.enumStringValue : na }

                QGCLabel { Layout.fillWidth: true; text: qsTr("HDOP") }
                QGCLabel { text: activeVehicle ? activeVehicle.gps.hdop.valueString : valueNA }

                QGCLabel { Layout.fillWidth: true; text: qsTr("VDOP") }
                QGCLabel { text: activeVehicle ? activeVehicle.gps.vdop.valueString : valueNA }

                QGCLabel { Layout.fillWidth: true; text: qsTr("Course Over Ground") }
                QGCLabel { text: activeVehicle ? activeVehicle.gps.courseOverGround.valueString : valueNA }
            }

            QGCLabel {
                Layout.alignment:   Qt.AlignHCenter
                text:               qsTr("RTK GPS Status")
                font.family:        ScreenTools.demiboldFontFamily
                visible:            QGroundControl.gpsRtk.connected.value
            }

            GridLayout {
                Layout.fillWidth:   true
                columnSpacing:      margins
                columns:            2
                visible:            QGroundControl.gpsRtk.connected.value

                QGCLabel {
                    Layout.alignment:   Qt.AlignLeft
                    Layout.columnSpan:  2
                    text:               (QGroundControl.gpsRtk.active.value) ? qsTr("Survey-in Active") : qsTr("RTK Streaming")
                }

                QGCLabel { Layout.fillWidth: true; text: qsTr("Satellites") }
                QGCLabel { text: QGroundControl.gpsRtk.numSatellites.value }

                QGCLabel { Layout.fillWidth: true; text: qsTr("Duration") }
                QGCLabel { text: QGroundControl.gpsRtk.currentDuration.value + ' s' }

                QGCLabel {
                    // during survey-in show the current Accuracy, after that show the final Accuracy
                    id:                 accuracyLabel
                    Layout.fillWidth:   true
                    text:               QGroundControl.gpsRtk.valid.value ? qsTr("Accuracy") : qsTr("Current Accuracy")
                    visible:            QGroundControl.gpsRtk.currentAccuracy.value > 0
                }
                QGCLabel {
                    text:       QGroundControl.gpsRtk.currentAccuracy.valueString + " " + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString
                    visible:    accuracyLabel.visible
                }
            }
        }
    }

    expandedComponent: Component {
        SettingsGroupLayout {
            heading:        qsTr("RTK GPS Settings")
            showDivider:    false

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
                label:                  rtkSettings.surveyInAccuracyLimit.shortDescription
                fact:                   QGroundControl.settingsManager.rtkSettings.surveyInAccuracyLimit
                visible:                rtkSettings.surveyInAccuracyLimit.visible
                enabled:                !useFixedPosition

                Component.onCompleted: console.log("increment", fact.increment)
            }

            LabelledFactSlider {
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
