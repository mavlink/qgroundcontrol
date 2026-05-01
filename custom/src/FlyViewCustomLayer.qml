/**
 * FlyViewCustomLayer.qml
 * DEEPSEE C2 Station — Custom UI overlay for QGroundControl
 * Crown & Eagle Engineering
 *
 * Layout overview:
 *   ┌───────────────────────────────────────────────────────┐
 *   │                    TOP STATUS BAR                     │
 *   ├─────────────┬─────────────────────────┬───────────────┤
 *   │  LEFT       │   QGC MAP (managed by   │   RIGHT       │
 *   │  SIDEBAR    │   QGC — do not touch)   │  TELEMETRY    │
 *   ├─────────────┴─────────────────────────┴───────────────┤
 *   │                    COMMAND STRIP                      │
 *   └───────────────────────────────────────────────────────┘
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

Item {
    id: root

    // ─────────────────────────────────────────────────────────────────────
    // REQUIRED INTERFACE PROPERTIES  (consumed by QGC's FlyView host)
    // ─────────────────────────────────────────────────────────────────────
    property var parentToolInsets
    property var totalToolInsets: _totalToolInsets
    property var mapControl

    // ─────────────────────────────────────────────────────────────────────
    // LAYOUT CONSTANTS
    // ─────────────────────────────────────────────────────────────────────
    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    property var _planMasterController: globals.planMasterControllerFlyView
    property var _missionController:    _planMasterController ? _planMasterController.missionController : null
    property int _currentWpIndex:       _missionController ? _missionController.currentMissionIndex    : 0
    property int _totalWpCount:         _missionController ? _missionController.missionItemCount       : 0
    property var _activeWpItem:         (_missionController && _missionController.visualItems && _currentWpIndex < _missionController.visualItems.count)
                                        ? _missionController.visualItems.get(_currentWpIndex) : null

    readonly property real _topBarHeight:    44
    readonly property real _leftPanelWidth:  260
    readonly property real _rightPanelWidth: 260
    readonly property real _bottomBarHeight: 56

    // ─────────────────────────────────────────────────────────────────────
    // COLOR PALETTE
    // ─────────────────────────────────────────────────────────────────────
    readonly property string _clrPanel:       "#1a1c1f"  // main panel background
    readonly property string _clrCard:        "#2a2d32"  // card / control background
    readonly property string _clrPurple:      "#6a0dad"  // demo / AI/ML accent
    readonly property string _clrPurpleDim:   "#4a006d"  // locked-demo shade
    readonly property string _clrBlue:        "#1565C0"  // badge accent
    readonly property string _clrGreen:       "#4CAF50"  // connected / active / continue
    readonly property string _clrRed:         "#f44336"  // disconnected
    readonly property string _clrOrange:      "#e65100"  // return to home
    readonly property string _clrKill:        "#c62828"  // kill switch / land
    readonly property string _clrMuted:       "#888888"  // secondary text
    readonly property string _clrAmber:       "#f0a500"  // current-action label

    // ─────────────────────────────────────────────────────────────────────
    // LIVE TELEMETRY HELPERS
    // Centralized bindings so telemetry cards stay clean.
    // All values update automatically when the vehicle reports new data.
    // When no vehicle is connected, values show "—".
    // ─────────────────────────────────────────────────────────────────────
    readonly property string _telSpeed:    _activeVehicle ? _activeVehicle.vehicle.groundSpeed.value.toFixed(1) : "—"
    readonly property string _telHeading:  _activeVehicle ? _activeVehicle.vehicle.heading.value.toFixed(0)     : "—"
    readonly property string _telAltitude: _activeVehicle ? _activeVehicle.vehicle.altitudeRelative.value.toFixed(1) : "—"
    readonly property string _telLat:      _activeVehicle ? _activeVehicle.coordinate.latitude.toFixed(6)      : "—"
    readonly property string _telLon:      _activeVehicle ? _activeVehicle.coordinate.longitude.toFixed(6)     : "—"

    readonly property string _unitSpeed:    _activeVehicle ? _activeVehicle.vehicle.groundSpeed.units    : "m/s"
    readonly property string _unitAltitude: _activeVehicle ? _activeVehicle.vehicle.altitudeRelative.units : "m"

    // Battery: first battery in the vehicle's battery list
    property var  _battery:     _activeVehicle && _activeVehicle.batteries.count > 0
                                ? _activeVehicle.batteries.get(0) : null
    property real _batteryPct:  _battery && !isNaN(_battery.percentRemaining.rawValue)
                                ? _battery.percentRemaining.rawValue : -1

    // ─────────────────────────────────────────────────────────────────────
    // FLIGHT MODE
    // PX4 flight mode names used by _activeVehicle.flightMode / setFlightMode.
    //   Position — manual flying with GPS position + altitude hold
    //   Mission  — autonomous waypoint flight
    //   Hold     — pause / loiter at current position
    // Return and Land are handled by dedicated buttons, not the dropdown.
    // ─────────────────────────────────────────────────────────────────────
    readonly property var flightModes: ["Position", "Mission", "Hold"]

    // Read the vehicle's actual current flight mode
    readonly property string _currentFlightMode: _activeVehicle ? _activeVehicle.flightMode : "—"

    // ─────────────────────────────────────────────────────────────────────
    // DEMO STATE
    // selectedDemoIndex: -1 = no demo chosen; 0-3 = one of the four demos
    // demoLocked: true once a demo is selected; cleared by Complete Demo or RTH
    // ─────────────────────────────────────────────────────────────────────
    property int    selectedDemoIndex: -1
    property bool   demoLocked:        false

    // Demo #1 state
    property string demo1Color: "Red"

    // Demo #2 state
    property string demo2Shape: "Cube"

    // Demo #3 & #4 state — pending selections for the "add asset" row
    property string pendingColor: "Red"
    property string pendingShape: "Cube"

    // ─────────────────────────────────────────────────────────────────────
    // VEHICLE CONTROL STATE
    // isArmed: toggled by ARM/DISARM button; arming is only permitted in Demo #4
    // ─────────────────────────────────────────────────────────────────────
    property bool isArmed: false

    // ARM is only permitted during Demo #4 (Points Round)
    readonly property bool _canArm: demoLocked && selectedDemoIndex === 3

    // ─────────────────────────────────────────────────────────────────────
    // DEMO LOOKUP TABLES  (read-only data, safe to treat as constants)
    // ─────────────────────────────────────────────────────────────────────
    readonly property var demoNames: [
        "DEMONSTRATION #1: Identify Asset By Color",
        "DEMONSTRATION #2: Identify Asset by Shape",
        "DEMONSTRATION #3: Battleship",
        "DEMONSTRATION #4: Points Round"
    ]

    readonly property var colorOptions:  ["Red", "Blue", "Yellow", "Green", "Purple"]
    readonly property var shapeOptions:  ["Cube", "Sphere", "Triangle"]

    // Point values per (color, shape) for Demo #4.
    readonly property var pointTable: ({
        "Red":    { "Cube": 10, "Sphere": 15, "Triangle": 20 },
        "Blue":   { "Cube": 12, "Sphere": 18, "Triangle": 22 },
        "Yellow": { "Cube":  8, "Sphere": 12, "Triangle": 16 },
        "Green":  { "Cube": 11, "Sphere": 14, "Triangle": 19 },
        "Purple": { "Cube": 14, "Sphere": 20, "Triangle": 25 }
    })

    // ─────────────────────────────────────────────────────────────────────
    // HELPER FUNCTIONS
    // ─────────────────────────────────────────────────────────────────────

    function pointsFor(color, shape) {
        return (pointTable[color] && pointTable[color][shape]) ? pointTable[color][shape] : 0
    }

    function lockDemo(index) {
        selectedDemoIndex = index
        demoLocked        = true
    }

    function unlockDemo() {
        demoLocked        = false
        selectedDemoIndex = -1
        isArmed           = false
        demo1Color        = "Red"
        demo2Shape        = "Cube"
        pendingColor      = "Red"
        pendingShape      = "Cube"
        assetModel.clear()
    }

    // ─────────────────────────────────────────────────────────────────────
    // DATA MODELS
    // ─────────────────────────────────────────────────────────────────────
    ListModel { id: assetModel }

    // ─────────────────────────────────────────────────────────────────────
    // QGC TOOL INSETS
    // ─────────────────────────────────────────────────────────────────────
    QGCToolInsets {
        id:                  _totalToolInsets
        leftEdgeTopInset:    _leftPanelWidth
        leftEdgeCenterInset: _leftPanelWidth
        leftEdgeBottomInset: _leftPanelWidth
        rightEdgeTopInset:   _rightPanelWidth
        rightEdgeCenterInset:_rightPanelWidth
        rightEdgeBottomInset:_rightPanelWidth
        topEdgeLeftInset:    parentToolInsets.topEdgeLeftInset   + _topBarHeight
        topEdgeCenterInset:  parentToolInsets.topEdgeCenterInset + _topBarHeight
        topEdgeRightInset:   parentToolInsets.topEdgeRightInset  + _topBarHeight
        bottomEdgeLeftInset: _bottomBarHeight
        bottomEdgeCenterInset: _bottomBarHeight
        bottomEdgeRightInset:  _bottomBarHeight
    }


    // =========================================================================
    // TOP STATUS BAR
    // =========================================================================
    Rectangle {
        id:             topBar
        anchors.top:    parent.top
        anchors.left:   parent.left
        anchors.right:  parent.right
        height:         _topBarHeight
        color:          _clrPanel

        RowLayout {
            anchors.fill:        parent
            anchors.leftMargin:  12
            anchors.rightMargin: 12
            spacing:             10

            Text {
                text:           "Crown & Eagle Engineering"
                color:          "white"
                font.pixelSize: 14
                font.bold:      true
            }

            Rectangle {
                color:  _clrBlue
                radius: 4
                width:  uavLabel.width + 12
                height: 24
                Text {
                    id:               uavLabel
                    anchors.centerIn: parent
                    text:             _activeVehicle ? "UAV-" + _activeVehicle.id : "UAV-01"
                    color:            "white"
                    font.pixelSize:   12
                    font.bold:        true
                }
            }

            Row {
                spacing: 6
                Rectangle {
                    width: 8; height: 8; radius: 4
                    color: _activeVehicle ? _clrGreen : _clrRed
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text:                   _activeVehicle ? "CONNECTED" : "DISCONNECTED"
                    color:                  _activeVehicle ? _clrGreen   : _clrRed
                    font.pixelSize:         12
                    font.bold:              true
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Item { Layout.fillWidth: true }

            // Demo selector
            ComboBox {
                id:             demoComboBox
                enabled:        !demoLocked
                model:          ["Select Demonstration…"].concat(root.demoNames)
                currentIndex:   root.selectedDemoIndex + 1
                implicitWidth:  340
                implicitHeight: 28

                onActivated: function(index) {
                    if (index > 0) root.lockDemo(index - 1)
                }

                background: Rectangle {
                    color:   demoLocked ? _clrPurpleDim : _clrPurple
                    radius:  4
                    opacity: demoComboBox.enabled ? 1.0 : 0.75
                }

                contentItem: Text {
                    text:              demoComboBox.displayText
                    color:             "white"
                    font.pixelSize:    12
                    font.bold:         true
                    verticalAlignment: Text.AlignVCenter
                    leftPadding:       10
                    elide:             Text.ElideRight
                }

                indicator: Text {
                    text:                   "▼"
                    color:                  "white"
                    font.pixelSize:         10
                    anchors.right:          parent.right
                    anchors.rightMargin:    8
                    anchors.verticalCenter: parent.verticalCenter
                    visible:                !demoLocked
                }

                popup: Popup {
                    y:       demoComboBox.height
                    width:   demoComboBox.width
                    padding: 1
                    background: Rectangle { color: _clrCard; radius: 4 }
                    contentItem: ListView {
                        clip:           true
                        implicitHeight: contentHeight
                        model:          demoComboBox.delegateModel
                        ScrollIndicator.vertical: ScrollIndicator {}
                    }
                }

                delegate: ItemDelegate {
                    width:       demoComboBox.width
                    highlighted: demoComboBox.highlightedIndex === index
                    background: Rectangle { color: highlighted ? _clrPurple : _clrCard }
                    contentItem: Text {
                        text:              modelData
                        color:             "white"
                        font.pixelSize:    12
                        leftPadding:       10
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            Item { Layout.fillWidth: true }

            // Flight mode badge — shows vehicle's actual current mode
            Row {
                spacing: 6
                Text {
                    text:                   "Flight Mode:"
                    color:                  "white"
                    font.pixelSize:         12
                    anchors.verticalCenter: parent.verticalCenter
                }
                Rectangle {
                    color:  _clrBlue
                    radius: 4
                    width:  topModeLabel.width + 12
                    height: 24
                    Text {
                        id:               topModeLabel
                        anchors.centerIn: parent
                        text:             root._currentFlightMode
                        color:            "white"
                        font.pixelSize:   12
                        font.bold:        true
                    }
                }
            }

            // UTC clock
            Text {
                id:             utcClock
                color:          "white"
                font.pixelSize: 12

                property var _timer: Timer {
                    interval:    1000
                    running:     true
                    repeat:      true
                    onTriggered: utcClock.tick()
                }

                function tick() {
                    var now = new Date()
                    utcClock.text = "UTC: "
                        + now.getUTCHours().toString().padStart(2, "0") + ":"
                        + now.getUTCMinutes().toString().padStart(2, "0") + ":"
                        + now.getUTCSeconds().toString().padStart(2, "0")
                }

                Component.onCompleted: tick()
            }
        }
    }


    // =========================================================================
    // LEFT SIDEBAR  (260 px)
    // =========================================================================
    Rectangle {
        id:             leftPanel
        anchors.top:    topBar.bottom
        anchors.left:   parent.left
        anchors.bottom: bottomBar.top
        width:          _leftPanelWidth
        color:          _clrPanel

        ColumnLayout {
            anchors.fill:    parent
            anchors.margins: 12
            spacing:         10

            Row {
                spacing: 10
                Rectangle {
                    width: 36; height: 36; radius: 18; color: _clrCard
                    Text { anchors.centerIn: parent; text: "✈"; color: _clrGreen; font.pixelSize: 18 }
                }
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        text:           _activeVehicle ? _activeVehicle.vehicleName : "DEEPSEE"
                        color:          "white"
                        font.pixelSize: 14
                        font.bold:      true
                    }
                    Text {
                        text:           _activeVehicle ? "ACTIVE" : "INACTIVE"
                        color:          _activeVehicle ? _clrGreen : _clrMuted
                        font.pixelSize: 11
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: _clrCard }

            Column {
                Layout.fillWidth: true
                spacing:          4
                Text { text: "Mission Name"; color: _clrMuted; font.pixelSize: 11 }
                Rectangle {
                    width: 236; height: 36; color: _clrCard; radius: 4
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left:           parent.left
                        anchors.leftMargin:     10
                        anchors.right:          parent.right
                        anchors.rightMargin:    10
                        text:                   demoLocked ? root.demoNames[root.selectedDemoIndex] : "—"
                        color:                  demoLocked ? "white" : _clrMuted
                        font.pixelSize:         11
                        elide:                  Text.ElideRight
                    }
                }
            }

            Row {
                spacing: 20
                Column {
                    spacing: 4
                    Text { text: "Mission Phase"; color: _clrMuted; font.pixelSize: 11 }
                    Text { text: "Search"; color: "white"; font.pixelSize: 13; font.bold: true }
                }
                Column {
                    spacing: 4
                    Text { text: "Elapsed Time"; color: _clrMuted; font.pixelSize: 11 }
                    Text {
                        id:             elapsedClock
                        color:          "white"
                        font.pixelSize: 13
                        font.bold:      true
                        property int _secs: 0
                        property var _t: Timer {
                            interval:    1000
                            running:     true
                            repeat:      true
                            onTriggered: elapsedClock._secs++
                        }
                        text: {
                            var h = Math.floor(_secs / 3600).toString().padStart(2, "0")
                            var m = Math.floor((_secs % 3600) / 60).toString().padStart(2, "0")
                            var s = (_secs % 60).toString().padStart(2, "0")
                            return h + ":" + m + ":" + s
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true; height: 1; color: _clrCard
                visible: demoLocked
            }

            // Demo #1
            Column {
                Layout.fillWidth: true; spacing: 6
                visible: demoLocked && selectedDemoIndex === 0
                Text { text: "Target Color"; color: _clrMuted; font.pixelSize: 11 }
                ComboBox {
                    id: d1Color; model: root.colorOptions; width: 236
                    currentIndex: root.colorOptions.indexOf(root.demo1Color)
                    onActivated: function(i) { root.demo1Color = root.colorOptions[i] }
                    background: Rectangle { color: _clrCard; radius: 4 }
                    contentItem: Text { text: d1Color.displayText; color: "white"; font.pixelSize: 12; verticalAlignment: Text.AlignVCenter; leftPadding: 10 }
                    popup: Popup {
                        y: d1Color.height; width: d1Color.width; padding: 1
                        background: Rectangle { color: _clrCard; radius: 4 }
                        contentItem: ListView { clip: true; implicitHeight: contentHeight; model: d1Color.delegateModel }
                    }
                    delegate: ItemDelegate {
                        width: d1Color.width; highlighted: d1Color.highlightedIndex === index
                        background: Rectangle { color: highlighted ? "#444" : _clrCard }
                        contentItem: Text { text: modelData; color: "white"; font.pixelSize: 12; leftPadding: 10; verticalAlignment: Text.AlignVCenter }
                    }
                }
            }

            // Demo #2
            Column {
                Layout.fillWidth: true; spacing: 6
                visible: demoLocked && selectedDemoIndex === 1
                Text { text: "Target Shape"; color: _clrMuted; font.pixelSize: 11 }
                ComboBox {
                    id: d2Shape; model: root.shapeOptions; width: 236
                    currentIndex: root.shapeOptions.indexOf(root.demo2Shape)
                    onActivated: function(i) { root.demo2Shape = root.shapeOptions[i] }
                    background: Rectangle { color: _clrCard; radius: 4 }
                    contentItem: Text { text: d2Shape.displayText; color: "white"; font.pixelSize: 12; verticalAlignment: Text.AlignVCenter; leftPadding: 10 }
                    popup: Popup {
                        y: d2Shape.height; width: d2Shape.width; padding: 1
                        background: Rectangle { color: _clrCard; radius: 4 }
                        contentItem: ListView { clip: true; implicitHeight: contentHeight; model: d2Shape.delegateModel }
                    }
                    delegate: ItemDelegate {
                        width: d2Shape.width; highlighted: d2Shape.highlightedIndex === index
                        background: Rectangle { color: highlighted ? "#444" : _clrCard }
                        contentItem: Text { text: modelData; color: "white"; font.pixelSize: 12; leftPadding: 10; verticalAlignment: Text.AlignVCenter }
                    }
                }
            }

            // Demo #3 & #4
            Column {
                Layout.fillWidth: true; spacing: 8
                visible: demoLocked && (selectedDemoIndex === 2 || selectedDemoIndex === 3)
                Text { text: "Add Asset (max 3)"; color: _clrMuted; font.pixelSize: 11 }
                Row {
                    spacing: 6
                    ComboBox {
                        id: assetColorPicker; model: root.colorOptions; width: 104
                        currentIndex: root.colorOptions.indexOf(root.pendingColor)
                        onActivated: function(i) { root.pendingColor = root.colorOptions[i] }
                        background: Rectangle { color: _clrCard; radius: 4 }
                        contentItem: Text { text: assetColorPicker.displayText; color: "white"; font.pixelSize: 11; verticalAlignment: Text.AlignVCenter; leftPadding: 6 }
                        popup: Popup {
                            y: assetColorPicker.height; width: assetColorPicker.width; padding: 1
                            background: Rectangle { color: _clrCard; radius: 4 }
                            contentItem: ListView { clip: true; implicitHeight: contentHeight; model: assetColorPicker.delegateModel }
                        }
                        delegate: ItemDelegate {
                            width: assetColorPicker.width; highlighted: assetColorPicker.highlightedIndex === index
                            background: Rectangle { color: highlighted ? "#444" : _clrCard }
                            contentItem: Text { text: modelData; color: "white"; font.pixelSize: 11; leftPadding: 6; verticalAlignment: Text.AlignVCenter }
                        }
                    }
                    ComboBox {
                        id: assetShapePicker; model: root.shapeOptions; width: 88
                        currentIndex: root.shapeOptions.indexOf(root.pendingShape)
                        onActivated: function(i) { root.pendingShape = root.shapeOptions[i] }
                        background: Rectangle { color: _clrCard; radius: 4 }
                        contentItem: Text { text: assetShapePicker.displayText; color: "white"; font.pixelSize: 11; verticalAlignment: Text.AlignVCenter; leftPadding: 6 }
                        popup: Popup {
                            y: assetShapePicker.height; width: assetShapePicker.width; padding: 1
                            background: Rectangle { color: _clrCard; radius: 4 }
                            contentItem: ListView { clip: true; implicitHeight: contentHeight; model: assetShapePicker.delegateModel }
                        }
                        delegate: ItemDelegate {
                            width: assetShapePicker.width; highlighted: assetShapePicker.highlightedIndex === index
                            background: Rectangle { color: highlighted ? "#444" : _clrCard }
                            contentItem: Text { text: modelData; color: "white"; font.pixelSize: 11; leftPadding: 6; verticalAlignment: Text.AlignVCenter }
                        }
                    }
                    Rectangle {
                        width: 30; height: 28
                        color: assetModel.count < 3 ? _clrGreen : "#555"; radius: 4
                        Text { anchors.centerIn: parent; text: "+"; color: "white"; font.pixelSize: 18; font.bold: true }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if (assetModel.count < 3) {
                                    var pts = root.pointsFor(root.pendingColor, root.pendingShape)
                                    assetModel.append({ assetColor: root.pendingColor, assetShape: root.pendingShape, points: pts })
                                }
                            }
                        }
                    }
                }
                Column {
                    spacing: 4; width: 236
                    Repeater {
                        model: assetModel
                        delegate: Rectangle {
                            width: 236; height: 30; color: _clrCard; radius: 4
                            Text { anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 10; text: model.assetColor + " / " + model.assetShape; color: "white"; font.pixelSize: 11 }
                            Text { anchors.verticalCenter: parent.verticalCenter; anchors.right: removeBtn.left; anchors.rightMargin: 8; visible: selectedDemoIndex === 3; text: model.points + " pts"; color: _clrGreen; font.pixelSize: 11 }
                            Rectangle {
                                id: removeBtn; anchors.verticalCenter: parent.verticalCenter; anchors.right: parent.right; anchors.rightMargin: 8
                                width: 20; height: 20; color: _clrKill; radius: 3
                                Text { anchors.centerIn: parent; text: "×"; color: "white"; font.pixelSize: 14 }
                                MouseArea { anchors.fill: parent; onClicked: assetModel.remove(index) }
                            }
                        }
                    }
                }
                Rectangle {
                    width: 236; height: 32; color: _clrCard; radius: 4
                    visible: selectedDemoIndex === 3 && assetModel.count > 0
                    Row {
                        anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 10; spacing: 8
                        Text { text: "Total Points:"; color: _clrMuted; font.pixelSize: 12 }
                        Text {
                            color: _clrGreen; font.pixelSize: 14; font.bold: true
                            text: { var sum = 0; for (var i = 0; i < assetModel.count; i++) sum += assetModel.get(i).points; return sum }
                        }
                    }
                }
            }

            // ── Mission planning ──────────────────────────────────────────
            Rectangle { Layout.fillWidth: true; height: 1; color: _clrCard }

            Row {
                spacing: 6
                Text { text: "Mission Planning"; color: "white"; font.pixelSize: 13; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
                Item { width: 8 }
                Rectangle {
                    color: _clrCard; radius: 4; width: wpBadge.width + 10; height: 20
                    Text { id: wpBadge; anchors.centerIn: parent; text: (_currentWpIndex + 1) + " / " + _totalWpCount; color: _clrMuted; font.pixelSize: 11 }
                }
            }

            Rectangle {
                Layout.fillWidth: true; color: _clrCard; radius: 4; height: actionCol.height + 16
                Column {
                    id: actionCol; anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; spacing: 4
                    Text { text: "CURRENT ACTION"; color: _clrAmber; font.pixelSize: 10; font.bold: true }
                    Text { text: _activeWpItem ? _activeWpItem.commandName : "No active mission item"; color: "white"; font.pixelSize: 11; wrapMode: Text.WordWrap; width: parent.width }
                    Text { text: "Slide or hold spacebar to confirm"; color: _clrMuted; font.pixelSize: 10 }
                }
            }

            ListView {
                id: waypointList; Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 4
                model: _missionController ? _missionController.visualItems : null
                delegate: Rectangle {
                    property string _status: index < _currentWpIndex  ? "done"
                                           : index === _currentWpIndex ? "active"
                                           :                             "pending"
                    width: waypointList.width; height: 36
                    color: _status === "active" ? "#1a3a5c" : "transparent"; radius: 4
                    Row {
                        anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8; spacing: 8
                        Rectangle {
                            width: 18; height: 18; radius: 9; anchors.verticalCenter: parent.verticalCenter
                            color: _status === "done" ? _clrGreen : _status === "active" ? _clrBlue : "transparent"
                            border.color: _status === "pending" ? _clrMuted : "transparent"; border.width: 1
                            Text { anchors.centerIn: parent; text: _status === "done" ? "✓" : ""; color: "white"; font.pixelSize: 10 }
                        }
                        Text {
                            text: object.commandName
                            color: _status === "active" ? "white" : _clrMuted
                            font.pixelSize: 12; font.bold: _status === "active"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }

            Column {
                spacing: 8; Layout.fillWidth: true
                Rectangle {
                    width: 236; height: 40; color: _clrGreen; radius: 6
                    Row {
                        anchors.centerIn: parent; spacing: 8
                        Text { text: "▶"; color: "white"; font.pixelSize: 14; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: "Continue Mission"; color: "white"; font.pixelSize: 13; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: { if (_activeVehicle) _activeVehicle.flightMode = "Mission" }
                    }
                }
                Rectangle {
                    width: 236; height: 36; color: "transparent"; radius: 6; border.color: _clrMuted; border.width: 1
                    Text { anchors.centerIn: parent; text: "Pause Mission"; color: _clrMuted; font.pixelSize: 13 }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: { if (_activeVehicle) _activeVehicle.flightMode = "Hold" }
                    }
                }
            }
        }
    }


    // =========================================================================
    // RIGHT TELEMETRY PANEL  (260 px)
    // =========================================================================
    Rectangle {
        id:             rightPanel
        anchors.top:    topBar.bottom
        anchors.right:  parent.right
        anchors.bottom: bottomBar.top
        width:          _rightPanelWidth
        color:          _clrPanel

        Column {
            anchors.fill:    parent
            anchors.margins: 12
            spacing:         8

            Text { text: "Telemetry"; color: "white"; font.pixelSize: 16; font.bold: true }

            Rectangle {
                width: 236; height: 60; color: _clrCard; radius: 6
                Row {
                    anchors.fill: parent; anchors.margins: 10; spacing: 8
                    Text { text: "⟳"; color: _clrMuted; font.pixelSize: 14; anchors.verticalCenter: parent.verticalCenter }
                    Column {
                        anchors.verticalCenter: parent.verticalCenter; spacing: 2
                        Text { text: "SPEED"; color: _clrMuted; font.pixelSize: 10 }
                        Text { text: root._telSpeed + " " + root._unitSpeed; color: "white"; font.pixelSize: 18; font.bold: true }
                    }
                }
            }

            Rectangle {
                width: 236; height: 60; color: _clrCard; radius: 6
                Row {
                    anchors.fill: parent; anchors.margins: 10; spacing: 8
                    Text { text: "⊙"; color: _clrMuted; font.pixelSize: 14; anchors.verticalCenter: parent.verticalCenter }
                    Column {
                        anchors.verticalCenter: parent.verticalCenter; spacing: 2
                        Text { text: "HEADING"; color: _clrMuted; font.pixelSize: 10 }
                        Text { text: root._telHeading + "°"; color: "white"; font.pixelSize: 18; font.bold: true }
                    }
                }
            }

            Rectangle {
                width: 236; height: 60; color: _clrCard; radius: 6
                Row {
                    anchors.fill: parent; anchors.margins: 10; spacing: 8
                    Text { text: "▲"; color: _clrMuted; font.pixelSize: 14; anchors.verticalCenter: parent.verticalCenter }
                    Column {
                        anchors.verticalCenter: parent.verticalCenter; spacing: 2
                        Text { text: "ALTITUDE"; color: _clrMuted; font.pixelSize: 10 }
                        Row {
                            spacing: 6
                            Text { text: root._telAltitude + " " + root._unitAltitude; color: "white"; font.pixelSize: 18; font.bold: true }
                            Text { text: "AGL"; color: _clrGreen; font.pixelSize: 10; anchors.verticalCenter: parent.verticalCenter }
                        }
                    }
                }
            }

            Rectangle {
                width: 236; height: 60; color: _clrCard; radius: 6
                Row {
                    anchors.fill: parent; anchors.margins: 10; spacing: 8
                    Text { text: "⊕"; color: _clrMuted; font.pixelSize: 14; anchors.verticalCenter: parent.verticalCenter }
                    Column {
                        anchors.verticalCenter: parent.verticalCenter; spacing: 2
                        Text { text: "LATITUDE"; color: _clrMuted; font.pixelSize: 10 }
                        Text { text: root._telLat + "°"; color: "white"; font.pixelSize: 18; font.bold: true }
                    }
                }
            }

            Rectangle {
                width: 236; height: 60; color: _clrCard; radius: 6
                Row {
                    anchors.fill: parent; anchors.margins: 10; spacing: 8
                    Text { text: "⊕"; color: _clrMuted; font.pixelSize: 14; anchors.verticalCenter: parent.verticalCenter }
                    Column {
                        anchors.verticalCenter: parent.verticalCenter; spacing: 2
                        Text { text: "LONGITUDE"; color: _clrMuted; font.pixelSize: 10 }
                        Text { text: root._telLon + "°"; color: "white"; font.pixelSize: 18; font.bold: true }
                    }
                }
            }

            Rectangle {
                width: 236; height: 60; color: _clrCard; radius: 6
                Row {
                    anchors.fill: parent; anchors.margins: 10; spacing: 8
                    Text { text: "⏱"; color: _clrMuted; font.pixelSize: 14; anchors.verticalCenter: parent.verticalCenter }
                    Column {
                        anchors.verticalCenter: parent.verticalCenter; spacing: 2
                        Text { text: "TIME"; color: _clrMuted; font.pixelSize: 10 }
                        Text {
                            id: telUtcClock; color: "white"; font.pixelSize: 18; font.bold: true
                            property var _timer: Timer { interval: 1000; running: true; repeat: true; onTriggered: telUtcClock.tick() }
                            function tick() {
                                var now = new Date()
                                telUtcClock.text = now.getUTCHours().toString().padStart(2, "0") + ":" + now.getUTCMinutes().toString().padStart(2, "0") + ":" + now.getUTCSeconds().toString().padStart(2, "0") + " UTC"
                            }
                            Component.onCompleted: tick()
                        }
                    }
                }
            }

            Rectangle {
                id: batteryCard; width: 236; height: 60; color: _clrCard; radius: 6
                Row {
                    anchors.fill: parent; anchors.margins: 10; spacing: 8
                    Text { text: "⚡"; color: _clrMuted; font.pixelSize: 14; anchors.verticalCenter: parent.verticalCenter }
                    Column {
                        anchors.verticalCenter: parent.verticalCenter; spacing: 2
                        Text { text: "BATTERY"; color: _clrMuted; font.pixelSize: 10 }
                        Row {
                            spacing: 6
                            Text {
                                text: root._batteryPct >= 0 ? root._batteryPct.toFixed(0) + "%" : "—%"
                                color: "white"; font.pixelSize: 18; font.bold: true
                            }
                            Text {
                                visible: root._batteryPct >= 0
                                text: root._batteryPct > 50 ? "GOOD" : root._batteryPct > 20 ? "LOW" : "CRITICAL"
                                color: root._batteryPct > 50 ? _clrGreen : root._batteryPct > 20 ? _clrAmber : _clrRed
                                font.pixelSize: 10; anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                    }
                }
            }
        }
    }


    // =========================================================================
    // COMMAND STRIP  (bottom bar)
    // Left:   ARM | armed status (live) | Flight Mode dropdown (Position/Mission/Hold)
    // Center: Complete Demo
    // Right:  Return to Home (RTH) | Kill Switch | Retrieval Mech | Settings | AI/ML
    // =========================================================================
    Rectangle {
        id:             bottomBar
        anchors.bottom: parent.bottom
        anchors.left:   parent.left
        anchors.right:  parent.right
        height:         _bottomBarHeight
        color:          _clrPanel

        RowLayout {
            anchors.fill:        parent
            anchors.leftMargin:  12
            anchors.rightMargin: 12
            spacing:             8

            // ARM / DISARM
            Rectangle {
                id:     armButton
                width:  armLbl.width + 24; height: 34; radius: 6
                color:        isArmed ? _clrGreen : "transparent"
                border.color: isArmed ? "transparent" : (_canArm ? _clrMuted : "#444")
                border.width: 1
                opacity:      _canArm ? 1.0 : 0.4

                Row {
                    anchors.centerIn: parent; spacing: 6
                    Text { text: isArmed ? "✓" : "⊙"; color: "white"; font.pixelSize: 14; anchors.verticalCenter: parent.verticalCenter }
                    Text { id: armLbl; text: isArmed ? "DISARM" : "ARM"; color: "white"; font.pixelSize: 13; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
                }
                MouseArea {
                    anchors.fill: parent; enabled: _canArm
                    onClicked: isArmed = !isArmed
                    cursorShape: _canArm ? Qt.PointingHandCursor : Qt.ArrowCursor
                }
            }

            // Armed status — shows vehicle's actual armed state
            Text {
                text: _activeVehicle
                      ? (_activeVehicle.armed ? "Vehicle is armed" : "Vehicle is disarmed")
                      : "No vehicle"
                color: _activeVehicle && _activeVehicle.armed ? _clrAmber : _clrMuted
                font.pixelSize: 12
            }

            // Flight Mode dropdown — sends real PX4 mode commands
            Row {
                spacing: 6
                Text { text: "Flight Mode:"; color: "white"; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }

                ComboBox {
                    id:             flightModeCombo
                    model:          root.flightModes
                    implicitWidth:  120
                    implicitHeight: 34

                    // Track the vehicle's actual flight mode
                    currentIndex: root.flightModes.indexOf(root._currentFlightMode)

                    // Command the vehicle to switch mode
                    onActivated: function(i) {
                        if (_activeVehicle) {
                            _activeVehicle.flightMode = root.flightModes[i]
                        }
                    }

                    background: Rectangle { color: _clrCard; radius: 4 }

                    contentItem: Text {
                        // If vehicle is in a mode not in the dropdown (e.g. Return, Land),
                        // show the actual mode name
                        text: flightModeCombo.currentIndex >= 0
                              ? flightModeCombo.displayText
                              : root._currentFlightMode
                        color:             "white"
                        font.pixelSize:    12
                        font.bold:         true
                        verticalAlignment: Text.AlignVCenter
                        leftPadding:       10
                    }

                    indicator: Text {
                        text: "▼"; color: _clrMuted; font.pixelSize: 10
                        anchors.right: parent.right; anchors.rightMargin: 8; anchors.verticalCenter: parent.verticalCenter
                    }

                    popup: Popup {
                        y: flightModeCombo.height; width: flightModeCombo.width; padding: 1
                        background: Rectangle { color: _clrCard; radius: 4 }
                        contentItem: ListView { clip: true; implicitHeight: contentHeight; model: flightModeCombo.delegateModel }
                    }

                    delegate: ItemDelegate {
                        width: flightModeCombo.width; highlighted: flightModeCombo.highlightedIndex === index
                        background: Rectangle { color: highlighted ? _clrBlue : _clrCard }
                        contentItem: Text {
                            text: modelData; color: "white"; font.pixelSize: 12
                            font.bold: flightModeCombo.currentIndex === index
                            leftPadding: 10; verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }

            Item { Layout.fillWidth: true }

            // Complete Demo
            Rectangle {
                visible: demoLocked
                width: completeLbl.width + 32; height: 36; color: _clrGreen; radius: 6
                Text { id: completeLbl; anchors.centerIn: parent; text: "Complete Demo"; color: "white"; font.pixelSize: 13; font.bold: true }
                MouseArea { anchors.fill: parent; onClicked: root.unlockDemo() }
            }

            Item { Layout.fillWidth: true }

            // Return to Home — commands guidedModeRTL + unlocks demo
            Rectangle {
                width: rthLbl.width + 24; height: 34; color: _clrOrange; radius: 6
                Row {
                    anchors.centerIn: parent; spacing: 6
                    Text { text: "⌂"; color: "white"; font.pixelSize: 14; anchors.verticalCenter: parent.verticalCenter }
                    Text { id: rthLbl; text: "RETURN TO HOME"; color: "white"; font.pixelSize: 12; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (_activeVehicle) {
                            _activeVehicle.guidedModeRTL(false)
                        }
                        root.unlockDemo()
                    }
                }
            }

            // Kill Switch — commands guidedModeLand (controlled descent at current position)
            Rectangle {
                width: landLbl.width + 24; height: 34; color: _clrKill; radius: 6
                Row {
                    anchors.centerIn: parent; spacing: 6
                    Text { text: "↓"; color: "white"; font.pixelSize: 14; anchors.verticalCenter: parent.verticalCenter }
                    Text { id: landLbl; text: "KILL SWITCH"; color: "white"; font.pixelSize: 12; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (_activeVehicle) {
                            _activeVehicle.emergencyStop()
                        }
                    }
                }
            }

	    Button {
		id: modeToggle
		
		property var vehicle: _activeVehicle
		
		text: {
		    switch (vehicle.flightMode) {
		    case "Mission":
		        return "Switch to Stabilized"
		    case "Stabilized":
		        return "Switch to Auto"
		    default:
		        return "Mode: " + vehicle.flightMode
		    }
		}
		
		onClicked: {
		    if (vehicle.flightMode === "Mission") {
		        vehicle.setFlightMode("Stabilized")
		    } else {
		        vehicle.setFlightMode("Mission")
		    }
		}
	    }

            // Retrieval Mechanism Control
            Rectangle {
                width: rmcLbl.width + 24; height: 34; color: _clrCard; radius: 6
                Text { id: rmcLbl; anchors.centerIn: parent; text: "Retrieval Mechanism Control"; color: "white"; font.pixelSize: 12 }
                MouseArea { anchors.fill: parent; onClicked: console.log("Retrieval Mechanism Control") }
            }

            // Settings
            Rectangle {
                width: settLbl.width + 24; height: 34; color: _clrCard; radius: 6
                Row {
                    anchors.centerIn: parent; spacing: 6
                    Text { text: "⚙"; color: "white"; font.pixelSize: 14; anchors.verticalCenter: parent.verticalCenter }
                    Text { id: settLbl; text: "Settings"; color: "white"; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
                }
                MouseArea { anchors.fill: parent; onClicked: console.log("Settings") }
            }

            // AI/ML Asset Identification
            Rectangle {
                width: aiLbl.width + 24; height: 34; color: _clrPurple; radius: 6
                Row {
                    anchors.centerIn: parent; spacing: 6
                    Text { text: "⊙"; color: "white"; font.pixelSize: 14; anchors.verticalCenter: parent.verticalCenter }
                    Text { id: aiLbl; text: "AI/ML Asset Identification"; color: "white"; font.pixelSize: 12; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
                }
                MouseArea { anchors.fill: parent; onClicked: console.log("AI/ML") }
            }
        }
    }
}

