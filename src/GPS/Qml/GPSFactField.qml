import QtQuick

import QGroundControl

/// A GPSField for a Fact: labelled with it, and hidden while its setting is.
GPSField {
    required property Fact fact
    /// Shows the field when the setting itself is visible.
    property bool shown: true
    readonly property SettingsFact _setting: fact as SettingsFact

    label: fact.label
    visible: shown && (!_setting || _setting.userVisible)
}
