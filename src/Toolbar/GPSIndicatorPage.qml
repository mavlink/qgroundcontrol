pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

// This indicator page is used both when showing RTK status only with no vehicle connect and when showing GPS/RTK status with a vehicle connected

ToolIndicatorPage {
    id: root
    showExpand: root._baseStation

    readonly property bool _baseStation: rtkSettings.receiverRole.rawValue === RTKSettings.RTKBase
    readonly property var _receiverHealth: QGroundControl.gpsManager.rtkConnection.health

    property var    activeVehicle:      QGroundControl.multiVehicleManager.activeVehicle
    property string na:                 qsTr("N/A", "No data to display")
    property string valueNA:            qsTr("–.––", "No data to display")
    property var    rtkSettings:        QGroundControl.settingsManager.rtkSettings
    property var    useFixedPosition:           rtkSettings.useFixedBasePosition.rawValue
    property var    manufacturer:       rtkSettings.baseReceiverManufacturers.rawValue

    readonly property var    _trimble:            0b0001
    readonly property var    _septentrio:         0b0010
    readonly property var    _femtomes:           0b0100
    readonly property var    _ublox:              0b1000
    readonly property var    _all:                0b1111
    property var             settingsDisplayId:     _all

    function updateSettingsDisplayId() {
        switch(manufacturer) {
            case 0: // All
                settingsDisplayId = _trimble | _septentrio | _femtomes | _ublox
                break
            case 1: // Trimble
                settingsDisplayId = _trimble
                break
            case 2: // Septentrio
                settingsDisplayId = _septentrio
                break
            case 3: // Femtomes
                settingsDisplayId = _femtomes
                break
            case 4: // UBlox
                settingsDisplayId = _ublox
                break
            default:
                settingsDisplayId = _all
        }
    }

    onManufacturerChanged: {
        updateSettingsDisplayId()
    }

    Component.onCompleted: {
        updateSettingsDisplayId()
    }

    function errorText() {
        if (!root.activeVehicle) {
            return qsTr("Disconnected");
        }

        switch (root.activeVehicle.gps.systemErrors.value) {
            case 1:
                return qsTr("Incoming correction");
            case 2:
                return qsTr("Configuration");
            case 4:
                return qsTr("Software");
            case 8:
                return qsTr("Antenna");
            case 16:
                return qsTr("Event congestion");
            case 32:
                return qsTr("CPU overload");
            case 64:
                return qsTr("Output congestion");
            default:
                return qsTr("Multiple errors");
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

                LabelledLabel {
                    label: qsTr("GPS Error")
                    labelText: errorText()
                    visible: activeVehicle && activeVehicle.gps.systemErrors.value > 0
                }
            }

            SettingsGroupLayout {
                heading:    root._baseStation ? qsTr("RTK Base Status") : qsTr("GNSS Receiver Status")
                visible:    QGroundControl.gpsReceiver.connected.value

                QGCLabel {
                    text: root._baseStation
                          ? (QGroundControl.gpsReceiver.rtk.active.value ? qsTr("Survey-in Active") : qsTr("RTK Streaming"))
                          : (root._receiverHealth.usable ? qsTr("Position available") : qsTr("Waiting for position"))
                }

                LabelledLabel {
                    readonly property int satelliteCount: root._baseStation
                                                          ? QGroundControl.gpsReceiver.numSatellites.value
                                                          : root._receiverHealth.satellitesInUseCount
                    label:      qsTr("Satellites")
                    labelText: satelliteCount >= 0 ? satelliteCount : root.na
                }

                LabelledLabel {
                    visible:    root._baseStation
                    label:      qsTr("Duration")
                    labelText:  QGroundControl.gpsReceiver.rtk.currentDuration.value + ' s'
                }

                LabelledLabel {
                    label:      QGroundControl.gpsReceiver.rtk.valid.value ? qsTr("Accuracy") : qsTr("Current Accuracy")
                    labelText:  QGroundControl.gpsReceiver.rtk.currentAccuracy.valueString + " " + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString
                    visible:    root._baseStation && QGroundControl.gpsReceiver.rtk.currentAccuracy.value > 0
                }
            }
        }
    }

    expandedComponent: Component {
        SettingsGroupLayout {
            heading:        qsTr("RTK Base Settings")
            visible:        root._baseStation

            property real sliderWidth: ScreenTools.defaultFontPixelWidth * 40

            FactCheckBoxSlider {
                Layout.fillWidth:   true
                text:               qsTr("AutoConnect")
                fact:               QGroundControl.settingsManager.autoConnectSettings.autoConnectRTKGPS
                visible:            fact.userVisible
            }

            GridLayout {
                columns: 2

                QGCLabel {
                    text: qsTr("Settings displayed")
                }
                FactComboBox {
                    Layout.fillWidth:   true
                    fact:               QGroundControl.settingsManager.rtkSettings.baseReceiverManufacturers
                    visible:            QGroundControl.settingsManager.rtkSettings.baseReceiverManufacturers.userVisible
                }
            }

            RowLayout {
                QGCRadioButton {
                    text:       qsTr("Survey-In")
                    checked:    useFixedPosition == BaseModeDefinition.BaseSurveyIn
                    onClicked:  rtkSettings.useFixedBasePosition.rawValue = BaseModeDefinition.BaseSurveyIn
                    visible:    settingsDisplayId & _all
                }

                QGCRadioButton {
                    text: qsTr("Specify position")
                    checked:    useFixedPosition == BaseModeDefinition.BaseFixed
                    onClicked:  rtkSettings.useFixedBasePosition.rawValue = BaseModeDefinition.BaseFixed
                    visible:    settingsDisplayId & _all
                }
            }

            FactSlider {
                Layout.fillWidth:       true
                Layout.preferredWidth:  sliderWidth
                label:                  qsTr("Accuracy")
                fact:                   QGroundControl.settingsManager.rtkSettings.surveyInAccuracyLimit
                majorTickStepSize:      0.1
                visible:                (
                    useFixedPosition == BaseModeDefinition.BaseSurveyIn
                    && rtkSettings.surveyInAccuracyLimit.userVisible
                    && (settingsDisplayId & _ublox)
                )
            }

            FactSlider {
                Layout.fillWidth:       true
                Layout.preferredWidth:  sliderWidth
                label:                  qsTr("Min Duration")
                fact:                   rtkSettings.surveyInMinObservationDuration
                majorTickStepSize:      10
                visible:                (
                    useFixedPosition == BaseModeDefinition.BaseSurveyIn
                    && rtkSettings.surveyInMinObservationDuration.userVisible
                    && (settingsDisplayId & (_ublox | _femtomes | _trimble))
                )
            }

            LabelledFactTextField {
                label:                  rtkSettings.fixedBasePositionLatitude.shortDescription
                fact:                   rtkSettings.fixedBasePositionLatitude
                visible:                (
                    useFixedPosition == BaseModeDefinition.BaseFixed
                    && (settingsDisplayId & _all)
                )
            }

            LabelledFactTextField {
                label:              rtkSettings.fixedBasePositionLongitude.shortDescription
                fact:               rtkSettings.fixedBasePositionLongitude
                visible:            (
                    useFixedPosition == BaseModeDefinition.BaseFixed
                    && (settingsDisplayId & _all)
                )
            }

            LabelledFactTextField {
                label:              rtkSettings.fixedBasePositionAltitude.shortDescription
                fact:               rtkSettings.fixedBasePositionAltitude
                visible:            (
                    useFixedPosition == BaseModeDefinition.BaseFixed
                    && (settingsDisplayId & _all)
                )
            }

            LabelledFactTextField {
                label:              rtkSettings.fixedBasePositionAccuracy.shortDescription
                fact:               rtkSettings.fixedBasePositionAccuracy
                visible:            (
                    useFixedPosition == BaseModeDefinition.BaseFixed
                    && (settingsDisplayId & _ublox)
                )
            }

            LabelledButton {
                objectName:         "saveBaseReference"
                label:              qsTr("Current Base Position")
                buttonText:         enabled ? qsTr("Save") : qsTr("Unavailable")
                visible:            useFixedPosition == BaseModeDefinition.BaseFixed
                enabled:            QGroundControl.gpsManager.canSaveBaseReference

                onClicked: QGroundControl.gpsManager.saveBaseReference()
            }

            QGCLabel {
                Layout.fillWidth: true
                text: QGroundControl.gpsManager.baseReferenceSaveError
                visible: useFixedPosition == BaseModeDefinition.BaseFixed && text.length > 0
                wrapMode: Text.WordWrap
            }
        }
    }
}
