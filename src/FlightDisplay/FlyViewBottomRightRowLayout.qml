/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightDisplay
import QGroundControl.Palette
import QGroundControl.ScreenTools

Rectangle {
    id:             bottomStrip
    height:         Math.max(ScreenTools.minTouchPixels * 0.92, ScreenTools.defaultFontPixelHeight * 3.05)
    implicitWidth:  _contentPreferredWidth
    color:          "transparent"
    radius:         Math.round(ScreenTools.defaultFontPixelWidth * 0.78)
    border.color:   "transparent"
    border.width:   0
    clip:           true

    property var  _activeVehicle:      QGroundControl.multiVehicleManager.activeVehicle
    property var  _vehicles:           QGroundControl.multiVehicleManager.vehicles
    property var  _selectedVehicles:   QGroundControl.multiVehicleManager.selectedVehicles
    property var  _guidedController:   globals.guidedControllerFlyView
    property var  backdropSourceItem:  null
    property real _stripMargin:        ScreenTools.defaultFontPixelWidth * 0.38
    property real _rowSpacing:         ScreenTools.defaultFontPixelWidth * 0.38
    property real _vehicleTileWidth:   Math.max(ScreenTools.defaultFontPixelWidth * 24.0, ScreenTools.minTouchPixels * 3.8)
    property real _actionPanelWidth:   Math.max(ScreenTools.defaultFontPixelWidth * 52.0, ScreenTools.minTouchPixels * 8.4)
    property int  _vehicleCount:       _vehicles ? _vehicles.count : 0
    property real _vehicleListWidth:   _vehicleCount > 0 ? ((_vehicleCount * _vehicleTileWidth) + ((_vehicleCount - 1) * _rowSpacing)) : 0
    property real _contentGapWidth:    _vehicleCount > 0 ? _rowSpacing : 0
    property real _contentPreferredWidth: (_stripMargin * 2) + _vehicleListWidth + _contentGapWidth + _actionPanelWidth
    property real spacing:             0
    property bool panelExpanded:        false
    property var  panelVehicle:         null

    signal vehicleClicked(var vehicle)

    QGCPalette { id: qgcPal }

    GlassBackdrop {
        anchors.fill:       parent
        sourceItem:         bottomStrip.backdropSourceItem
        backdropBlurEnabled:true
        targetItem:         bottomStrip
        cornerRadius:       bottomStrip.radius
        sourceScale:        0.42
        blurAmount:         0.94
        blurMax:            42
        sourceBrightness:   -0.01
        sourceSaturation:   0.62
        tintColor:          Qt.rgba(0.045, 0.048, 0.052, 0.80)
        sheenColor:         "transparent"
    }

    function batteryText(vehicle) {
        if (vehicle && vehicle.batteries && vehicle.batteries.count > 0) {
            var battery = vehicle.batteries.get(0)
            if (battery && !isNaN(battery.percentRemaining.rawValue)) {
                return battery.percentRemaining.valueString + battery.percentRemaining.units
            }
            if (battery && !isNaN(battery.voltage.rawValue)) {
                return battery.voltage.valueString + " " + battery.voltage.units
            }
        }
        return qsTr("N/A")
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

    function flightModeDisplayText(mode) {
        if (!mode || mode.length === 0) {
            return qsTr("--")
        }

        var prefix = ""
        var displayMode = mode
        var normalized = mode.toLowerCase()
        if (normalized.indexOf("quadplane ") === 0) {
            prefix = qsTr("QuadPlane") + " "
            displayMode = mode.substr(10)
            normalized = displayMode.toLowerCase()
        }

        if (normalized === "manual") {
            return prefix + qsTr("Manual")
        }
        if (normalized === "stabilize" || normalized === "stabilized") {
            return prefix + qsTr("Stabilize")
        }
        if (normalized === "alt hold" || normalized === "althold" || normalized === "altitude hold") {
            return prefix + qsTr("Alt Hold")
        }
        if (normalized === "loiter") {
            return prefix + qsTr("Loiter")
        }
        if (normalized.indexOf("follow") !== -1) {
            return prefix + qsTr("Follow")
        }
        if (normalized.indexOf("guided") !== -1) {
            return prefix + qsTr("Guided")
        }
        if (normalized.indexOf("auto") !== -1) {
            return prefix + qsTr("Auto")
        }
        if (normalized === "rtl") {
            return prefix + qsTr("RTL")
        }
        if (normalized === "land") {
            return prefix + qsTr("Land")
        }
        if (normalized === "takeoff") {
            return prefix + qsTr("Takeoff")
        }
        if (normalized === "hold") {
            return prefix + qsTr("Hold")
        }
        if (normalized.indexOf("position") !== -1 || normalized === "poshold") {
            return prefix + qsTr("Position")
        }
        if (normalized === "acro") {
            return prefix + qsTr("Acro")
        }
        if (normalized === "brake") {
            return prefix + qsTr("Brake")
        }
        if (normalized === "circle") {
            return prefix + qsTr("Circle")
        }

        return prefix + displayMode
    }

    function factText(fact, fallbackText) {
        if (!fact || fact.valueString === undefined) {
            return fallbackText
        }
        return fact.valueString + (fact.units && fact.units !== "" ? " " + fact.units : "")
    }

    function vehicleSelected(vehicleId) {
        if (!_selectedVehicles) {
            return false
        }
        for (var i = 0; i < _selectedVehicles.count; i++) {
            if (_selectedVehicles.get(i).id === vehicleId) {
                return true
            }
        }
        return false
    }

    function toggleSelect(vehicleId) {
        if (vehicleSelected(vehicleId)) {
            QGroundControl.multiVehicleManager.deselectVehicle(vehicleId)
        } else {
            QGroundControl.multiVehicleManager.selectVehicle(vehicleId)
        }
    }

    function activateVehicleFromCard(vehicle) {
        if (!vehicle) {
            vehicleClicked(vehicle)
            return
        }

        if (panelExpanded && panelVehicle === vehicle) {
            QGroundControl.multiVehicleManager.deselectVehicle(vehicle.id)
        } else {
            QGroundControl.multiVehicleManager.deselectAllVehicles()
            QGroundControl.multiVehicleManager.selectVehicle(vehicle.id)
        }

        vehicleClicked(vehicle)
    }

    function selectAll() {
        if (!_vehicles) {
            return
        }
        for (var i = 0; i < _vehicles.count; i++) {
            var vehicle = _vehicles.get(i)
            if (vehicle && !vehicleSelected(vehicle.id)) {
                QGroundControl.multiVehicleManager.selectVehicle(vehicle.id)
            }
        }
    }

    function deselectAll() {
        QGroundControl.multiVehicleManager.deselectAllVehicles()
    }

    function allSelected() {
        return _vehicles && _selectedVehicles && _vehicles.count > 0 && _selectedVehicles.count === _vehicles.count
    }

    function toggleAllSelected() {
        if (allSelected()) {
            deselectAll()
        } else {
            selectAll()
        }
    }

    function armAvailable() {
        if (!_selectedVehicles) {
            return false
        }
        for (var i = 0; i < _selectedVehicles.count; i++) {
            if (_selectedVehicles.get(i).armed === false) {
                return true
            }
        }
        return false
    }

    function disarmAvailable() {
        if (!_selectedVehicles) {
            return false
        }
        for (var i = 0; i < _selectedVehicles.count; i++) {
            if (_selectedVehicles.get(i).armed === true) {
                return true
            }
        }
        return false
    }

    function startAvailable() {
        if (!_selectedVehicles) {
            return false
        }
        for (var i = 0; i < _selectedVehicles.count; i++) {
            var vehicle = _selectedVehicles.get(i)
            if (vehicle.armed === true && vehicle.flightMode !== vehicle.missionFlightMode) {
                return true
            }
        }
        return false
    }

    function pauseAvailable() {
        if (!_selectedVehicles) {
            return false
        }
        for (var i = 0; i < _selectedVehicles.count; i++) {
            var vehicle = _selectedVehicles.get(i)
            if (vehicle.armed === true && vehicle.pauseVehicleSupported) {
                return true
            }
        }
        return false
    }

    component SelectBox: Item {
        id: selectBox

        property bool checked: false
        property bool available: true
        property string countText: ""
        property real preferredBoxSize: 0
        property real boxSize: preferredBoxSize > 0 ? preferredBoxSize : Math.min(height * 0.82, Math.max(18, ScreenTools.defaultFontPixelHeight * 0.88))
        property real _countSpacing: ScreenTools.defaultFontPixelWidth * 0.42
        property real _sidePadding: ScreenTools.defaultFontPixelWidth * 0.22
        signal clicked()

        opacity:        available ? 1.0 : 0.50

        QGCLabel {
            id:                     countLabel
            x:                      selectBox._sidePadding
            width:                  Math.max(0, checkBoxGraphic.x - selectBox._countSpacing - x)
            anchors.verticalCenter: checkBoxGraphic.verticalCenter
            text:                   selectBox.countText
            visible:                text !== ""
            color:                  qgcPal.text
            font.bold:              true
            font.pointSize:         ScreenTools.defaultFontPointSize
            horizontalAlignment:    Text.AlignRight
            verticalAlignment:      Text.AlignVCenter
            elide:                  Text.ElideRight
        }

        Rectangle {
            id:                 checkBoxGraphic
            x:                  selectBox.countText === "" ? (selectBox.width - width) / 2 : selectBox.width - width - selectBox._sidePadding
            y:                  (selectBox.height - height) / 2
            width:              selectBox.boxSize
            height:             width
            radius:             Math.round(width * 0.18)
            color:              selectBox.checked ? qgcPal.primaryButton : Qt.rgba(1, 1, 1, 0.035)
            border.color:       selectBox.checked ? qgcPal.primaryButton : qgcPal.buttonText
            border.width:       1
            opacity:            selectBox.checked ? 1.0 : 0.58

            Canvas {
                anchors.fill:       parent
                visible:            selectBox.checked

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    ctx.strokeStyle = Qt.rgba(0.020, 0.026, 0.034, 1)
                    ctx.lineWidth = Math.max(2, width * 0.14)
                    ctx.lineCap = "round"
                    ctx.lineJoin = "round"
                    ctx.beginPath()
                    ctx.moveTo(width * 0.23, height * 0.54)
                    ctx.lineTo(width * 0.42, height * 0.72)
                    ctx.lineTo(width * 0.78, height * 0.30)
                    ctx.stroke()
                }

                onWidthChanged:     requestPaint()
                onHeightChanged:    requestPaint()
                onVisibleChanged:   if (visible) requestPaint()
            }
        }

        QGCMouseArea {
            anchors.fill: parent
            enabled:      selectBox.available
            onClicked:    selectBox.clicked()
        }
    }

    component StripButton: Rectangle {
        id: stripButton

        property string text: ""
        property string iconSource: ""
        property bool available: true
        signal clicked()

        Layout.fillWidth:       true
        Layout.minimumWidth:    Math.max(ScreenTools.defaultFontPixelWidth * 8.0, ScreenTools.minTouchPixels * 1.10)
        Layout.preferredWidth:  Math.max(ScreenTools.defaultFontPixelWidth * 9.2, ScreenTools.minTouchPixels * 1.24)
        Layout.fillHeight:      true
        radius:                 Math.round(ScreenTools.defaultFontPixelWidth * 0.28)
        color:                  available ? (buttonMouse.pressed ? Qt.rgba(0.135, 0.140, 0.150, 0.44) :
                                             (buttonMouse.containsMouse ? Qt.rgba(0.175, 0.180, 0.190, 0.30) : "transparent"))
                                             : "transparent"
        border.color:           (available && (buttonMouse.pressed || buttonMouse.containsMouse)) ? Qt.rgba(0.82, 0.90, 0.95, 0.16) : "transparent"
        border.width:           (available && (buttonMouse.pressed || buttonMouse.containsMouse)) ? 1 : 0
        opacity:                available ? 1.0 : 0.54

        ColumnLayout {
            anchors.centerIn:   parent
            width:              parent.width - ScreenTools.defaultFontPixelWidth * 0.38
            spacing:            0

            QGCColoredImage {
                Layout.alignment:   Qt.AlignHCenter
                source:             stripButton.iconSource
                visible:            stripButton.iconSource !== ""
                color:              stripButton.available ? qgcPal.text : qgcPal.buttonText
                width:              ScreenTools.defaultFontPixelHeight * 0.78
                height:             width
                sourceSize.width:   width
                fillMode:           Image.PreserveAspectFit
            }

            QGCLabel {
                Layout.alignment:       Qt.AlignHCenter
                Layout.fillWidth:       true
                text:                   stripButton.text
                color:                  qgcPal.text
                font.bold:              true
                font.pointSize:         ScreenTools.defaultFontPointSize
                horizontalAlignment:    Text.AlignHCenter
                verticalAlignment:      Text.AlignVCenter
                fontSizeMode:           Text.HorizontalFit
                minimumPointSize:       ScreenTools.smallFontPointSize
                elide:                  Text.ElideRight
                maximumLineCount:       1
            }
        }

        QGCMouseArea {
            id:         buttonMouse
            anchors.fill: parent
            enabled:    stripButton.available
            hoverEnabled: !ScreenTools.isMobile
            onClicked:  stripButton.clicked()
        }
    }

    RowLayout {
        anchors.fill:       parent
        anchors.margins:    _stripMargin
        spacing:            _rowSpacing

        QGCFlickable {
            id:                     vehicleFlick
            visible:                _vehicleCount > 0
            Layout.fillWidth:       true
            Layout.fillHeight:      true
            Layout.minimumWidth:    0
            contentWidth:           vehicleRow.implicitWidth
            contentHeight:          height
            flickableDirection:     Flickable.HorizontalFlick
            boundsBehavior:         Flickable.StopAtBounds
            interactive:            contentWidth > width
            clip:                   true
            indicatorColor:         qgcPal.buttonText

            RowLayout {
                id:                 vehicleRow
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                spacing:            _rowSpacing

                Repeater {
                    model: _vehicles

                    Rectangle {
                        id:                     vehicleTile
                        Layout.preferredWidth:  _vehicleTileWidth
                        Layout.fillHeight:      true
                        radius:                 Math.round(ScreenTools.defaultFontPixelWidth * 0.38)
                        color:                  "transparent"
                        border.color:           _activeVehicleTile ? qgcPal.primaryButton : Qt.rgba(0.82, 0.90, 0.95, 0.12)
                        border.width:           _activeVehicleTile ? 1 : 0

                        property var _vehicle: object
                        property bool _activeVehicleTile: _vehicle && _activeVehicle === _vehicle
                        property bool _expandedVehicle: panelExpanded && _vehicle === panelVehicle
                        property bool _selectedVehicle: _vehicle && vehicleSelected(_vehicle.id)

                        QGCMouseArea {
                            anchors.fill: parent
                            onClicked:  bottomStrip.activateVehicleFromCard(_vehicle)
                        }

                        RowLayout {
                            anchors.fill:       parent
                            anchors.margins:    ScreenTools.defaultFontPixelWidth * 0.58
                            spacing:            ScreenTools.defaultFontPixelWidth * 0.58

                            Rectangle {
                                Layout.alignment:   Qt.AlignVCenter
                                width:              ScreenTools.defaultFontPixelHeight * 0.42
                                height:             width
                                radius:             width / 2
                                color:              _vehicle && !_vehicle.vehicleLinkManager.communicationLost ? qgcPal.colorGreen : qgcPal.buttonBorder
                            }

                            ColumnLayout {
                                Layout.fillWidth:   true
                                spacing:            0

                                QGCLabel {
                                    Layout.fillWidth:   true
                                    text:               vehicleDisplayName(_vehicle)
                                    color:              _expandedVehicle ? qgcPal.text : qgcPal.buttonText
                                    font.bold:          _expandedVehicle
                                    font.pointSize:     ScreenTools.defaultFontPointSize
                                    elide:              Text.ElideRight
                                }

                                QGCLabel {
                                    Layout.fillWidth:   true
                                    text:               _vehicle ? flightModeDisplayText(_vehicle.flightMode) : qsTr("--")
                                    color:              qgcPal.buttonText
                                    font.pointSize:     ScreenTools.smallFontPointSize
                                    elide:              Text.ElideRight
                                }
                            }

                            QGCColoredImage {
                                Layout.alignment:   Qt.AlignVCenter
                                source:             "qrc:/InstrumentValueIcons/battery-full.svg"
                                color:              _expandedVehicle ? qgcPal.primaryButton : qgcPal.buttonBorder
                                width:              ScreenTools.defaultFontPixelHeight * 1.25
                                height:             width
                                sourceSize.width:   width
                                fillMode:           Image.PreserveAspectFit
                                transform: Translate { y: ScreenTools.defaultFontPixelHeight * 0.42 }
                            }

                            QGCLabel {
                                Layout.alignment:   Qt.AlignVCenter
                                text:               batteryText(_vehicle)
                                color:              _expandedVehicle ? qgcPal.text : qgcPal.buttonText
                                font.bold:          _expandedVehicle
                                elide:              Text.ElideRight
                                transform: Translate { y: ScreenTools.defaultFontPixelHeight * 0.42 }
                            }
                        }

                        SelectBox {
                            z:                  2
                            width:              Math.max(14, Math.min(16, ScreenTools.defaultFontPixelHeight * 0.56))
                            height:             width
                            anchors.top:        parent.top
                            anchors.right:      parent.right
                            anchors.topMargin:  ScreenTools.defaultFontPixelWidth * 0.28
                            anchors.rightMargin:ScreenTools.defaultFontPixelWidth * 0.28
                            checked:            vehicleTile._selectedVehicle
                            available:          !!_vehicle
                            onClicked: {
                                if (_vehicle) {
                                    bottomStrip.toggleSelect(_vehicle.id)
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.preferredWidth:  _actionPanelWidth
            Layout.minimumWidth:    _actionPanelWidth
            Layout.fillHeight:      true
            radius:                 Math.round(ScreenTools.defaultFontPixelWidth * 0.38)
            color:                  "transparent"
            border.color:           "transparent"
            border.width:           0

            RowLayout {
                anchors.fill:       parent
                anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 0.68
                anchors.rightMargin:ScreenTools.defaultFontPixelWidth * 0.68
                anchors.topMargin:  ScreenTools.defaultFontPixelHeight * 0.24
                anchors.bottomMargin:ScreenTools.defaultFontPixelHeight * 0.24
                spacing:            ScreenTools.defaultFontPixelWidth * 0.68

                SelectBox {
                    Layout.preferredWidth:  Math.max(ScreenTools.defaultFontPixelWidth * 8.8, ScreenTools.minTouchPixels * 1.35)
                    Layout.minimumWidth:    Math.max(ScreenTools.defaultFontPixelWidth * 8.2, ScreenTools.minTouchPixels * 1.25)
                    Layout.fillHeight:      true
                    preferredBoxSize:       Math.max(14, Math.min(16, ScreenTools.defaultFontPixelHeight * 0.56))
                    checked:                bottomStrip.allSelected()
                    available:              _vehicles && _vehicles.count > 0
                    countText:              (_selectedVehicles ? _selectedVehicles.count : 0) + "/" + (_vehicles ? _vehicles.count : 0)
                    onClicked:              bottomStrip.toggleAllSelected()
                }

                Rectangle {
                    Layout.fillHeight:      true
                    Layout.preferredWidth:  1
                    Layout.topMargin:       ScreenTools.defaultFontPixelHeight * 0.34
                    Layout.bottomMargin:    ScreenTools.defaultFontPixelHeight * 0.34
                    color:                  Qt.rgba(0.82, 0.90, 0.95, 0.18)
                }

                StripButton {
                    text:       qsTr("Arm")
                    iconSource: "/res/LockOpen.svg"
                    available:  bottomStrip.armAvailable()
                    onClicked:  _guidedController.confirmAction(_guidedController.actionMVArm)
                }

                StripButton {
                    text:       qsTr("Disarm")
                    iconSource: "/res/LockClosed.svg"
                    available:  bottomStrip.disarmAvailable()
                    onClicked:  _guidedController.confirmAction(_guidedController.actionMVDisarm)
                }

                StripButton {
                    text:       qsTr("Start")
                    iconSource: "/res/takeoff.svg"
                    available:  bottomStrip.startAvailable()
                    onClicked:  _guidedController.confirmAction(_guidedController.actionMVStartMission)
                }

                StripButton {
                    text:       qsTr("Pause")
                    iconSource: "/res/Pause.svg"
                    available:  bottomStrip.pauseAvailable()
                    onClicked:  _guidedController.confirmAction(_guidedController.actionMVPause)
                }
            }
        }
    }
}
