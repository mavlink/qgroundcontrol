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

// This indicator page is used both when showing RTK status only with no vehicle connect and when showing GPS/RTK status with a vehicle connected

ToolIndicatorPage {
    showExpand: true

    property var    activeVehicle:      QGroundControl.multiVehicleManager.activeVehicle
    property string na:                 qsTr("N/A", "No data to display")
    property string valueNA:            qsTr("--.--", "No data to display")
    property var    rtkSettings:        QGroundControl.settingsManager.rtkSettings
    property var    baseMode:           rtkSettings.baseMode.rawValue
    property var    manufacturer:       rtkSettings.baseReceiverManufacturers.rawValue
    readonly property var    _standard:           0b00001
    readonly property var    _trimble:            0b00010
    readonly property var    _septentrio:         0b00100
    readonly property var    _femtomes:           0b01000
    readonly property var    _ublox:              0b10000
    
    /* Manufacturer is used to determine witch parameters to displays
     *  1 0b00001 : Standard parameters implemented for all receivers manufacturer
     *  2 0b00010 : Trimble
     *  4 0b00100 : Septentrio
     *  8 0b01000 : Femtomes
     * 16 0b10000 : U-Blox
     *
     * If you want to display :
     * All settings      : 0b11111 (31)
     * Standard settings : 0b00001 ( 1)
     * Only Trimble      : 0b00011 ( 3) = Standard | Trimble
     * Etc ...
    */

    onManufacturerChanged: {
        if (baseMode == 3 && !(manufacturer & _septentrio)){
            baseMode = 1
        }
    }

    contentComponent: Component {
        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight / 2

            SettingsGroupLayout {
                heading: qsTr("Vehicle GPS Status")
                visible: activeVehicle

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

            property real sliderWidth: ScreenTools.defaultFontPixelWidth * 40

            FactCheckBoxSlider {
                Layout.fillWidth:   true
                text:               qsTr("AutoConnect")
                fact:               QGroundControl.settingsManager.autoConnectSettings.autoConnectRTKGPS
                visible:            fact.visible
            }

            GridLayout {
                columns: 2

                QGCLabel {
                    text: qsTr("Manufacturer")
                }
                FactComboBox {
                    Layout.fillWidth:   true
                    // sizeToContents:     true
                    fact:               QGroundControl.settingsManager.rtkSettings.baseReceiverManufacturers
                    visible:            QGroundControl.settingsManager.rtkSettings.baseReceiverManufacturers.visible
                }
            }

            RowLayout {
                QGCRadioButton {
                    text:       qsTr("Survey-In")
                    checked:    baseMode == 0
                    onClicked:  rtkSettings.baseMode.rawValue = 0
                    visible:    manufacturer & _standard
                }

                QGCRadioButton {
                    text: qsTr("Specify position")
                    checked:    baseMode == 1
                    onClicked:  rtkSettings.baseMode.rawValue = 1
                    visible:    manufacturer & _standard
                }
            }

            FactSlider {
                Layout.fillWidth:       true
                Layout.preferredWidth:  sliderWidth
                label:                  qsTr("Accuracy")
                fact:                   QGroundControl.settingsManager.rtkSettings.surveyInAccuracyLimit
                majorTickStepSize:      0.1
                visible:                (
                    baseMode == 0
                    && rtkSettings.surveyInAccuracyLimit.visible
                    && (manufacturer & _ublox)
                )
            }

            FactSlider {
                Layout.fillWidth:       true
                Layout.preferredWidth:  sliderWidth
                label:                  qsTr("Min Duration")
                fact:                   rtkSettings.surveyInMinObservationDuration
                majorTickStepSize:      10
                visible:                ( 
                    baseMode == 0
                    && rtkSettings.surveyInMinObservationDuration.visible
                    && (manufacturer & (_ublox | _femtomes | _trimble))
                )
            }

            LabelledFactTextField {
                label:                  rtkSettings.fixedBasePositionLatitude.shortDescription
                fact:                   rtkSettings.fixedBasePositionLatitude
                visible:                (
                    baseMode == 1
                    && (manufacturer & _standard)
                )
            }

            LabelledFactTextField {
                label:              rtkSettings.fixedBasePositionLongitude.shortDescription
                fact:               rtkSettings.fixedBasePositionLongitude
                visible:            (
                    baseMode == 1
                    && (manufacturer & _standard)
                )
            }

            LabelledFactTextField {
                label:              rtkSettings.fixedBasePositionAltitude.shortDescription
                fact:               rtkSettings.fixedBasePositionAltitude
                visible:            (
                    baseMode == 1
                    && (manufacturer & _standard)
                )
            }

            LabelledFactTextField {
                label:              rtkSettings.fixedBasePositionAccuracy.shortDescription
                fact:               rtkSettings.fixedBasePositionAccuracy
                visible:            (
                    baseMode == 1
                    && (manufacturer & _ublox)
                )
            }

            LabelledButton {
                label:              qsTr("Current Base Position")
                buttonText:         enabled ? qsTr("Save") : qsTr("Not Yet Valid")
                visible:            baseMode == 1
                enabled:            QGroundControl.gpsRtk.valid.value

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
