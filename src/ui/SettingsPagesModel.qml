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
        url: "/qml/GeneralSettings.qml"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Fly View")
        url: "/qml/FlyViewSettings.qml"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Plan View")
        url: "/qml/PlanViewSettings.qml"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Video")
        url: "/qml/VideoSettings.qml"
        pageVisible: function() { return QGroundControl.settingsManager.videoSettings.visible }
    }

    ListElement {
        name: qsTr("Telemetry")
        url: "/qml/TelemetrySettings.qml"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("ADSB Server")
        url: "/qml/ADSBServerSettings.qml"
        pageVisible: function() { return true }
    }

    //ListElement {
    //    name: qsTr("General Old")
    //    url: "/qml/GeneralSettings2.qml"
    //    pageVisible: function() { return true }
    //}

    ListElement {
        name: qsTr("Comm Links")
        url: "/qml/LinkSettings.qml"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Offline Maps")
        url: "/qml/OfflineMap.qml"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("PX4 Log Transfer")
        url: "/qml/PX4LogTransferSettings.qml"
        pageVisible: function() { 
            var activeVehicle = QGroundControl.multiVehicleManager.activeVehicle
            return QGroundControl.corePlugin.options.showPX4LogTransferOptions && 
                        QGroundControl.px4ProFirmwareSupported && 
                        (activeVehicle ? activeVehicle.px4Firmware : true)
        }
    }

    ListElement {
        name: qsTr("Remote ID")
        url: "/qml/RemoteIDSettings.qml"
        pageVisible: function() { return QGroundControl.settingsManager.remoteIDSettings.enable.rawValue }
    }

    ListElement {
        name: qsTr("Console")
        url: "/qml/QGroundControl/Controls/AppMessages.qml"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Help")
        url: "/qml/HelpSettings.qml"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Mock Link")
        url: "/qml/MockLink.qml"
        pageVisible: function() { return ScreenTools.isDebug }
    }

    ListElement {
        name: qsTr("Debug")
        url: "/qml/DebugWindow.qml"
        pageVisible: function() { return ScreenTools.isDebug }
    }

    ListElement {
        name: qsTr("Palette Test")
        url: "/qml/QmlTest.qml"
        pageVisible: function() { return ScreenTools.isDebug }
    }
}

