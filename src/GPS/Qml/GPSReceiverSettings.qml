pragma ComponentBehavior: Bound

import QtQuick

import QGroundControl.GPS

/// The GNSS receiver settings as one form: every group of them under one heading, for hosts that show them whole.
GPSReceiverSettingsGroup {
    id: root

    property alias serialPorts: connection.serialPorts
    property alias serialBaudRates: connection.serialBaudRates
    property alias showErrorMessage: connectControls.showErrorMessage

    heading: qsTr("GNSS Receiver")

    GPSReceiverConnection {
        id: connection
        receiver: root.receiver
        settings: root.settings
        autoConnectFact: root.autoConnectFact
        heading: ""
        showBorder: false
    }

    GPSReceiverBaseStation {
        receiver: root.receiver
        settings: root.settings
        heading: ""
        showBorder: false
    }

    GPSReceiverBaseSites {
        receiver: root.receiver
        settings: root.settings
        heading: ""
        showBorder: false
    }

    GPSReceiverConnectControls {
        id: connectControls
        receiver: root.receiver
        settings: root.settings
        autoConnectFact: root.autoConnectFact
        heading: ""
        showBorder: false
    }
}
