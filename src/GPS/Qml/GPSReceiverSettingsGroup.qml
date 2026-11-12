import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

/// A group of the GNSS receiver settings: the receiver, its saved settings and what the selected receiver offers,
/// which the controls of every group follow. The settings page shows the groups one below the other.
QGCGroupBox {
    property GPSReceiver receiver: QGroundControl.gpsManager.receiver
    property RTKSettings settings: QGroundControl.settingsManager.rtkSettings
    property SettingsFact autoConnectFact: settings.autoConnect as SettingsFact

    readonly property int role: settings.receiverRole.rawValue
    readonly property bool configuredBase: role === RTKSettings.ConfiguredBase
    readonly property int baseMode: settings.baseMode.rawValue
    readonly property gpsReceiverPresentation presentation: receiver.configuredPresentation
    /// What the notes of one receiver family follow: the selected manufacturer or, with automatic detection, the one
    /// that was identified for the connection that runs. Until then it offers what no single receiver does, which
    /// _familyKnown tells.
    readonly property gpsReceiverPresentation _family: presentation.automatic ? receiver.activePresentation
                                                                             : presentation
    readonly property bool _familyKnown: !_family.automatic
    // A pending automatic reconnect keeps the saved connection; stop it before editing.
    readonly property bool _active: receiver.state !== GPSReceiver.Disconnected
    readonly property bool _editable: !_active
}
