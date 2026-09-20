import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

// This indicator page is used both when showing RTK status only with no vehicle connect and when showing GPS/RTK status with a vehicle connected

ToolIndicatorPage {
    id: root
    showExpand: true

    property var    activeVehicle:      QGroundControl.multiVehicleManager.activeVehicle
    property string na:                 qsTr("N/A", "No data to display")
    property string valueNA:            qsTr("–.––", "No data to display")
    property var    rtkSettings:        QGroundControl.settingsManager.rtkSettings
    property var    useFixedPosition:           rtkSettings.useFixedBasePosition.rawValue
    property var    manufacturer:       rtkSettings.baseReceiverManufacturers.rawValue

    readonly property var _receiver: QGroundControl.gpsManager.gpsRtk
    readonly property var _capabilities: _receiver.capabilitiesForManufacturer(manufacturer)
    readonly property var _activeCapabilities: _receiver.capabilitiesForManufacturer(_receiver.activeManufacturer)
    readonly property var _serialPortManager: QGroundControl.serialPortManager
    readonly property var _serialPorts: _serialPortManager ? _serialPortManager.serialPorts : []
    readonly property var _serialBaudRates: _serialPortManager ? _serialPortManager.serialBaudRates : []
    readonly property bool _passiveConnected: _activeCapabilities.passive
    readonly property bool _averagingConnected: _receiver.activeBaseMode === BaseModeDefinition.BaseReceiverAveraging
    readonly property bool _modeCompatible: _capabilities.passive
        || (useFixedPosition === BaseModeDefinition.BaseFixed && _capabilities.rtkBase)
        || (useFixedPosition === BaseModeDefinition.BaseSurveyIn && _capabilities.surveyIn)
        || (useFixedPosition === BaseModeDefinition.BaseReceiverAveraging && _capabilities.receiverAveraging)
    property bool _allowPersistentChanges: false

    onManufacturerChanged: _allowPersistentChanges = false
    onUseFixedPositionChanged: _allowPersistentChanges = false

    function connectSelectedReceiver() {
        const allowPersistentChanges = manufacturer === 6 && _allowPersistentChanges
        _allowPersistentChanges = false
        return _receiver.connectConfiguredGPS(allowPersistentChanges)
    }

    function errorText() {
        if (!activeVehicle) {
            return qsTr("Disconnected");
        }

        switch (activeVehicle.gps.systemErrors.value) {
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

    Connections {
        target: root._receiver
        function onReceiverChanged() {
            if (root._receiver.hasReceiver) {
                root._allowPersistentChanges = false
            }
        }
    }

    Connections {
        target: root.rtkSettings.serialDevice
        function onRawValueChanged() {
            root._allowPersistentChanges = false
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
                heading:    qsTr("RTK GPS Status")
                visible:    QGroundControl.gpsRtk.connected.value

                QGCLabel {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: root._passiveConnected ? qsTr("Passive RTCM/NMEA input connected")
                          : root._averagingConnected ? qsTr("Receiver-managed averaging — no accuracy guarantee")
                          : QGroundControl.gpsRtk.active.value ? qsTr("Survey-in Active") : qsTr("Receiver connected")
                }

                LabelledLabel {
                    label:      qsTr("Satellites in View")
                    labelText:  QGroundControl.gpsRtk.numSatellites.rawValue < 0
                                ? na : QGroundControl.gpsRtk.numSatellites.valueString
                }

                LabelledLabel {
                    label:      qsTr("Satellites Used")
                    labelText:  QGroundControl.gpsRtk.numSatellitesUsed.rawValue < 0
                                ? na : QGroundControl.gpsRtk.numSatellitesUsed.valueString
                }

                LabelledLabel {
                    label:      root._receiver.activeManufacturer === 6 ? qsTr("Accepted observation time") : qsTr("Duration")
                    visible:    !root._passiveConnected && !root._averagingConnected
                    //: %1 is Survey-In duration in seconds
                    labelText:  qsTr("%1 s").arg(QGroundControl.gpsRtk.currentDuration.value)
                }

                LabelledLabel {
                    label:      QGroundControl.gpsRtk.valid.value ? qsTr("Accuracy") : qsTr("Current Accuracy")
                    labelText:  QGroundControl.gpsRtk.currentAccuracy.valueString + " " + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString
                    visible:    !root._passiveConnected && !root._averagingConnected && QGroundControl.gpsRtk.currentAccuracy.value > 0
                }
            }

            QGCLabel {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                visible: root._receiver.errorMessage.length > 0
                text: root._receiver.errorMessage
                textFormat: Text.PlainText
            }
        }
    }

    expandedComponent: Component {
        SettingsGroupLayout {
            heading:        qsTr("RTK GPS Settings")

            property real sliderWidth: ScreenTools.defaultFontPixelWidth * 40

            FactCheckBoxSlider {
                Layout.fillWidth:   true
                text:               qsTr("Auto-connect known receivers")
                fact:               QGroundControl.settingsManager.autoConnectSettings.autoConnectRTKGPS
                visible:            QGroundControl.settingsManager.autoConnectSettings.autoConnectRTKGPS.userVisible
            }

            GridLayout {
                columns: 2

                QGCLabel {
                    text: qsTr("Receiver / settings")
                }
                FactComboBox {
                    Layout.fillWidth:   true
                    fact:               QGroundControl.settingsManager.rtkSettings.baseReceiverManufacturers
                    enabled:            !root._receiver.hasReceiver
                    visible:            QGroundControl.settingsManager.rtkSettings.baseReceiverManufacturers.userVisible
                }
            }

            LabelledComboBox {
                objectName: "rtkSerialDevice"
                label: qsTr("Serial device")
                visible: root._receiver.serialSupported
                enabled: !root._receiver.hasReceiver && root._serialPorts.length > 0
                model: root._serialPorts.length > 0 ? root._serialPorts : [qsTr("<none available>")]
                currentIndex: root._serialPorts.length > 0 ? root._serialPorts.indexOf(root.rtkSettings.serialDevice.valueString) : 0
                onActivated: (index) => {
                    if (index >= 0 && index < root._serialPorts.length) {
                        root.rtkSettings.serialDevice.rawValue = root._serialPorts[index]
                    }
                }
            }

            LabelledComboBox {
                id: baudCombo
                objectName: "rtkSerialBaudRate"
                label: qsTr("Baud rate")
                visible: root._receiver.serialSupported
                enabled: !root._receiver.hasReceiver
                readonly property string customLabel: qsTr("Custom")
                readonly property var rates: root._serialBaudRates.filter(rate => Number(rate) >= 1200 && Number(rate) <= 4000000)
                property bool customSelected: rates.indexOf(root.rtkSettings.serialBaudRate.valueString) < 0
                model: rates.concat([customLabel])
                currentIndex: customSelected ? rates.length : rates.indexOf(root.rtkSettings.serialBaudRate.valueString)
                onActivated: (index) => {
                    customSelected = index === rates.length
                    if (index >= 0 && index < rates.length) {
                        root.rtkSettings.serialBaudRate.rawValue = Number(rates[index])
                    }
                }
            }

            LabelledFactTextField {
                label: qsTr("Custom baud rate")
                fact: root.rtkSettings.serialBaudRate
                visible: root._receiver.serialSupported && baudCombo.customSelected
                enabled: !root._receiver.hasReceiver
            }

            QGCLabel {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                visible: !root._receiver.hasReceiver
                text: root.manufacturer === 0
                      ? qsTr("Select a specific receiver type, device, and baud rate to connect manually.")
                      : qsTr("Connect only the selected receiver. USB adapter identity does not identify its GNSS manufacturer. Manual connections disable auto-connect.")
            }

            QGCLabel {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                visible: root._capabilities.passive
                text: qsTr("Passive input never configures the receiver. Configure RTCM/NMEA output externally and select its existing baud rate. No survey-in status is inferred.")
            }

            QGCLabel {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                visible: root.manufacturer === 6
                text: qsTr("Without permission to save, Quectel role and base settings must already match settings saved externally. The receiver restarts on connection. Survey-in counts accepted 1 Hz observations; its accuracy limit filters each observation and does not guarantee final position accuracy.")
            }

            QGCLabel {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                visible: root.manufacturer === 6 && root.useFixedPosition === BaseModeDefinition.BaseSurveyIn
                text: qsTr("The Quectel receiver may automatically store the completed survey position in its own memory.")
            }

            ColumnLayout {
                visible: root._capabilities.rtkBase
                enabled: !root._receiver.hasReceiver
                QGCRadioButton {
                    text:       qsTr("Survey-In")
                    checked:    useFixedPosition == BaseModeDefinition.BaseSurveyIn
                    onClicked:  rtkSettings.useFixedBasePosition.rawValue = BaseModeDefinition.BaseSurveyIn
                    visible:    root._capabilities.surveyIn
                }

                QGCRadioButton {
                    text: qsTr("Specify position")
                    checked:    useFixedPosition == BaseModeDefinition.BaseFixed
                    onClicked:  rtkSettings.useFixedBasePosition.rawValue = BaseModeDefinition.BaseFixed
                    visible:    root._capabilities.rtkBase
                }

                QGCRadioButton {
                    text: qsTr("Receiver-managed averaging")
                    checked: root.useFixedPosition === BaseModeDefinition.BaseReceiverAveraging
                    onClicked: root.rtkSettings.useFixedBasePosition.rawValue = BaseModeDefinition.BaseReceiverAveraging
                    visible: root._capabilities.receiverAveraging
                }
            }

            QGCLabel {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                visible: !root._modeCompatible
                text: qsTr("The selected base mode is not supported by this receiver. Choose a supported mode explicitly.")
            }

            QGCLabel {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                visible: root._capabilities.receiverAveraging && root.useFixedPosition === BaseModeDefinition.BaseReceiverAveraging
                text: qsTr("The receiver averages its position for up to the maximum time. This is not accuracy-controlled survey-in and does not guarantee a position accuracy.")
            }

            LabelledFactTextField {
                label: qsTr("Maximum averaging time")
                fact: root.rtkSettings.receiverAveragingDuration
                visible: root._capabilities.receiverAveraging && root.useFixedPosition === BaseModeDefinition.BaseReceiverAveraging
                enabled: !root._receiver.hasReceiver
            }

            FactSlider {
                Layout.fillWidth:       true
                Layout.preferredWidth:  sliderWidth
                label:                  root.manufacturer === 6 ? qsTr("Observation accuracy limit") : qsTr("Accuracy")
                fact:                   QGroundControl.settingsManager.rtkSettings.surveyInAccuracyLimit
                majorTickStepSize:      0.1
                enabled:                !root._receiver.hasReceiver
                visible:                (
                    useFixedPosition == BaseModeDefinition.BaseSurveyIn
                    && rtkSettings.surveyInAccuracyLimit.userVisible
                    && root._capabilities.surveyIn
                    && (root.manufacturer === 0 || root.manufacturer === 4 || root.manufacturer === 6)
                )
            }

            FactSlider {
                Layout.fillWidth:       true
                Layout.preferredWidth:  sliderWidth
                label:                  root.manufacturer === 6 ? qsTr("Accepted observation time") : qsTr("Min Duration")
                fact:                   rtkSettings.surveyInMinObservationDuration
                majorTickStepSize:      10
                enabled:                !root._receiver.hasReceiver
                visible:                (
                    useFixedPosition == BaseModeDefinition.BaseSurveyIn
                    && rtkSettings.surveyInMinObservationDuration.userVisible
                    && root._capabilities.surveyIn
                    && root.manufacturer !== 2
                )
            }

            LabelledFactTextField {
                label:                  rtkSettings.fixedBasePositionLatitude.shortDescription
                fact:                   rtkSettings.fixedBasePositionLatitude
                enabled:                !root._receiver.hasReceiver
                visible:                (
                    useFixedPosition == BaseModeDefinition.BaseFixed
                    && root._capabilities.rtkBase
                )
            }

            LabelledFactTextField {
                label:              rtkSettings.fixedBasePositionLongitude.shortDescription
                fact:               rtkSettings.fixedBasePositionLongitude
                enabled:            !root._receiver.hasReceiver
                visible:            (
                    useFixedPosition == BaseModeDefinition.BaseFixed
                    && root._capabilities.rtkBase
                )
            }

            LabelledFactTextField {
                label:              rtkSettings.fixedBasePositionAltitude.shortDescription
                fact:               rtkSettings.fixedBasePositionAltitude
                enabled:            !root._receiver.hasReceiver
                visible:            (
                    useFixedPosition == BaseModeDefinition.BaseFixed
                    && root._capabilities.rtkBase
                )
            }

            LabelledFactTextField {
                label:              rtkSettings.fixedBasePositionAccuracy.shortDescription
                fact:               rtkSettings.fixedBasePositionAccuracy
                enabled:            !root._receiver.hasReceiver
                visible:            (
                    useFixedPosition == BaseModeDefinition.BaseFixed
                    && root._capabilities.rtkBase
                    && (root.manufacturer === 0 || root.manufacturer === 4)
                )
            }

            LabelledButton {
                label:              qsTr("Current Base Position")
                buttonText:         !QGroundControl.gpsRtk.valid.rawValue ? qsTr("Not Yet Valid")
                                    : !Number.isFinite(QGroundControl.gpsRtk.currentAccuracy.rawValue)
                                      || QGroundControl.gpsRtk.currentAccuracy.rawValue < 0
                                      ? qsTr("Accuracy Unavailable")
                                    : QGroundControl.gpsRtk.canSaveCurrentBasePosition ? qsTr("Save")
                                    : qsTr("Invalid Base Position")
                visible:            root._capabilities.rtkBase && useFixedPosition == BaseModeDefinition.BaseFixed
                enabled:            QGroundControl.gpsRtk.canSaveCurrentBasePosition

                onClicked: {
                    if (!QGroundControl.gpsRtk.canSaveCurrentBasePosition) {
                        return
                    }
                    const latitude = QGroundControl.gpsRtk.currentLatitude.rawValue
                    const longitude = QGroundControl.gpsRtk.currentLongitude.rawValue
                    const altitude = QGroundControl.gpsRtk.currentAltitude.rawValue
                    const accuracy = QGroundControl.gpsRtk.currentAccuracy.rawValue
                    rtkSettings.fixedBasePositionLatitude.rawValue  = latitude
                    rtkSettings.fixedBasePositionLongitude.rawValue = longitude
                    rtkSettings.fixedBasePositionAltitude.rawValue  = altitude
                    rtkSettings.fixedBasePositionAccuracy.rawValue  = accuracy
                }
            }

            QGCCheckBox {
                objectName: "rtkPersistentChangesCheckBox"
                text: qsTr("Allow flash save and restart")
                focusPolicy: Qt.StrongFocus
                visible: root._receiver.serialSupported && root.manufacturer === 6
                enabled: !root._receiver.hasReceiver
                checked: root._allowPersistentChanges
                onClicked: root._allowPersistentChanges = checked
            }

            QGCLabel {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                visible: root._receiver.serialSupported && root.manufacturer === 6
                text: qsTr("For this connection only, allow QGroundControl to write requested role or base-setting changes to receiver flash and restart it. Changes may remain saved even if reconnecting fails. No factory reset is performed. Permission is cleared after each attempt and is never used by auto-connect.")
            }

            LabelledButton {
                objectName: "rtkConnectButton"
                label: root._receiver.hasReceiver
                       ? (QGroundControl.gpsRtk.connected.value ? qsTr("Receiver connected") : qsTr("Connecting receiver"))
                       : qsTr("Manual serial connection")
                buttonText: root._receiver.hasReceiver ? qsTr("Disconnect") : qsTr("Connect")
                visible: root._receiver.serialSupported
                enabled: root._receiver.hasReceiver || (root.manufacturer !== 0 && root._modeCompatible)
                onClicked: {
                    if (root._receiver.hasReceiver) {
                        root._allowPersistentChanges = false
                        root._receiver.disconnectConfiguredGPS()
                    } else {
                        root.connectSelectedReceiver()
                    }
                }

            }
        }
    }
}
