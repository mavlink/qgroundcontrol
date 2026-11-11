import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

/// A group of the GNSS receiver settings: the receiver, its saved settings and what the selected receiver offers,
/// which the controls of every group follow. GPSReceiverSettings shows the groups as one form.
QGCGroupBox {
    property GPSReceiver receiver: QGroundControl.gpsManager.receiver
    property RTKSettings settings: QGroundControl.settingsManager.rtkSettings
    property SettingsFact autoConnectFact: settings.autoConnect as SettingsFact

    readonly property int role: settings.receiverRole.rawValue
    readonly property bool configuredBase: role === RTKSettings.ConfiguredBase
    readonly property int manufacturer: settings.baseReceiverManufacturer.rawValue
    readonly property int baseMode: settings.baseMode.rawValue
    readonly property gpsReceiverPresentation presentation: receiver.capabilitiesFor(role, manufacturer)
    // A pending automatic reconnect keeps the saved connection; stop it before editing.
    readonly property bool _active: receiver.hasReceiver || receiver.reconnecting
    readonly property bool _editable: !_active
    // Automatic detection words the Quectel notes for a receiver that may not be one.
    readonly property string _quectelOnly: presentation.automatic
                                           ? qsTr("Applies only if a Quectel receiver is identified.") + " " : ""
}
