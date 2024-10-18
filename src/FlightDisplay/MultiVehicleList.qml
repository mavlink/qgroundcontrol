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

Item {
    property real   _margin:              ScreenTools.defaultFontPixelWidth / 2
    property real   _widgetHeight:        ScreenTools.defaultFontPixelHeight * 2
    property var    _guidedController:    globals.guidedControllerFlyView
    property var    _activeVehicleColor:  "green"
    property var    _activeVehicle:       QGroundControl.multiVehicleManager.activeVehicle

    QGCPalette { id: qgcPal }

    ListModel {
        id: selectedVehiclesModel
    }

    function getSelectedVehicles() {
        var selectedVehicles = [ ]
        forEachSelectedVehicle(function(vehicle) {
            selectedVehicles.push(vehicle)
        })
        return selectedVehicles
    }

    function getVehicleIndex(vehicleId) {
        for (var i = 0; i < selectedVehiclesModel.count; i++) {
            if (selectedVehiclesModel.get(i).id === vehicleId) {
                return i
            }
        }
        return -1
    }

    function forEachSelectedVehicle(action) {
        for (var i = 0; i < selectedVehiclesModel.count; i++) {
            var vehicleId = selectedVehiclesModel.get(i).id
            var vehicle = QGroundControl.multiVehicleManager.getVehicleById(vehicleId)
            action(vehicle)
        }
    }

    function armSelectedVehicles() {
        forEachSelectedVehicle(function(vehicle) {
            vehicle.armed = true
            console.log("vehicle " + vehicle.id + " has been armed.")
        })
    }

    function armAvailable() {
        var available = false
        forEachSelectedVehicle(function(vehicle) {
            if(vehicle.armed === false){
                available =  true
            }
        })
        return available
    }

    function disarmSelectedVehicles() {
        forEachSelectedVehicle(function(vehicle) {
            vehicle.armed = false
            console.log("vehicle " + vehicle.id + " has been disarmed.")
        })
    }

    function disarmAvailable() {
        var available = false
        forEachSelectedVehicle(function(vehicle) {
            if(vehicle.armed === true){
                available = true
            }
        })
        return available
    }

    function startSelectedVehicles() {
        forEachSelectedVehicle(function(vehicle) {
            if (vehicle.armed === true){
                vehicle.startMission()
                console.log("mission started for " + vehicle.id)
            }

        })
    }

    function startAvailable() {
        var available = false
        forEachSelectedVehicle(function(vehicle) {
            if(vehicle.armed === true && vehicle.flightMode !== vehicle.missionFlightMode){
                available = true
            }
        })
        return available
    }

    function pauseSelectedVehicles() {
        forEachSelectedVehicle(function(vehicle) {
            if(vehicle.pauseVehicleSupported){
                vehicle.pauseVehicle()
                console.log("mission paused for " + vehicle.id)
            }
        })
    }
    function pauseAvailable() {
        var available = false
        forEachSelectedVehicle(function(vehicle) {
            if(vehicle.armed === true && vehicle.pauseVehicleSupported){
                available = true
            }
        })
        return available
    }

    function rtlSelectedVehicles() {
        forEachSelectedVehicle(function(vehicle) {
            if (vehicle.armed === true){
                vehicle.flightMode = vehicle.rtlFlightMode
                console.log("vehicle " + vehicle.id + " returning to launch")
            }
        })
    }

    function rtlAvailable(){
        var available = false
        forEachSelectedVehicle(function(vehicle) {
            if(vehicle.armed === true && vehicle.flightMode !== vehicle.rtlFlightMode){
                available = true
            }
        })
        return available
    }

    function takeControlSelectedVehicles() {
        forEachSelectedVehicle(function(vehicle) {
            vehicle.flightMode = vehicle.takeControlFlightMode
            console.log("taking control for " + vehicle.id)
        })
    }

    function takeControlAvailable(){
        var available = false
        forEachSelectedVehicle(function(vehicle) {
            if(vehicle.armed === true && vehicle.flightMode !== vehicle.takeControlFlightMode){
                available = true
            }
        })
        return available
    }

    function activateVehicle() {
        if (selectedVehiclesModel.count === 1){
            var vehicle = getSelectedVehicles()[0]
            QGroundControl.multiVehicleManager.activeVehicle = vehicle
        }
    }

    function selectAll() {
        var vehicles = QGroundControl.multiVehicleManager.vehicles
        for (var i = 0; i < vehicles.count; i++) {
            var vehicle = vehicles.get(i)
            var vehicleId = vehicle.id
            if (getVehicleIndex(vehicleId) === -1){
                selectVehicle(vehicleId)
            }
        }
        printSelectedVehicles()
    }

    function deselectAll() {
        selectedVehiclesModel.clear()
    }

    function isSelected(vehicleId) {
        for (var i = 0; i < selectedVehiclesModel.count; i++) {
            if (selectedVehiclesModel.get(i).id === vehicleId) {
                return true
            }
        }
        return false
    }


    function selectVehicle(vehicleId) {
        selectedVehiclesModel.append({ id: vehicleId })
    }

    function deselectVehicle(vehicleId) {
        var index = getVehicleIndex(vehicleId)
        if (index !== -1) {
            selectedVehiclesModel.remove(index)
        }
    }

    function toggleSelect(vehicleId) {
        if (getVehicleIndex(vehicleId) !== -1) {
            deselectVehicle(vehicleId)
        } else {
            selectVehicle(vehicleId)
        }
        printSelectedVehicles()
    }

    function printSelectedVehicles() {
        var vehicles = [ ]
        forEachSelectedVehicle(function(vehicle) {
            vehicles.push(vehicle.id)
        })
        console.log(vehicles)
    }

    QGCListView {
        id:                 vehicleList
        anchors.left:       parent.left
        anchors.right:      parent.right
        anchors.topMargin:  _margin
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        spacing:            ScreenTools.defaultFontPixelHeight / 2
        orientation:        ListView.Vertical
        model:              QGroundControl.multiVehicleManager.vehicles
        cacheBuffer:        _cacheBuffer < 0 ? 0 : _cacheBuffer
        clip:               true

        property real _cacheBuffer:     height * 2

        delegate: Rectangle {
            width:          vehicleList.width
            height:         innerColumn.y + innerColumn.height + _margin
            color:          QGroundControl.multiVehicleManager.activeVehicle == _vehicle ? _activeVehicleColor : qgcPal.button
            radius:         _margin
            border.width:   isSelected(_vehicle.id) ? 1 : 0
            border.color:   qgcPal.text

            property var    _vehicle:   object

            RowLayout {
                id:                 innerColumn
                anchors.margins:    _margin
                anchors.top:        parent.top
                anchors.left:       parent.left
                spacing:            _margin
                Layout.fillWidth:   true

                QGCCompassWidget {
                    size:                        _widgetHeight
                    usedByMultipleVehicleList:   true
                    vehicle:                     _vehicle
                }

                QGCLabel {
                    text: " | "
                    font.pointSize:     ScreenTools.largeFontPointSize
                    color:              qgcPal.text
                }

                QGCLabel {
                    text:               _vehicle ? _vehicle.id : ""
                    font.pointSize:     ScreenTools.largeFontPointSize
                    color:              qgcPal.text
                }

                QGCLabel {
                    text: " | "
                    font.pointSize:     ScreenTools.largeFontPointSize
                    color:              qgcPal.text
                }

                ColumnLayout {
                    Layout.alignment:   Qt.AlignCenter
                    spacing:            _margin

                    FlightModeMenu {
                        Layout.alignment:           Qt.AlignHCenter
                        font.pointSize:             ScreenTools.largeFontPointSize
                        color:                      qgcPal.text
                        currentVehicle:             _vehicle
                    }

                    QGCLabel {
                        Layout.alignment:           Qt.AlignHCenter
                        text:                       _vehicle && _vehicle.armed ? qsTr("Armed") : qsTr("Disarmed")
                        color:                      qgcPal.text
                    }
                }

            }

            QGCMouseArea {
                anchors.fill:   parent
                onClicked:      multiVehicleList.toggleSelect(_vehicle.id)
            }

        }
    }
}
