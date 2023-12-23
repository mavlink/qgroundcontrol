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

    property real   _margins:       ScreenTools.defaultFontPixelHeight
    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property string _NA:            qsTr("N/A", "No data to display")
    property string _valueNA:       qsTr("--.--", "No data to display")

    contentComponent: Component {
        ColumnLayout {
            spacing: _margins

            QGCLabel {
                Layout.alignment:   Qt.AlignHCenter
                text:               qsTr("Vehicle GPS Status")
                font.family:        ScreenTools.demiboldFontFamily
            }

            GridLayout {
                Layout.fillWidth:   true
                columnSpacing:      _margins
                columns:            2

                QGCLabel { Layout.fillWidth: true; text: qsTr("Satellites") }
                QGCLabel { text: _activeVehicle ? _activeVehicle.gps.count.valueString : _NA }

                QGCLabel { Layout.fillWidth: true; text: qsTr("GPS Lock") }
                QGCLabel { text: _activeVehicle ? _activeVehicle.gps.lock.enumStringValue : _NA }

                QGCLabel { Layout.fillWidth: true; text: qsTr("HDOP") }
                QGCLabel { text: _activeVehicle ? _activeVehicle.gps.hdop.valueString : _valueNA }

                QGCLabel { Layout.fillWidth: true; text: qsTr("VDOP") }
                QGCLabel { text: _activeVehicle ? _activeVehicle.gps.vdop.valueString : _valueNA }

                QGCLabel { Layout.fillWidth: true; text: qsTr("Course Over Ground") }
                QGCLabel { text: _activeVehicle ? _activeVehicle.gps.courseOverGround.valueString : _valueNA }
            }

            QGCLabel {
                Layout.alignment:   Qt.AlignHCenter
                text:               qsTr("RTK GPS Status")
                font.family:        ScreenTools.demiboldFontFamily
                visible:            QGroundControl.gpsRtk.connected.value
            }

            GridLayout {
                Layout.fillWidth:   true
                columnSpacing:      _margins
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
                    // during survey-in show the current accuracy, after that show the final accuracy
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
        IndicatorPageGroupLayout {
            heading:        qsTr("RTK GPS Settings")
            showDivider:    false

            FactCheckBoxSlider {
                Layout.fillWidth:   true
                text:               qsTr("AutoConnect")
                fact:               QGroundControl.settingsManager.autoConnectSettings.autoConnectRTKGPS
                visible:            fact.visible
            }

            GridLayout {
                id:         rtkGrid
                columns:    3

                property var  rtkSettings:      QGroundControl.settingsManager.rtkSettings
                property bool useFixedPosition: rtkSettings.useFixedBasePosition.rawValue
                property real firstColWidth:    ScreenTools.defaultFontPixelWidth * 5

                FactCheckBoxSlider {
                    Layout.columnSpan:  3
                    Layout.fillWidth:   true
                    text:               qsTr("Perform Survey-In")
                    fact:               rtkGrid.rtkSettings.useFixedBasePosition
                    checkedValue:       false
                    uncheckedValue:     true
                    visible:            rtkGrid.rtkSettings.useFixedBasePosition.visible
                }

                Item { width: rtkGrid.firstColWidth; height: 1 }
                QGCLabel {
                    text:       rtkGrid.rtkSettings.surveyInAccuracyLimit.shortDescription
                    visible:    rtkGrid.rtkSettings.surveyInAccuracyLimit.visible
                    enabled:    !rtkGrid.useFixedPosition
                }
                FactTextField {
                    fact:                   rtkGrid.rtkSettings.surveyInAccuracyLimit
                    visible:                rtkGrid.rtkSettings.surveyInAccuracyLimit.visible
                    enabled:                !rtkGrid.useFixedPosition
                }

                Item { width: rtkGrid.firstColWidth; height: 1 }
                QGCLabel {
                    text:       rtkGrid.rtkSettings.surveyInMinObservationDuration.shortDescription
                    visible:    rtkGrid.rtkSettings.surveyInMinObservationDuration.visible
                    enabled:    !rtkGrid.useFixedPosition
                }
                FactTextField {
                    fact:                   rtkGrid.rtkSettings.surveyInMinObservationDuration
                    visible:                rtkGrid.rtkSettings.surveyInMinObservationDuration.visible
                    enabled:                !rtkGrid.useFixedPosition
                }

                FactCheckBoxSlider {
                    Layout.columnSpan:  3
                    Layout.fillWidth:   true
                    text:               qsTr("Use Specified Base Position")
                    fact:               rtkGrid.rtkSettings.useFixedBasePosition
                    visible:            rtkGrid.rtkSettings.useFixedBasePosition.visible
                }

                Item { width: rtkGrid.firstColWidth; height: 1 }
                QGCLabel {
                    text:       rtkGrid.rtkSettings.fixedBasePositionLatitude.shortDescription
                    visible:    rtkGrid.rtkSettings.fixedBasePositionLatitude.visible
                    enabled:    rtkGrid.useFixedPosition
                }
                FactTextField {
                    fact:                   rtkGrid.rtkSettings.fixedBasePositionLatitude
                    visible:                rtkGrid.rtkSettings.fixedBasePositionLatitude.visible
                    enabled:                rtkGrid.useFixedPosition
                }

                Item { width: rtkGrid.firstColWidth; height: 1 }
                QGCLabel {
                    text:       rtkGrid.rtkSettings.fixedBasePositionLongitude.shortDescription
                    visible:    rtkGrid.rtkSettings.fixedBasePositionLongitude.visible
                    enabled:    rtkGrid.useFixedPosition
                }
                FactTextField {
                    fact:               rtkGrid.rtkSettings.fixedBasePositionLongitude
                    visible:            rtkGrid.rtkSettings.fixedBasePositionLongitude.visible
                    enabled:            rtkGrid.useFixedPosition
                }

                Item { width: rtkGrid.firstColWidth; height: 1 }
                QGCLabel {
                    text:       rtkGrid.rtkSettings.fixedBasePositionAltitude.shortDescription
                    visible:    rtkGrid.rtkSettings.fixedBasePositionAltitude.visible
                    enabled:    rtkGrid.useFixedPosition
                }
                FactTextField {
                    fact:               rtkGrid.rtkSettings.fixedBasePositionAltitude
                    visible:            rtkGrid.rtkSettings.fixedBasePositionAltitude.visible
                    enabled:            rtkGrid.useFixedPosition
                }

                Item { width: rtkGrid.firstColWidth; height: 1 }
                QGCLabel {
                    text:       rtkGrid.rtkSettings.fixedBasePositionAccuracy.shortDescription
                    visible:    rtkGrid.rtkSettings.fixedBasePositionAccuracy.visible
                    enabled:    rtkGrid.useFixedPosition
                }
                FactTextField {
                    fact:               rtkGrid.rtkSettings.fixedBasePositionAccuracy
                    visible:            rtkGrid.rtkSettings.fixedBasePositionAccuracy.visible
                    enabled:            rtkGrid.useFixedPosition
                }

                Item { width: rtkGrid.firstColWidth; height: 1 }
                RowLayout {
                    Layout.columnSpan:  2

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
                            rtkGrid.rtkSettings.fixedBasePositionLatitude.rawValue  = QGroundControl.gpsRtk.currentLatitude.rawValue
                            rtkGrid.rtkSettings.fixedBasePositionLongitude.rawValue = QGroundControl.gpsRtk.currentLongitude.rawValue
                            rtkGrid.rtkSettings.fixedBasePositionAltitude.rawValue  = QGroundControl.gpsRtk.currentAltitude.rawValue
                            rtkGrid.rtkSettings.fixedBasePositionAccuracy.rawValue  = QGroundControl.gpsRtk.currentAccuracy.rawValue
                        }
                    }
                }
            }
        }
    }
}
