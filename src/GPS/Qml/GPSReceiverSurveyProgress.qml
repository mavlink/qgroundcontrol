pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.GPS

/// How far a surveying base has got, as its receiver family reports it, and the survey's duration and accuracy once it
/// is done. A receiver that measures its survey (u-blox, Quectel) reports its progress: the time, or the observations
/// it accepted, is shown against the configured minimum with a bar, and the accuracy against the limit a u-blox survey
/// has to reach. A receiver that averages (Trimble, Septentrio, Femtomes) only reports when it is done: its time is
/// the connection's own count since the averaging started, which gets no bar.
ColumnLayout {
    id: root

    property GPSReceiver receiver: QGroundControl.gpsManager.receiver
    /// The receiver's status Facts.
    property GPSReceiverFactGroup facts: root.receiver.facts

    readonly property gpsReceiverPresentation _presentation: root.receiver.activePresentation
    readonly property bool _surveyBase: root.receiver.state === GPSReceiver.Connected
                                        && root.receiver.activeBaseMode === BaseModeDefinition.BaseSurveyIn
    readonly property bool _surveying: root.facts.baseState.value === GPSReceiverFactGroup.BaseState.Active
    /// The receiver reports the progress of its survey; one that averages reports only that it is done.
    readonly property bool _measured: root._presentation.surveyAccuracy
    readonly property bool _durationShown: root._surveyBase && root._presentation.reportsSurveyDuration
    readonly property bool _accuracyShown: root._surveyBase && root.facts.currentAccuracy.value > 0
    /// The family takes the configured time: the least a survey lasts, or how long an averaging does.
    readonly property bool _durationTargeted: root._surveying && root._presentation.surveyDuration
                                              && root.facts.targetDuration.rawValue > 0
    /// The survey ends once its accuracy is within the limit. A Quectel receiver applies the limit to each
    /// observation instead, so its survey's accuracy has none to reach.
    readonly property bool _accuracyTargeted: root._surveying && root._measured
                                              && !root._presentation.observationAccuracyFilter
                                              && root.facts.targetAccuracy.rawValue > 0

    /// "95 of 180 s": @a fact against @a target, which has the same unit.
    function _progressText(fact, target) {
        //: %1 is what a survey has reached and %2 what it has to reach, both in the unit %3: "95 of 180 s"
        return qsTr("%1 of %2 %3").arg(fact.valueString).arg(target.valueString).arg(fact.units)
    }

    function _valueText(fact) {
        return fact.valueString + " " + fact.units
    }

    // The bars take the width the rows have, and never widen the panel.
    spacing: ScreenTools.defaultFontPixelHeight / 4
    visible: root._durationShown || root._accuracyShown

    LabelledLabel {
        objectName: "rtkSurveyDuration"
        visible: root._durationShown
        label: root._presentation.acceptedObservationTime ? qsTr("Accepted observation time")
               : root._measured ? qsTr("Duration")
               //: Label of the time a base station receiver has been averaging its position for
               : qsTr("Averaging")
        labelText: root._durationTargeted ? root._progressText(root.facts.currentDuration, root.facts.targetDuration)
                                          : root._valueText(root.facts.currentDuration)
        labelTextFormat: Text.PlainText
    }

    ProgressBar {
        objectName: "rtkSurveyDurationProgress"
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredWidth: 0
        visible: root._durationShown && root._durationTargeted && root._measured
        value: Math.min(root.facts.currentDuration.rawValue / root.facts.targetDuration.rawValue, 1)
    }

    LabelledLabel {
        objectName: "rtkSurveyAccuracy"
        visible: root._accuracyShown
        label: root.facts.baseState.value === GPSReceiverFactGroup.BaseState.Valid ? qsTr("Accuracy")
                                                                                   : qsTr("Current Accuracy")
        labelText: root._accuracyTargeted ? root._progressText(root.facts.currentAccuracy, root.facts.targetAccuracy)
                                          : root._valueText(root.facts.currentAccuracy)
        labelTextFormat: Text.PlainText
    }

    // The accuracy improves towards the limit, so the bar fills as the limit's share of the accuracy grows.
    ProgressBar {
        objectName: "rtkSurveyAccuracyProgress"
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredWidth: 0
        visible: root._accuracyShown && root._accuracyTargeted
        value: Math.min(root.facts.targetAccuracy.rawValue / root.facts.currentAccuracy.rawValue, 1)
    }
}
