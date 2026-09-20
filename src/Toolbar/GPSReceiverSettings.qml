pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SettingsGroupLayout {
    id: root

    required property var receiver
    required property var settings
    required property var baseFacts
    required property var autoConnectFact
    property var serialPorts: []
    property var serialBaudRates: []
    property var consent: QtObject { property bool allowed: false }

    readonly property int manufacturer: settings.baseReceiverManufacturers.rawValue
    readonly property int baseMode: settings.useFixedBasePosition.rawValue
    readonly property var presentation: receiver.capabilitiesForManufacturer(manufacturer)
    readonly property bool modeCompatible: presentation.passive
        || (baseMode === BaseModeDefinition.BaseFixed && presentation.rtkBase)
        || (baseMode === BaseModeDefinition.BaseSurveyIn && presentation.surveyIn)
        || (baseMode === BaseModeDefinition.BaseReceiverAveraging && presentation.receiverAveraging)
    readonly property bool _editable: !receiver.hasReceiver

    implicitWidth: ScreenTools.defaultFontPixelWidth * 56
    heading: qsTr("RTK GPS Settings")

    onManufacturerChanged: clearConsent()
    onBaseModeChanged: clearConsent()
    onReceiverChanged: clearConsent()
    onSettingsChanged: clearConsent()
    Component.onDestruction: clearConsent()

    function clearConsent() {
        if (consent) {
            consent.allowed = false
        }
    }

    function connectSelectedReceiver() {
        const allowPersistentChanges = presentation.persistentConfiguration && consent.allowed
        clearConsent()
        return receiver.connectConfiguredGPS(allowPersistentChanges)
    }

    function saveCurrentBasePosition() {
        if (!baseFacts.canSaveCurrentBasePosition) {
            return false
        }
        const latitude = baseFacts.currentLatitude.rawValue
        const longitude = baseFacts.currentLongitude.rawValue
        const altitude = baseFacts.currentAltitude.rawValue
        const accuracy = baseFacts.currentAccuracy.rawValue
        if (![latitude, longitude, altitude, accuracy].every(Number.isFinite)
            || accuracy < 0 || Math.abs(latitude) > 90 || Math.abs(longitude) > 180) {
            return false
        }
        settings.fixedBasePositionLatitude.rawValue = latitude
        settings.fixedBasePositionLongitude.rawValue = longitude
        settings.fixedBasePositionAltitude.rawValue = altitude
        settings.fixedBasePositionAccuracy.rawValue = accuracy
        return true
    }

    component Explanation: QGCLabel {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredWidth: 0
        wrapMode: Text.Wrap
        textFormat: Text.PlainText
    }

    component SettingField: ColumnLayout {
        id: field
        required property Fact fact
        property string label: fact.shortDescription

        Layout.fillWidth: true
        Layout.minimumWidth: 0
        spacing: ScreenTools.defaultFontPixelHeight / 4

        Explanation { text: field.label }
        FactTextField {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            fact: field.fact
        }
    }

    component ModeButton: QGCRadioButton {
        id: button
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        focusPolicy: Qt.StrongFocus
        contentItem: QGCLabel {
            text: button.text
            color: button.textColor
            leftPadding: button.indicator.width + ScreenTools.defaultFontPixelWidth / 2
            wrapMode: Text.Wrap
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        visible: root.autoConnectFact.userVisible
        Explanation { text: qsTr("Auto-connect known receivers") }
        FactCheckBoxSlider {
            text: ""
            Accessible.name: qsTr("Auto-connect known receivers")
            fact: root.autoConnectFact
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        visible: root.settings.baseReceiverManufacturers.userVisible
        Explanation { text: qsTr("Receiver / settings") }
        FactComboBox {
            objectName: "rtkManufacturer"
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            fact: root.settings.baseReceiverManufacturers
            enabled: root._editable
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        visible: root.receiver.serialSupported
        Explanation { text: qsTr("Serial device") }
        QGCComboBox {
            objectName: "rtkSerialDevice"
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            enabled: root._editable && root.serialPorts.length > 0
            model: root.serialPorts.length > 0 ? root.serialPorts : [qsTr("<none available>")]
            currentIndex: root.serialPorts.length > 0
                          ? root.serialPorts.indexOf(root.settings.serialDevice.valueString) : 0
            onActivated: (index) => {
                if (index >= 0 && index < root.serialPorts.length) {
                    root.settings.serialDevice.rawValue = root.serialPorts[index]
                }
            }
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        visible: root.receiver.serialSupported
        Explanation { text: qsTr("Baud rate") }
        QGCComboBox {
            id: baudCombo
            objectName: "rtkSerialBaudRate"
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            enabled: root._editable

            readonly property var rates: root.serialBaudRates.filter(rate => Number(rate) >= 1200 && Number(rate) <= 4000000)
            property bool customSelected: rates.indexOf(root.settings.serialBaudRate.valueString) < 0

            model: rates.concat([qsTr("Custom")])
            currentIndex: customSelected ? rates.length : rates.indexOf(root.settings.serialBaudRate.valueString)
            onActivated: (index) => {
                customSelected = index === rates.length
                if (index >= 0 && index < rates.length) {
                    root.settings.serialBaudRate.rawValue = Number(rates[index])
                }
            }
        }
    }

    SettingField {
        label: qsTr("Custom baud rate")
        fact: root.settings.serialBaudRate
        visible: root.receiver.serialSupported && baudCombo.customSelected
        enabled: root._editable
    }

    Explanation {
        visible: root._editable
        text: !root.presentation.specificReceiver
              ? qsTr("Select a specific receiver type, device, and baud rate to connect manually.")
              : qsTr("Connect only the selected receiver. USB adapter identity does not identify its GNSS manufacturer. Manual connections disable auto-connect.")
    }

    Explanation {
        visible: root.presentation.passive
        text: qsTr("Passive input never configures the receiver. Configure RTCM/NMEA output externally and select its existing baud rate. No survey-in status is inferred.")
    }

    Explanation {
        objectName: "rtkPersistentConfigurationWarning"
        visible: root.presentation.restartOnConnect
        text: qsTr("Without permission to save, Quectel role and base settings must already match settings saved externally. The receiver restarts on connection. Survey-in counts accepted 1 Hz observations; its accuracy limit filters each observation and does not guarantee final position accuracy.")
    }

    Explanation {
        visible: root.presentation.surveyMaySavePosition && root.baseMode === BaseModeDefinition.BaseSurveyIn
        text: qsTr("The Quectel receiver may automatically store the completed survey position in its own memory.")
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        visible: root.presentation.rtkBase
        enabled: root._editable

        ModeButton {
            objectName: "rtkSurveyMode"
            text: qsTr("Survey-In")
            checked: root.baseMode === BaseModeDefinition.BaseSurveyIn
            onClicked: root.settings.useFixedBasePosition.rawValue = BaseModeDefinition.BaseSurveyIn
            visible: root.presentation.surveyIn
        }
        ModeButton {
            objectName: "rtkFixedMode"
            text: qsTr("Specify position")
            checked: root.baseMode === BaseModeDefinition.BaseFixed
            onClicked: root.settings.useFixedBasePosition.rawValue = BaseModeDefinition.BaseFixed
        }
        ModeButton {
            text: qsTr("Receiver-managed averaging")
            checked: root.baseMode === BaseModeDefinition.BaseReceiverAveraging
            onClicked: root.settings.useFixedBasePosition.rawValue = BaseModeDefinition.BaseReceiverAveraging
            visible: root.presentation.receiverAveraging
        }
    }

    Explanation {
        visible: !root.modeCompatible
        text: qsTr("The selected base mode is not supported by this receiver. Choose a supported mode explicitly.")
    }

    Explanation {
        visible: root.presentation.receiverAveraging && root.baseMode === BaseModeDefinition.BaseReceiverAveraging
        text: qsTr("The receiver averages its position for up to the maximum time. This is not accuracy-controlled survey-in and does not guarantee a position accuracy.")
    }

    SettingField {
        label: qsTr("Maximum averaging time")
        fact: root.settings.receiverAveragingDuration
        visible: root.presentation.receiverAveraging && root.baseMode === BaseModeDefinition.BaseReceiverAveraging
        enabled: root._editable
    }

    FactSlider {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        label: root.presentation.observationAccuracyFilter ? qsTr("Observation accuracy limit") : qsTr("Accuracy")
        fact: root.settings.surveyInAccuracyLimit
        majorTickStepSize: 0.1
        enabled: root._editable
        visible: root.baseMode === BaseModeDefinition.BaseSurveyIn
                 && root.settings.surveyInAccuracyLimit.userVisible && root.presentation.surveyAccuracy
    }

    FactSlider {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        label: root.presentation.acceptedObservationTime ? qsTr("Accepted observation time") : qsTr("Min Duration")
        fact: root.settings.surveyInMinObservationDuration
        majorTickStepSize: 10
        enabled: root._editable
        visible: root.baseMode === BaseModeDefinition.BaseSurveyIn
                 && root.settings.surveyInMinObservationDuration.userVisible && root.presentation.surveyDuration
    }

    SettingField {
        fact: root.settings.fixedBasePositionLatitude
        enabled: root._editable
        visible: root.baseMode === BaseModeDefinition.BaseFixed && root.presentation.rtkBase
    }
    SettingField {
        fact: root.settings.fixedBasePositionLongitude
        enabled: root._editable
        visible: root.baseMode === BaseModeDefinition.BaseFixed && root.presentation.rtkBase
    }
    SettingField {
        fact: root.settings.fixedBasePositionAltitude
        enabled: root._editable
        visible: root.baseMode === BaseModeDefinition.BaseFixed && root.presentation.rtkBase
    }
    SettingField {
        fact: root.settings.fixedBasePositionAccuracy
        enabled: root._editable
        visible: root.baseMode === BaseModeDefinition.BaseFixed && root.presentation.fixedBaseAccuracy
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        visible: root.presentation.rtkBase
        Explanation {
            text: qsTr("Save the current base position for a later fixed-position connection. This does not change the running receiver's mode.")
        }
        QGCButton {
            objectName: "rtkSaveBasePosition"
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            wrapMode: Text.Wrap
            focusPolicy: Qt.StrongFocus
            text: root.baseFacts.canSaveCurrentBasePosition ? qsTr("Save Current Base Position")
                  : !root.baseFacts.valid.rawValue ? qsTr("Not Yet Valid")
                  : !Number.isFinite(root.baseFacts.currentAccuracy.rawValue)
                    || root.baseFacts.currentAccuracy.rawValue < 0 ? qsTr("Accuracy Unavailable")
                  : qsTr("Invalid Base Position")
            enabled: root.baseFacts.canSaveCurrentBasePosition
            onClicked: root.saveCurrentBasePosition()
        }
    }

    QGCCheckBox {
        id: persistenceCheckbox
        objectName: "rtkPersistentChangesCheckBox"
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        text: qsTr("Allow flash save and restart")
        focusPolicy: Qt.StrongFocus
        visible: root.receiver.serialSupported && root.presentation.persistentConfiguration
        enabled: root._editable
        checked: root.consent.allowed
        onClicked: root.consent.allowed = checked
        contentItem: QGCLabel {
            text: persistenceCheckbox.text
            color: persistenceCheckbox.textColor
            leftPadding: persistenceCheckbox.indicator.width + persistenceCheckbox.spacing
            wrapMode: Text.Wrap
        }
    }

    Explanation {
        objectName: "rtkPersistentConsentWarning"
        visible: root.receiver.serialSupported && root.presentation.persistentConfiguration
        text: qsTr("For this connection only, allow QGroundControl to write requested role or base-setting changes to receiver flash and restart it. Changes may remain saved even if reconnecting fails. No factory reset is performed. Permission is cleared after each attempt and is never used by auto-connect.")
    }

    QGCButton {
        objectName: "rtkConnectButton"
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        wrapMode: Text.Wrap
        focusPolicy: Qt.StrongFocus
        text: root.receiver.hasReceiver ? qsTr("Disconnect") : qsTr("Connect")
        visible: root.receiver.serialSupported
        enabled: root.receiver.hasReceiver || (root.presentation.specificReceiver && root.modeCompatible)
        onClicked: {
            if (root.receiver.hasReceiver) {
                root.clearConsent()
                root.receiver.disconnectConfiguredGPS()
            } else {
                root.connectSelectedReceiver()
            }
        }
    }

    Connections {
        target: root.receiver
        function onReceiverChanged() { root.clearConsent() }
    }
    Connections {
        target: root.settings.serialDevice
        function onRawValueChanged() { root.clearConsent() }
    }
    Connections {
        target: root.settings.serialBaudRate
        function onRawValueChanged() { root.clearConsent() }
    }
}
