pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.GPS

/// The position a configured base sends its corrections for, once its receiver reports it valid: what a survey or the
/// receiver's own averaging found, or the fixed position the receiver was given. A receiver that reports no height,
/// as a Trimble receiver does after its averaging, shows none.
ColumnLayout {
    id: root

    property GPSReceiver receiver: QGroundControl.gpsManager.receiver
    /// The receiver's status Facts.
    property GPSReceiverFactGroup facts: root.receiver.facts

    spacing: ScreenTools.defaultFontPixelHeight / 4
    visible: root.receiver.state === GPSReceiver.Connected
             && root.facts.baseState.value === GPSReceiverFactGroup.BaseState.Valid
             && Number.isFinite(root.facts.baseLatitude.rawValue) && Number.isFinite(root.facts.baseLongitude.rawValue)

    LabelledFactLabel {
        objectName: "rtkBaseLatitude"
        fact: root.facts.baseLatitude
        factLabel.textFormat: Text.PlainText
    }

    LabelledFactLabel {
        objectName: "rtkBaseLongitude"
        fact: root.facts.baseLongitude
        factLabel.textFormat: Text.PlainText
    }

    LabelledFactLabel {
        objectName: "rtkBaseHeight"
        visible: Number.isFinite(root.facts.baseHeight.rawValue)
        fact: root.facts.baseHeight
        factLabel.textFormat: Text.PlainText
    }
}
