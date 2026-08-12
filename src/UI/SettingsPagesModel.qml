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
        name: qsTr("Volador Login")
        url: "qrc:/qml/VoladorLoginView.qml"
        iconUrl: "qrc:/InstrumentValueIcons/lock.svg"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Volador Dashboard")
        url: "qrc:/qml/VoladorDashboardView.qml"
        iconUrl: "qrc:/InstrumentValueIcons/drone.svg"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Volador Platform")
        url: "qrc:/qml/VoladorPlatform.qml"
        iconUrl: "qrc:/Volador/Assets/Logos/volador_compact.png"
        pageVisible: function() { return true }
    }


    ListElement {
        name: qsTr("Fleet Management")
        url: "qrc:/qml/FleetManagement.qml"
        iconUrl: "qrc:/qmlimages/Quad.svg"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("General")
        url: "qrc:/qml/GeneralSettings.qml"
        iconUrl: "qrc:/Volador/Assets/Logos/volador_compact.png"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Fly View")
        url: "qrc:/qml/FlyViewSettings.qml"
        iconUrl: "qrc:/qmlimages/PaperPlane.svg"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Plan View")
        url: "qrc:/qml/PlanViewSettings.qml"
        iconUrl: "qrc:/qmlimages/Plan.svg"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Video")
        url: "qrc:/qml/VideoSettings.qml"
        iconUrl: "qrc:/InstrumentValueIcons/camera.svg"
        pageVisible: function() { return QGroundControl.settingsManager.videoSettings.visible }
    }

    ListElement {
        name: qsTr("Telemetry")
        url: "qrc:/qml/TelemetrySettings.qml"
        iconUrl: "qrc:/InstrumentValueIcons/drone.svg"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("ADSB Server")
        url: "qrc:/qml/ADSBServerSettings.qml"
        iconUrl: "qrc:/InstrumentValueIcons/airplane.svg"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Comm Links")
        url: "qrc:/qml/LinkSettings.qml"
        iconUrl: "qrc:/InstrumentValueIcons/usb.svg"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Maps")
        url: "qrc:/qml/MapSettings.qml"
        iconUrl: "qrc:/InstrumentValueIcons/globe.svg"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("PX4 Log Transfer")
        url: "qrc:/qml/PX4LogTransferSettings.qml"
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
        url: "qrc:/qml/RemoteIDSettings.qml"
        iconUrl: "qrc:/qmlimages/RidIconManNoID.svg"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Console")
        url: "qrc:/qml/QGroundControl/Controls/AppMessages.qml"
        iconUrl: "qrc:/InstrumentValueIcons/conversation.svg"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Help")
        url: "qrc:/qml/HelpSettings.qml"
        iconUrl: "qrc:/InstrumentValueIcons/question.svg"
        pageVisible: function() { return true }
    }

    ListElement {
        name: qsTr("Mock Link")
        url: "qrc:/qml/MockLink.qml"
        iconUrl: "qrc:/InstrumentValueIcons/drone.svg"
        pageVisible: function() { return ScreenTools.isDebug }
    }

    ListElement {
        name: qsTr("Debug")
        url: "qrc:/qml/DebugWindow.qml"
        iconUrl: "qrc:/InstrumentValueIcons/bug.svg"
        pageVisible: function() { return ScreenTools.isDebug }
    }

    ListElement {
        name: qsTr("Palette Test")
        url: "qrc:/qml/QmlTest.qml"
        iconUrl: "qrc:/InstrumentValueIcons/photo.svg"
        pageVisible: function() { return ScreenTools.isDebug }
    }
}

