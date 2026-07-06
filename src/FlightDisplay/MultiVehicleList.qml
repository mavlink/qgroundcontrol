/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Controls
import QGroundControl.Palette
import QGroundControl.Vehicle
import QGroundControl.FlightMap
import QGroundControl.FlightDisplay

Item {
    property real   _margin:              ScreenTools.defaultFontPixelWidth * 0.42
    property real   _widgetHeight:        ScreenTools.defaultFontPixelHeight * 1.72
    property var    _guidedController:    globals.guidedControllerFlyView
    property var    _activeVehicleColor:  qgcPal.primaryButton
    property var    _activeVehicle:       QGroundControl.multiVehicleManager.activeVehicle
    property var    selectedVehicles:     QGroundControl.multiVehicleManager.selectedVehicles

    implicitHeight: vehicleList.contentHeight

    QGCPalette { id: qgcPal }

    function armAvailable() {
        for (var i = 0; i < selectedVehicles.count; i++) {
            var vehicle = selectedVehicles.get(i)
            if (vehicle.armed === false) {
                return true
            }
        }
        return false
    }


    function disarmAvailable() {
        for (var i = 0; i < selectedVehicles.count; i++) {
            var vehicle = selectedVehicles.get(i)
            if (vehicle.armed === true) {
                return true
            }
        }
        return false
    }

    function startAvailable() {
        for (var i = 0; i < selectedVehicles.count; i++) {
            var vehicle = selectedVehicles.get(i)
            if (vehicle.armed === true && vehicle.flightMode !== vehicle.missionFlightMode){
                return true
            }
        }
        return false
    }

    function pauseAvailable() {
        for (var i = 0; i < selectedVehicles.count; i++) {
            var vehicle = selectedVehicles.get(i)
            if (vehicle.armed === true && vehicle.pauseVehicleSupported) {
                return true
            }
        }
        return false
    }

    function selectVehicle(vehicleId) {
        QGroundControl.multiVehicleManager.selectVehicle(vehicleId)
    }

    function deselectVehicle(vehicleId) {
        QGroundControl.multiVehicleManager.deselectVehicle(vehicleId)
    }

    function toggleSelect(vehicleId) {
        if (!vehicleSelected(vehicleId)) {
            selectVehicle(vehicleId)
        } else {
            deselectVehicle(vehicleId)
        }
    }

    function selectAll() {
        var vehicles = QGroundControl.multiVehicleManager.vehicles
        for (var i = 0; i < vehicles.count; i++) {
            var vehicle = vehicles.get(i)
            var vehicleId = vehicle.id
            if (!vehicleSelected(vehicleId)) {
                selectVehicle(vehicleId)
            }
        }
    }

    function deselectAll() {
        QGroundControl.multiVehicleManager.deselectAllVehicles()
    }

    function vehicleSelected(vehicleId) {
        for (var i = 0; i < selectedVehicles.count; i++ ) {
            var currentId = selectedVehicles.get(i).id
            if (vehicleId === currentId) {
                return true
            }
        }
        return false
    }

    function factText(fact, fallbackText) {
        if (!fact || fact.valueString === undefined) {
            return fallbackText
        }
        return fact.valueString + (fact.units && fact.units !== "" ? " " + fact.units : "")
    }

    function vehicleKindText(vehicle) {
        if (!vehicle) {
            return qsTr("Unknown")
        }
        if (vehicle.vehicleTypeString && vehicle.vehicleTypeString !== "" && vehicle.vehicleTypeString !== "MAV_TYPE_UNKNOWN") {
            return vehicle.vehicleTypeString
        }
        return qsTr("Unknown")
    }

    function vehicleDisplayName(vehicle) {
        return vehicle ? vehicleKindText(vehicle) + " #" + vehicle.id : qsTr("Unknown")
    }

    QGCListView {
        id:                 vehicleList
        anchors.left:       parent.left
        anchors.right:      parent.right
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        spacing:            ScreenTools.defaultFontPixelHeight * 0.34
        orientation:        ListView.Vertical
        model:              QGroundControl.multiVehicleManager.vehicles
        cacheBuffer:        _cacheBuffer < 0 ? 0 : _cacheBuffer
        clip:               true

        property real _cacheBuffer:     height * 2

        delegate: Rectangle {
            width:          vehicleList.width
            height:         Math.max(ScreenTools.minTouchPixels * 0.92, innerRow.implicitHeight + _margin * 1.65)
            color:          QGroundControl.multiVehicleManager.activeVehicle == _vehicle ? Qt.rgba(0.115, 0.120, 0.130, 0.82) : Qt.rgba(0.070, 0.073, 0.078, 0.58)
            radius:         Math.round(ScreenTools.defaultFontPixelWidth * 0.65)
            border.width:   _vehicle && vehicleSelected(_vehicle.id) ? 1 : 0
            border.color:   QGroundControl.multiVehicleManager.activeVehicle == _vehicle ? qgcPal.primaryButton : Qt.rgba(0.82, 0.88, 0.94, 0.12)

            property var    _vehicle:   object

            QGCMouseArea {
                anchors.fill:       parent
                onClicked:          toggleSelect(_vehicle.id)
            }

            RowLayout {
                id:                         innerRow
                anchors.left:               parent.left
                anchors.right:              parent.right
                anchors.verticalCenter:     parent.verticalCenter
                anchors.leftMargin:         _margin
                anchors.rightMargin:        _margin
                spacing:                    _margin

                IntegratedCompassAttitude {
                    id: compassWidget
                    Layout.alignment:           Qt.AlignVCenter
                    compassRadius:              _widgetHeight / 2 - attitudeSize / 2
                    compassBorder:              0
                    attitudeSize:               ScreenTools.defaultFontPixelWidth / 2
                    attitudeSpacing:            attitudeSize / 2
                    usedByMultipleVehicleList:  true
                    vehicle:                    _vehicle
                }

                ColumnLayout {
                    spacing:              ScreenTools.defaultFontPixelHeight * 0.10
                    Layout.fillWidth:     true
                    Layout.alignment:     Qt.AlignVCenter

                    RowLayout {
                        Layout.fillWidth: true
                        spacing:          ScreenTools.defaultFontPixelWidth * 0.45

                        Rectangle {
                            Layout.alignment:   Qt.AlignVCenter
                            width:              ScreenTools.defaultFontPixelHeight * 0.58
                            height:             width
                            radius:             width / 2
                            color:              QGroundControl.multiVehicleManager.activeVehicle == _vehicle ? qgcPal.primaryButton : qgcPal.buttonBorder
                        }

                        QGCLabel {
                            text:                 vehicleDisplayName(_vehicle)
                            font.pointSize:       ScreenTools.defaultFontPointSize
                            font.bold:            true
                            color:                qgcPal.text
                            Layout.fillWidth:     true
                            elide:                Text.ElideRight
                        }
                    }

                    FlightModeMenu {
                        Layout.alignment:     Qt.AlignLeft
                        font.pointSize:       ScreenTools.smallFontPointSize
                        color:                qgcPal.text
                        currentVehicle:       _vehicle
                    }
                }

                Rectangle {
                    Layout.alignment:       Qt.AlignVCenter
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 8.7
                    Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.82
                    radius:                 Math.round(ScreenTools.defaultFontPixelWidth * 0.45)
                    color:                  Qt.rgba(0.045, 0.048, 0.052, 0.58)
                    border.color:           Qt.rgba(0.82, 0.88, 0.94, 0.12)
                    border.width:           1

                    Column {
                        anchors.centerIn:   parent
                        spacing:            0

                        QGCLabel {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text:                     factText(_vehicle ? _vehicle.altitudeRelative : null, qsTr("N/A"))
                            color:                    qgcPal.text
                            font.bold:                true
                            font.pointSize:           ScreenTools.defaultFontPointSize
                        }

                        QGCLabel {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text:                     factText(_vehicle ? _vehicle.groundSpeed : null, qsTr("N/A"))
                            color:                    qgcPal.buttonText
                            font.pointSize:           ScreenTools.smallFontPointSize
                        }
                    }
                }
            }
        }
    }
}
