pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

/// How a configured base finds its position: the base mode and that mode's parameters.
GPSReceiverSettingsGroup {
    id: root

    readonly property list<string> _baseModeNames: settings.baseMode.enumStrings

    heading: qsTr("RTK Base Station")
    // A passive receiver has no base settings. A manufacturer no receiver family has supports no base mode, which
    // the note on the unsupported mode says.
    visible: root.presentation.rtkBase || !root.receiver.baseModeSupported

    component ModeButton: QGCRadioButton {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        focusPolicy: Qt.StrongFocus
        wrapMode: Text.Wrap
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
            onClicked: root.settings.baseMode.rawValue = BaseModeDefinition.BaseSurveyIn
            visible: root.presentation.surveyIn
        }
        ModeButton {
            objectName: "rtkFixedMode"
            text: root._baseModeNames[BaseModeDefinition.BaseFixed]
            checked: root.baseMode === BaseModeDefinition.BaseFixed
            onClicked: root.settings.baseMode.rawValue = BaseModeDefinition.BaseFixed
        }
        ModeButton {
            text: root._baseModeNames[BaseModeDefinition.BaseReceiverAveraging]
            checked: root.baseMode === BaseModeDefinition.BaseReceiverAveraging
            onClicked: root.settings.baseMode.rawValue = BaseModeDefinition.BaseReceiverAveraging
            visible: root.presentation.receiverAveraging
        }
    }

    GPSNoteLabel {
        visible: !root.receiver.baseModeSupported
        text: qsTr("The selected base mode is not supported by this receiver. Choose a supported mode explicitly.")
    }

    GPSFactTextField {
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

        GPSFactTextField { fact: root.settings.fixedBasePositionLatitude }
        GPSFactTextField { fact: root.settings.fixedBasePositionLongitude }
        GPSFactTextField { fact: root.settings.fixedBasePositionAltitude }
        GPSFactTextField {
            fact: root.settings.fixedBasePositionAccuracy
            shown: root.presentation.fixedBaseAccuracy
        }
    }
}
