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
        iconUrl: "qrc:/res/QGCLogoWhite.svg"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Fly View")
        url: "/qml/FlyViewSettings.qml"
        iconUrl: "qrc:/qmlimages/PaperPlane.svg"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Plan View")
        url: "/qml/PlanViewSettings.qml"
        iconUrl: "qrc:/qmlimages/Plan.svg"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Video")
        url: "/qml/VideoSettings.qml"
        iconUrl: "qrc:/InstrumentValueIcons/camera.svg"
        pageVisible: function() { return QGroundControl.settingsManager.videoSettings.visible }
    }

    ListElement {
        name: qsTr("Telemetry")
        url: "/qml/TelemetrySettings.qml"
        iconUrl: "qrc:/InstrumentValueIcons/drone.svg"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("ADSB Server")
        url: "/qml/ADSBServerSettings.qml"
        iconUrl: "qrc:/InstrumentValueIcons/airplane.svg"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Comm Links")
        url: "/qml/LinkSettings.qml"
        iconUrl: "qrc:/InstrumentValueIcons/usb.svg"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Maps")
        url: "/qml/MapSettings.qml"
        iconUrl: "qrc:/InstrumentValueIcons/globe.svg"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("PX4 Log Transfer")
        url: "/qml/PX4LogTransferSettings.qml"
        iconUrl: "qrc:/InstrumentValueIcons/inbox-download.svg"
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
        iconUrl: "qrc:/qmlimages/RidIconManNoID.svg"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Console")
        url: "/qml/QGroundControl/Controls/AppMessages.qml"
        iconUrl: "qrc:/InstrumentValueIcons/conversation.svg"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Help")
        url: "/qml/HelpSettings.qml"
        iconUrl: "qrc:/InstrumentValueIcons/question.svg"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Mock Link")
        url: "/qml/MockLink.qml"
        iconUrl: "qrc:/InstrumentValueIcons/drone.svg"
        pageVisible: function() { return ScreenTools.isDebug }
    }

    ListElement {
        name: qsTr("Debug")
        url: "/qml/DebugWindow.qml"
        iconUrl: "qrc:/InstrumentValueIcons/bug.svg"
        pageVisible: function() { return ScreenTools.isDebug }
    }

    ListElement {
        name: qsTr("Palette Test")
        url: "/qml/QmlTest.qml"
        iconUrl: "qrc:/InstrumentValueIcons/photo.svg"
        pageVisible: function() { return ScreenTools.isDebug }
    }
}

