import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SettingsGroupLayout {
    id: root

    required property string heading
    required property var gps

    // Outer visibility already gates on gps + at least one resilience state
    // being present, so inner rows can assume root.gps is non-null.
    visible:      gps && _hasResilienceData(gps)
    showDividers: true

    function _hasResilienceData(gps) {
        if (!gps) return false
        var jam  = gps.jammingState.value
        var spf  = gps.spoofingState.value
        var auth = gps.authenticationState.value
        return (jam  > VehicleGPSFactGroup.JammingUnknown  && jam  !== VehicleGPSFactGroup.JammingInvalid)
            || (spf  > VehicleGPSFactGroup.SpoofingUnknown && spf  !== VehicleGPSFactGroup.SpoofingInvalid)
            || (auth > VehicleGPSFactGroup.AuthUnknown     && auth !== VehicleGPSFactGroup.AuthInvalid)
    }

    function _isVisible(val, unknownVal, invalidVal) {
        return val > unknownVal && val !== invalidVal
    }

    property string _na: qsTr("N/A")

    LabelledLabel {
        label:     qsTr("Jamming")
        labelText: root.gps ? (root.gps.jammingState.enumStringValue || root._na) : root._na
        visible:   root.gps && root._isVisible(root.gps.jammingState.value,
                                               VehicleGPSFactGroup.JammingUnknown, VehicleGPSFactGroup.JammingInvalid)
    }
    LabelledLabel {
        label:     qsTr("Spoofing")
        labelText: root.gps ? (root.gps.spoofingState.enumStringValue || root._na) : root._na
        visible:   root.gps && root._isVisible(root.gps.spoofingState.value,
                                               VehicleGPSFactGroup.SpoofingUnknown, VehicleGPSFactGroup.SpoofingInvalid)
    }
    LabelledLabel {
        label:     qsTr("Authentication")
        labelText: root.gps ? (root.gps.authenticationState.enumStringValue || root._na) : root._na
        visible:   root.gps && root._isVisible(root.gps.authenticationState.value,
                                               VehicleGPSFactGroup.AuthUnknown, VehicleGPSFactGroup.AuthInvalid)
    }
}
