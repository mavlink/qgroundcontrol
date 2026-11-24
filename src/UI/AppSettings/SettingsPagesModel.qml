/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQml.Models

import QGroundControl
import QGroundControl.ScreenTools

ListModel {
    ListElement {
        name: qsTr("General")
        category: ""
        url: "qrc:/qml/QGroundControl/AppSettings/GeneralSettings.qml"
        iconUrl: "qrc:/res/QGCLogoWhite.svg"
        badgeColor: "#6B7280"
        pageVisible: function() { return true }
    }

    // --- Flight Section ---
    ListElement {
        name: qsTr("Fly View")
        category: "Flight"
        url: "qrc:/qml/QGroundControl/AppSettings/FlyViewSettings.qml"
        iconUrl: "qrc:/qmlimages/PaperPlane.svg"
        badgeColor: "#1E88E5"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Plan View")
        category: "Flight"
        url: "qrc:/qml/QGroundControl/AppSettings/PlanViewSettings.qml"
        iconUrl: "qrc:/qmlimages/Plan.svg"
        badgeColor: "#26A69A"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Video")
        category: "Flight"
        url: "qrc:/qml/QGroundControl/AppSettings/VideoSettings.qml"
        iconUrl: "qrc:/InstrumentValueIcons/camera.svg"
        badgeColor: "#FB8C00"
        pageVisible: function() { return QGroundControl.settingsManager.videoSettings.visible }
    }

    ListElement {
        name: qsTr("Telemetry")
        category: "Flight"
        url: "qrc:/qml/QGroundControl/AppSettings/TelemetrySettings.qml"
        iconUrl: "qrc:/InstrumentValueIcons/drone.svg"
        badgeColor: "#AB47BC"
        pageVisible: function() { return true }
    }

    // --- Connections Section ---
    ListElement {
        name: qsTr("ADSB Server")
        category: "Connections"
        url: "qrc:/qml/QGroundControl/AppSettings/ADSBServerSettings.qml"
        iconUrl: "qrc:/InstrumentValueIcons/airplane.svg"
        badgeColor: "#5C6BC0"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Comm Links")
        category: "Connections"
        url: "qrc:/qml/QGroundControl/AppSettings/LinkSettings.qml"
        iconUrl: "qrc:/InstrumentValueIcons/usb.svg"
        badgeColor: "#43A047"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Maps")
        category: "Connections"
        url: "qrc:/qml/QGroundControl/AppSettings/MapSettings.qml"
        iconUrl: "qrc:/InstrumentValueIcons/globe.svg"
        badgeColor: "#0288D1"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("PX4 Log Transfer")
        category: "Connections"
        url: "qrc:/qml/QGroundControl/AppSettings/PX4LogTransferSettings.qml"
        iconUrl: "qrc:/InstrumentValueIcons/inbox-download.svg"
        badgeColor: "#E53935"
        pageVisible: function() { 
            var activeVehicle = QGroundControl.multiVehicleManager.activeVehicle
            return QGroundControl.corePlugin.options.showPX4LogTransferOptions && 
                        QGroundControl.px4ProFirmwareSupported && 
                        (activeVehicle ? activeVehicle.px4Firmware : true)
        }
    }

    ListElement {
        name: qsTr("Remote ID")
        category: "Connections"
        url: "qrc:/qml/QGroundControl/AppSettings/RemoteIDSettings.qml"
        iconUrl: "qrc:/qmlimages/RidIconManNoID.svg"
        badgeColor: "#78909C"
        pageVisible: function() { return true }
    }

    // --- Developer Section ---
    ListElement {
        name: qsTr("Console")
        category: "Developer"
        url: "qrc:/qml/QGroundControl/Controls/AppMessages.qml"
        iconUrl: "qrc:/InstrumentValueIcons/conversation.svg"
        badgeColor: "#374151"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Help")
        category: "Developer"
        url: "qrc:/qml/QGroundControl/AppSettings/HelpSettings.qml"
        iconUrl: "qrc:/InstrumentValueIcons/question.svg"
        badgeColor: "#29B6F6"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Mock Link")
        category: "Developer"
        url: "qrc:/qml/QGroundControl/AppSettings/MockLink.qml"
        iconUrl: "qrc:/InstrumentValueIcons/drone.svg"
        badgeColor: "#FF9800"
        pageVisible: function() { return ScreenTools.isDebug }
    }

    ListElement {
        name: qsTr("Debug")
        category: "Developer"
        url: "qrc:/qml/QGroundControl/AppSettings/DebugWindow.qml"
        iconUrl: "qrc:/InstrumentValueIcons/bug.svg"
        badgeColor: "#F44336"
        pageVisible: function() { return ScreenTools.isDebug }
    }

    ListElement {
        name: qsTr("Palette Test")
        category: "Developer"
        url: "qrc:/qml/QGroundControl/AppSettings/QmlTest.qml"
        iconUrl: "qrc:/InstrumentValueIcons/photo.svg"
        badgeColor: "#8E24AA"
        pageVisible: function() { return ScreenTools.isDebug }
    }
}
