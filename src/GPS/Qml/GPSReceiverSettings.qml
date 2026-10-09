pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.GPS

SettingsGroupLayout {
    id: root

    property GPSReceiver receiver: QGroundControl.gpsManager.receiver
    property RTKSettings settings: QGroundControl.settingsManager.rtkSettings
    property SettingsFact autoConnectFact: settings.autoConnect as SettingsFact
    property list<gpsSerialPortEntry> serialPorts: root.receiver.serialPortEntries
    property list<int> serialBaudRates: root.receiver.serialBaudRates
    /// Hosts that already show receiver errors elsewhere can hide the inline message.
    property bool showErrorMessage: true

    readonly property int role: settings.receiverRole.rawValue
    readonly property bool configuredBase: role === RTKSettings.ConfiguredBase
    readonly property int manufacturer: settings.baseReceiverManufacturers.rawValue
    readonly property int baseMode: settings.useFixedBasePosition.rawValue
    readonly property gpsReceiverPresentation presentation: receiver.capabilitiesFor(role, manufacturer)
    // A pending automatic reconnect keeps the saved connection; stop it before editing.
    readonly property bool _active: receiver.hasReceiver || receiver.reconnecting
    readonly property bool _editable: !_active
    readonly property int _connection: settings.connectionType.rawValue
    readonly property bool _udp: receiver.effectiveConnectionType === RTKSettings.Udp
    readonly property bool _tcp: receiver.effectiveConnectionType === RTKSettings.Tcp
    readonly property bool _serial: receiver.effectiveConnectionType === RTKSettings.Serial
    readonly property bool _connectionSupported: receiver.connectionSupported
    readonly property list<string> _baseModeNames: settings.useFixedBasePosition.enumStrings
    // Automatic detection words the Quectel notes for a receiver that may not be one.
    readonly property string _quectelOnly: presentation.automatic
                                           ? qsTr("Applies only if a Quectel receiver is identified.") + " " : ""

    heading: qsTr("GNSS Receiver")

    component Field: GPSField {
        required property Fact fact
        /// Shows the field when the setting itself is visible.
        property bool shown: true
        readonly property SettingsFact _setting: fact as SettingsFact

        label: fact.label
        visible: shown && (!_setting || _setting.userVisible)
    }

    component SettingField: Field {
        id: textField
        FactTextField {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            fact: textField.fact
        }
    }

    component ComboField: Field {
        id: comboField
        property alias comboBox: combo
        FactComboBox {
            id: combo
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            fact: comboField.fact
        }
    }

    component SwitchField: Field {
        id: switchField
        property alias toggle: slider
        FactCheckBoxSlider {
            id: slider
            text: ""
            Accessible.name: switchField.fact.label
            fact: switchField.fact
        }
    }

    component ModeButton: QGCRadioButton {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        focusPolicy: Qt.StrongFocus
        wrapMode: Text.Wrap
    }

    ComboField {
        fact: root.settings.receiverRole
        comboBox.objectName: "rtkReceiverRole"
        comboBox.enabled: root._editable
        note: root.configuredBase
              ? qsTr("QGroundControl configures a supported receiver as an RTK base station and forwards its RTCM corrections to vehicles.")
              : qsTr("QGroundControl never configures this receiver. Its NMEA, u-blox UBX or Septentrio SBF position output provides the ground station position. Configure the receiver's output externally and select its existing baud rate. No survey-in status is inferred.")
    }

    SwitchField {
        fact: root.settings.forwardReceiverRtcm
        note: root.settings.forwardReceiverRtcm.shortDescription
        shown: !root.configuredBase
        toggle.objectName: "rtkForwardReceiverRtcm"
        toggle.enabled: root._editable
    }

    ComboField {
        fact: root.settings.baseReceiverManufacturers
        shown: root.configuredBase
        comboBox.objectName: "rtkManufacturer"
        comboBox.enabled: root._editable

        GPSNoteLabel {
            objectName: "rtkAutomaticExplanation"
            visible: root.presentation.automatic
            text: root.settings.baseReceiverManufacturers.shortDescription
        }
    }

    ComboField {
        fact: root.settings.connectionType
        comboBox.objectName: "rtkConnectionType"
        comboBox.enabled: root._editable

        GPSNoteLabel {
            objectName: "rtkTcpOnly"
            visible: !root.receiver.serialSupported && root._connection === RTKSettings.Serial
            text: qsTr("Serial receivers are not supported on this platform, so the receiver connects over TCP.")
        }
    }

    SettingField {
        objectName: "rtkTcpHost"
        fact: root.settings.tcpHost
        enabled: root._editable
        shown: root._tcp
    }

    SettingField {
        objectName: "rtkTcpPort"
        fact: root.settings.tcpPort
        enabled: root._editable
        shown: root._tcp
        note: qsTr("Connect to a receiver's TCP port or a serial-to-TCP bridge. A bridge must already run the receiver link at 115200 baud; QGroundControl cannot change a bridge's rate.")
    }

    SettingField {
        objectName: "rtkUdpPort"
        fact: root.settings.udpPort
        enabled: root._editable
        shown: root._udp

        GPSNoteLabel {
            objectName: "rtkUdpExplanation"
            warning: !root._connectionSupported
            text: root._connectionSupported ? root.settings.udpPort.shortDescription
                                            : qsTr("A configured base needs a serial or TCP connection. UDP only receives data.")
        }
    }

    GPSReceiverSerialPort {
        Layout.minimumWidth: 0
        visible: root.receiver.serialSupported && root._serial
        deviceFact: root.settings.serialDevice
        baudFact: root.settings.serialBaudRate
        serialPorts: root.serialPorts
        serialBaudRates: root.serialBaudRates
        anyDeviceLabel: root.configuredBase && root.autoConnectFact.rawValue ? qsTr("Any RTK receiver on USB") : ""
        allowAutoBaud: !root.presentation.passive
        editable: root._editable
    }

    GPSNoteLabel {
        visible: root._editable && root.configuredBase
        text: root.presentation.automatic
              ? qsTr("If the identified receiver does not support the selected base mode, connecting fails and names the receiver. Auto baud tries every rate the supported receivers use.")
              : !root.presentation.specificReceiver
                ? qsTr("Select a receiver type and its connection to connect manually. Auto baud detects the rate of configurable receivers.")
                : qsTr("Connect only the selected receiver; automatic connections configure it too. A USB adapter's identity only decides which USB ports are tried, not the GNSS manufacturer.")
    }

    GPSNoteLabel {
        objectName: "rtkPersistentConfigurationWarning"
        visible: root.presentation.restartOnConnect
        text: root._quectelOnly + qsTr("Without permission to save, Quectel role and base settings must already match settings saved externally. The receiver restarts on connection. Survey-In counts accepted 1 Hz observations; its accuracy limit filters each observation and does not guarantee final position accuracy.")
    }

    GPSNoteLabel {
        objectName: "rtkSurveySavesPosition"
        visible: root.presentation.surveyMaySavePosition && root.baseMode === BaseModeDefinition.BaseSurveyIn
        text: root._quectelOnly + qsTr("The Quectel receiver may automatically store the completed survey position in its own memory.")
    }

    ColumnLayout {
        Layout.minimumWidth: 0
        visible: root.presentation.rtkBase
        enabled: root._editable

        ModeButton {
            objectName: "rtkSurveyMode"
            text: root._baseModeNames[BaseModeDefinition.BaseSurveyIn]
            checked: root.baseMode === BaseModeDefinition.BaseSurveyIn
            onClicked: root.settings.useFixedBasePosition.rawValue = BaseModeDefinition.BaseSurveyIn
            visible: root.presentation.surveyIn
        }
        ModeButton {
            objectName: "rtkFixedMode"
            text: root._baseModeNames[BaseModeDefinition.BaseFixed]
            checked: root.baseMode === BaseModeDefinition.BaseFixed
            onClicked: root.settings.useFixedBasePosition.rawValue = BaseModeDefinition.BaseFixed
        }
        ModeButton {
            text: root._baseModeNames[BaseModeDefinition.BaseReceiverAveraging]
            checked: root.baseMode === BaseModeDefinition.BaseReceiverAveraging
            onClicked: root.settings.useFixedBasePosition.rawValue = BaseModeDefinition.BaseReceiverAveraging
            visible: root.presentation.receiverAveraging
        }
    }

    GPSNoteLabel {
        visible: !root.receiver.baseModeSupported
        text: qsTr("The selected base mode is not supported by this receiver. Choose a supported mode explicitly.")
    }

    SettingField {
        fact: root.settings.receiverAveragingDuration
        shown: root.presentation.receiverAveraging && root.baseMode === BaseModeDefinition.BaseReceiverAveraging
        enabled: root._editable
        note: root.settings.receiverAveragingDuration.shortDescription
    }

    ColumnLayout {
        Layout.minimumWidth: 0
        spacing: ScreenTools.defaultFontPixelHeight / 2
        visible: root.baseMode === BaseModeDefinition.BaseSurveyIn
                 && (root.presentation.surveyAccuracy || root.presentation.surveyDuration)
        enabled: root._editable

        FactSlider {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            label: root.presentation.observationAccuracyFilter ? qsTr("Observation accuracy limit")
                                                               : root.settings.surveyInAccuracyLimit.label
            fact: root.settings.surveyInAccuracyLimit
            majorTickStepSize: 0.1
            visible: root.presentation.surveyAccuracy && (root.settings.surveyInAccuracyLimit as SettingsFact).userVisible
        }

        FactSlider {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            label: root.presentation.acceptedObservationTime ? qsTr("Accepted observation time")
                                                             : root.settings.surveyInMinObservationDuration.label
            fact: root.settings.surveyInMinObservationDuration
            majorTickStepSize: 10
            visible: root.presentation.surveyDuration
                     && (root.settings.surveyInMinObservationDuration as SettingsFact).userVisible
        }
    }

    // The fixed base position reads as one value, so its coordinates are not divided.
    ColumnLayout {
        Layout.minimumWidth: 0
        spacing: ScreenTools.defaultFontPixelHeight / 2
        visible: root.baseMode === BaseModeDefinition.BaseFixed && root.presentation.rtkBase
        enabled: root._editable

        SettingField { fact: root.settings.fixedBasePositionLatitude }
        SettingField { fact: root.settings.fixedBasePositionLongitude }
        SettingField { fact: root.settings.fixedBasePositionAltitude }
        SettingField {
            fact: root.settings.fixedBasePositionAccuracy
            shown: root.presentation.fixedBaseAccuracy
        }
    }

    SwitchField {
        fact: root.settings.compactRtcmCorrections
        note: root.settings.compactRtcmCorrections.shortDescription
        shown: root.configuredBase && root.presentation.compactObservations
        toggle.objectName: "rtkCompactRtcm"
        toggle.enabled: root._editable
    }

    ColumnLayout {
        Layout.minimumWidth: 0
        spacing: ScreenTools.defaultFontPixelHeight / 4
        visible: root.presentation.persistentConfiguration

        QGCCheckBox {
            objectName: "rtkPersistentChangesCheckBox"
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            text: root.presentation.automatic ? qsTr("Allow flash save and restart if a Quectel receiver is identified")
                                              : qsTr("Allow flash save and restart")
            focusPolicy: Qt.StrongFocus
            wrapMode: Text.Wrap
            enabled: root._editable
            checked: root.receiver.persistentChangesAllowed
            onClicked: root.receiver.persistentChangesAllowed = checked
        }

        GPSNoteLabel {
            objectName: "rtkPersistentConsentWarning"
            text: root._quectelOnly + qsTr("For this connection only, allow QGroundControl to write requested base role or base-setting changes to receiver flash and restart it. Changes may remain saved even if reconnecting fails. No factory reset is performed. Permission is cleared after each attempt and is never used by automatic connections. To use the receiver as a rover again, restore its role with Quectel QGNSS or $PQTMCFGRCVRMODE,W,1 followed by $PQTMSAVEPAR.")
        }
    }

    SwitchField {
        fact: root.autoConnectFact
        note: root.autoConnectFact.rawValue ? root.autoConnectFact.shortDescription : ""
        toggle.objectName: "rtkAutoConnect"
    }

    QGCButton {
        objectName: "rtkConnectButton"
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        wrapMode: Text.Wrap
        focusPolicy: Qt.StrongFocus
        text: root._active ? qsTr("Disconnect") : qsTr("Connect")
        enabled: root._active || root.receiver.canConnect
        onClicked: {
            if (root._active) {
                root.receiver.disconnectReceiver()
            } else {
                root.receiver.connectReceiver()
            }
        }
    }

    GPSNoteLabel {
        objectName: "rtkErrorMessage"
        visible: root.showErrorMessage && text.length > 0
        text: root.receiver.errorMessage || ""
    }

    // The permission is for the configuration shown here, so leaving the page withdraws it.
    Component.onDestruction: {
        if (receiver) {
            receiver.persistentChangesAllowed = false
        }
    }
}
