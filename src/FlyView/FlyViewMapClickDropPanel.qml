/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Guided-action popup for a click on the fly view map (Go to / Orbit / ROI /
/// Set home / Set Estimator Origin / Set Heading). Shared between the
/// QtLocation FlyViewMap and the GeoMap engine adapter: the map indicator
/// properties are optional, engines without those visuals still get the
/// actions (confirmAction works without an indicator).
DropPanel {
    id: root

    property var mapClickCoord

    // Optional map indicators (QtLocation engine only)
    property var gotoIndicator: null
    property var orbitIndicator: null

    readonly property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property var _guidedController: globals.guidedControllerFlyView
    readonly property var _flyViewSettings: QGroundControl.settingsManager.flyViewSettings

    // The available actions were evaluated against the vehicle the panel was
    // opened for; close if that vehicle goes away or the active vehicle changes
    on_ActiveVehicleChanged: close()

    // Callers create the panel dynamically per map click; close() alone would leak it
    onClosed: destroy()

    sourceComponent: Component {
        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelWidth / 2

            QGCButton {
                Layout.fillWidth: true
                text: qsTr("Go to location")
                visible: root._guidedController.showGotoLocation
                onClicked: {
                    root.close()
                    if (!root._activeVehicle) {
                        return
                    }
                    if (root.gotoIndicator) {
                        root.gotoIndicator.show(root.mapClickCoord)
                    }

                    if ((root._activeVehicle.flightMode == root._activeVehicle.gotoFlightMode) && !root._flyViewSettings.goToLocationRequiresConfirmInGuided.value) {
                        const executed = root._guidedController.executeAction(root._guidedController.actionGoto, root.mapClickCoord)
                        if (root.gotoIndicator) {
                            if (executed) {
                                root.gotoIndicator.actionConfirmed() // Still need to call this to commit the new coordinate and radius
                            } else {
                                root.gotoIndicator.actionCancelled()
                            }
                        }
                    } else {
                        root._guidedController.confirmAction(root._guidedController.actionGoto, root.mapClickCoord, root.gotoIndicator)
                    }
                }
            }

            QGCButton {
                Layout.fillWidth: true
                text: qsTr("Orbit at location")
                visible: root._guidedController.showOrbit && (root.orbitIndicator !== null)
                onClicked: {
                    root.close()
                    root.orbitIndicator.show(root.mapClickCoord)
                    root._guidedController.confirmAction(root._guidedController.actionOrbit, root.mapClickCoord, root.orbitIndicator)
                }
            }

            QGCButton {
                objectName: "mapClickROI"
                Layout.fillWidth: true
                text: qsTr("ROI at location")
                visible: root._guidedController.showROI
                onClicked: {
                    root.close()
                    root._guidedController.confirmAction(root._guidedController.actionROI, root.mapClickCoord)
                }
            }

            QGCButton {
                Layout.fillWidth: true
                text: qsTr("Set home here")
                visible: root._guidedController.showSetHome
                onClicked: {
                    root.close()
                    root._guidedController.confirmAction(root._guidedController.actionSetHome, root.mapClickCoord)
                }
            }

            QGCButton {
                Layout.fillWidth: true
                text: qsTr("Set Estimator Origin")
                visible: root._guidedController.showSetEstimatorOrigin
                onClicked: {
                    root.close()
                    root._guidedController.confirmAction(root._guidedController.actionSetEstimatorOrigin, root.mapClickCoord)
                }
            }

            QGCButton {
                Layout.fillWidth: true
                text: qsTr("Set Heading")
                visible: root._guidedController.showChangeHeading
                onClicked: {
                    root.close()
                    root._guidedController.confirmAction(root._guidedController.actionChangeHeading, root.mapClickCoord)
                }
            }

            ColumnLayout {
                spacing: 0
                QGCLabel { text: qsTr("Lat: %1").arg(root.mapClickCoord.latitude.toFixed(6)) }
                QGCLabel { text: qsTr("Lon: %1").arg(root.mapClickCoord.longitude.toFixed(6)) }
            }
        }
    }
}
