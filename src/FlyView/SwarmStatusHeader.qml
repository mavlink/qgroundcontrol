import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView

ColumnLayout {
    id: _root

    property var    _vehicles:          QGroundControl.multiVehicleManager.vehicles
    property var    _selectedVehicles:  QGroundControl.multiVehicleManager.selectedVehicles
    property int    _totalCount:        _vehicles ? _vehicles.count : 0
    property int    _selectedCount:     _selectedVehicles ? _selectedVehicles.count : 0
    property real   _spacing:           ScreenTools.defaultFontPixelHeight / 2

    spacing: _spacing

    // --- Header ---
    QGCLabel {
        text:               qsTr("Swarm Status")
        font.pointSize:     ScreenTools.mediumFontPointSize
        font.bold:          true
        Layout.alignment:   Qt.AlignHCenter
    }

    // --- Vehicle Counts Row ---
    Rectangle {
        Layout.fillWidth:   true
        height:             countsRow.implicitHeight + ScreenTools.defaultFontPixelHeight
        color:              qgcPal.windowShade
        radius:             ScreenTools.defaultFontPixelHeight / 4

        RowLayout {
            id:                     countsRow
            anchors.fill:           parent
            anchors.margins:        ScreenTools.defaultFontPixelHeight / 2

            Repeater {
                model: [
                    { label: qsTr("Total"),    value: _totalCount,    color: qgcPal.text },
                    { label: qsTr("Selected"), value: _selectedCount, color: qgcPal.text },
                    { label: qsTr("Armed"),    value: _armedCount(),  color: _armedCount() > 0 ? "orange" : qgcPal.text },
                    { label: qsTr("Flying"),   value: _flyingCount(), color: _flyingCount() > 0 ? "green" : qgcPal.text }
                ]

                ColumnLayout {
                    Layout.fillWidth:   true
                    spacing:            2

                    QGCLabel {
                        text:               modelData.value.toString()
                        font.pointSize:     ScreenTools.largeFontPointSize
                        font.bold:          true
                        color:              modelData.color
                        Layout.alignment:   Qt.AlignHCenter
                    }

                    QGCLabel {
                        text:               modelData.label
                        font.pointSize:     ScreenTools.smallFontPointSize
                        Layout.alignment:   Qt.AlignHCenter
                    }
                }
            }
        }
    }

    // --- Battery Overview ---
    Rectangle {
        Layout.fillWidth:   true
        height:             batteryColumn.implicitHeight + ScreenTools.defaultFontPixelHeight
        color:              qgcPal.windowShade
        radius:             ScreenTools.defaultFontPixelHeight / 4

        ColumnLayout {
            id:                     batteryColumn
            anchors.fill:           parent
            anchors.margins:        ScreenTools.defaultFontPixelHeight / 2
            spacing:                _spacing / 2

            QGCLabel {
                text:               qsTr("Battery Overview")
                font.bold:          true
                Layout.alignment:   Qt.AlignHCenter
            }

            RowLayout {
                Layout.fillWidth:   true
                spacing:            ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    text:               qsTr("Avg:")
                    Layout.fillWidth:   true
                }

                Rectangle {
                    width:              ScreenTools.defaultFontPixelWidth * 10
                    height:             ScreenTools.defaultFontPixelHeight
                    color:              qgcPal.window
                    radius:             height / 4

                    Rectangle {
                        anchors.left:       parent.left
                        anchors.top:        parent.top
                        anchors.bottom:     parent.bottom
                        width:              parent.width * Math.max(0, Math.min(1, _avgBattery() / 100))
                        radius:             parent.radius
                        color:              _batteryColor(_avgBattery())
                    }
                }

                QGCLabel {
                    text:   _avgBattery() >= 0 ? _avgBattery().toFixed(0) + "%" : qsTr("N/A")
                    color:  _batteryColor(_avgBattery())
                }
            }

            RowLayout {
                Layout.fillWidth:   true
                spacing:            ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    text:               qsTr("Min:")
                    Layout.fillWidth:   true
                }

                QGCLabel {
                    text:   _minBattery() >= 0 ? _minBattery().toFixed(0) + "%" : qsTr("N/A")
                    color:  _batteryColor(_minBattery())
                }

                QGCLabel {
                    text:               qsTr("Max:")
                    Layout.fillWidth:   true
                }

                QGCLabel {
                    text:   _maxBattery() >= 0 ? _maxBattery().toFixed(0) + "%" : qsTr("N/A")
                    color:  _batteryColor(_maxBattery())
                }
            }
        }
    }

    // --- Altitude & Speed Overview ---
    Rectangle {
        Layout.fillWidth:   true
        height:             altSpeedColumn.implicitHeight + ScreenTools.defaultFontPixelHeight
        color:              qgcPal.windowShade
        radius:             ScreenTools.defaultFontPixelHeight / 4

        ColumnLayout {
            id:                     altSpeedColumn
            anchors.fill:           parent
            anchors.margins:        ScreenTools.defaultFontPixelHeight / 2
            spacing:                _spacing / 2

            QGCLabel {
                text:               qsTr("Altitude & Speed")
                font.bold:          true
                Layout.alignment:   Qt.AlignHCenter
            }

            GridLayout {
                columns:            4
                Layout.fillWidth:   true
                columnSpacing:      ScreenTools.defaultFontPixelWidth / 2
                rowSpacing:         2

                QGCLabel { text: qsTr("Alt Min:"); font.pointSize: ScreenTools.smallFontPointSize }
                QGCLabel {
                    text: _minAltitude().toFixed(1) + " m"
                    font.pointSize: ScreenTools.smallFontPointSize
                }
                QGCLabel { text: qsTr("Alt Max:"); font.pointSize: ScreenTools.smallFontPointSize }
                QGCLabel {
                    text: _maxAltitude().toFixed(1) + " m"
                    font.pointSize: ScreenTools.smallFontPointSize
                }

                QGCLabel { text: qsTr("Avg Spd:"); font.pointSize: ScreenTools.smallFontPointSize }
                QGCLabel {
                    text: _avgSpeed().toFixed(1) + " m/s"
                    font.pointSize: ScreenTools.smallFontPointSize
                }
                QGCLabel { text: qsTr("Max Spd:"); font.pointSize: ScreenTools.smallFontPointSize }
                QGCLabel {
                    text: _maxSpeed().toFixed(1) + " m/s"
                    font.pointSize: ScreenTools.smallFontPointSize
                }
            }
        }
    }

    // --- GPS Status ---
    Rectangle {
        Layout.fillWidth:   true
        height:             gpsRow.implicitHeight + ScreenTools.defaultFontPixelHeight
        color:              qgcPal.windowShade
        radius:             ScreenTools.defaultFontPixelHeight / 4

        RowLayout {
            id:                     gpsRow
            anchors.fill:           parent
            anchors.margins:        ScreenTools.defaultFontPixelHeight / 2
            spacing:                ScreenTools.defaultFontPixelWidth

            QGCLabel {
                text:               qsTr("GPS:")
                font.bold:          true
            }

            QGCLabel {
                text:               qsTr("%1 with fix").arg(_gpsFixCount())
                color:              _gpsFixCount() === _selectedCount ? "green" : "orange"
                Layout.fillWidth:   true
            }

            QGCLabel {
                text:               qsTr("Avg Sats: %1").arg(_avgSatCount().toFixed(0))
            }
        }
    }

    // Refresh timer to update computed values
    Timer {
        interval:   1000
        running:    true
        repeat:     true
        onTriggered: _root._totalCount = _vehicles ? _vehicles.count : 0
    }

    // --- Helper Functions ---
    function _armedCount() {
        var count = 0
        if (!_selectedVehicles) return 0
        for (var i = 0; i < _selectedVehicles.count; i++) {
            if (_selectedVehicles.get(i).armed) count++
        }
        return count
    }

    function _flyingCount() {
        var count = 0
        if (!_selectedVehicles) return 0
        for (var i = 0; i < _selectedVehicles.count; i++) {
            if (_selectedVehicles.get(i).flying) count++
        }
        return count
    }

    function _avgBattery() {
        if (!_selectedVehicles || _selectedVehicles.count === 0) return -1
        var total = 0
        var validCount = 0
        for (var i = 0; i < _selectedVehicles.count; i++) {
            var vehicle = _selectedVehicles.get(i)
            if (vehicle.batteries && vehicle.batteries.count > 0) {
                var batt = vehicle.batteries.get(0)
                if (batt && batt.percentRemaining && batt.percentRemaining.rawValue >= 0) {
                    total += batt.percentRemaining.rawValue
                    validCount++
                }
            }
        }
        return validCount > 0 ? total / validCount : -1
    }

    function _minBattery() {
        if (!_selectedVehicles || _selectedVehicles.count === 0) return -1
        var minVal = 101
        for (var i = 0; i < _selectedVehicles.count; i++) {
            var vehicle = _selectedVehicles.get(i)
            if (vehicle.batteries && vehicle.batteries.count > 0) {
                var batt = vehicle.batteries.get(0)
                if (batt && batt.percentRemaining && batt.percentRemaining.rawValue >= 0) {
                    minVal = Math.min(minVal, batt.percentRemaining.rawValue)
                }
            }
        }
        return minVal <= 100 ? minVal : -1
    }

    function _maxBattery() {
        if (!_selectedVehicles || _selectedVehicles.count === 0) return -1
        var maxVal = -1
        for (var i = 0; i < _selectedVehicles.count; i++) {
            var vehicle = _selectedVehicles.get(i)
            if (vehicle.batteries && vehicle.batteries.count > 0) {
                var batt = vehicle.batteries.get(0)
                if (batt && batt.percentRemaining && batt.percentRemaining.rawValue >= 0) {
                    maxVal = Math.max(maxVal, batt.percentRemaining.rawValue)
                }
            }
        }
        return maxVal
    }

    function _batteryColor(percent) {
        if (percent < 0)  return qgcPal.text
        if (percent < 20) return "red"
        if (percent < 50) return "orange"
        return "green"
    }

    function _minAltitude() {
        if (!_selectedVehicles || _selectedVehicles.count === 0) return 0
        var minVal = 99999
        for (var i = 0; i < _selectedVehicles.count; i++) {
            var vehicle = _selectedVehicles.get(i)
            var alt = vehicle.altitudeRelative.rawValue
            if (alt !== undefined) {
                minVal = Math.min(minVal, alt)
            }
        }
        return minVal < 99999 ? minVal : 0
    }

    function _maxAltitude() {
        if (!_selectedVehicles || _selectedVehicles.count === 0) return 0
        var maxVal = -99999
        for (var i = 0; i < _selectedVehicles.count; i++) {
            var vehicle = _selectedVehicles.get(i)
            var alt = vehicle.altitudeRelative.rawValue
            if (alt !== undefined) {
                maxVal = Math.max(maxVal, alt)
            }
        }
        return maxVal > -99999 ? maxVal : 0
    }

    function _avgSpeed() {
        if (!_selectedVehicles || _selectedVehicles.count === 0) return 0
        var total = 0
        var count = 0
        for (var i = 0; i < _selectedVehicles.count; i++) {
            var vehicle = _selectedVehicles.get(i)
            var spd = vehicle.groundSpeed.rawValue
            if (spd !== undefined) {
                total += spd
                count++
            }
        }
        return count > 0 ? total / count : 0
    }

    function _maxSpeed() {
        if (!_selectedVehicles || _selectedVehicles.count === 0) return 0
        var maxVal = 0
        for (var i = 0; i < _selectedVehicles.count; i++) {
            var vehicle = _selectedVehicles.get(i)
            var spd = vehicle.groundSpeed.rawValue
            if (spd !== undefined) {
                maxVal = Math.max(maxVal, spd)
            }
        }
        return maxVal
    }

    function _gpsFixCount() {
        if (!_selectedVehicles) return 0
        var count = 0
        for (var i = 0; i < _selectedVehicles.count; i++) {
            var vehicle = _selectedVehicles.get(i)
            if (vehicle.gps && vehicle.gps.lock && vehicle.gps.lock.rawValue >= 3) {
                count++
            }
        }
        return count
    }

    function _avgSatCount() {
        if (!_selectedVehicles || _selectedVehicles.count === 0) return 0
        var total = 0
        var count = 0
        for (var i = 0; i < _selectedVehicles.count; i++) {
            var vehicle = _selectedVehicles.get(i)
            if (vehicle.gps && vehicle.gps.count) {
                total += vehicle.gps.count.rawValue
                count++
            }
        }
        return count > 0 ? total / count : 0
    }
}
