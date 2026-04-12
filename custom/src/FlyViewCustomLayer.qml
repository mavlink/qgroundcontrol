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
    readonly property string _clrKill:        "#c62828"  // kill switch
    readonly property string _clrMuted:       "#888888"  // secondary text
    readonly property string _clrAmber:       "#f0a500"  // current-action label

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
    // selectedFlightMode: drives both the bottom dropdown and the top bar badge
    // ─────────────────────────────────────────────────────────────────────
    property bool   isArmed:            false
    property string selectedFlightMode: "AUTO"

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

    // Flight mode options for the bottom-bar dropdown
    //   MANUAL — operator uses physical remote control
    //   AUTO   — autonomous/mission flight
    //   HOVER  — QGC fallback mode; UAV holds position when uplink is lost
    readonly property var flightModes: ["MANUAL", "AUTO", "HOVER"]

    // Point values per (color, shape) for Demo #4.
    // TODO: wire these through the Settings panel once it exists.
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

    /** Returns the point value for a given color+shape combo (Demo #4). */
    function pointsFor(color, shape) {
        return (pointTable[color] && pointTable[color][shape]) ? pointTable[color][shape] : 0
    }

    /** Locks the UI to a specific demo (0-based index). */
    function lockDemo(index) {
        selectedDemoIndex = index
        demoLocked        = true
    }

    /**
     * Releases the demo lock and resets all demo-specific state.
     * Called by Complete Demo button and Return to Home.
     */
    function unlockDemo() {
        demoLocked        = false
        selectedDemoIndex = -1
        isArmed           = false   // disarm whenever leaving a demo
        demo1Color        = "Red"
        demo2Shape        = "Cube"
        pendingColor      = "Red"
        pendingShape      = "Cube"
        assetModel.clear()
    }

    // ─────────────────────────────────────────────────────────────────────
    // DATA MODELS
    // ─────────────────────────────────────────────────────────────────────

    /** Holds added assets for Demo #3 and #4 (max 3 entries). */
    ListModel { id: assetModel }

    // ─────────────────────────────────────────────────────────────────────
    // QGC TOOL INSETS  (tells QGC how much of the screen our UI occupies)
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
    // Left:   Company name | UAV badge | Connection status
    // Center: Demo selector ComboBox (locks on selection)
    // Right:  Flight Mode badge | UTC clock
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

            // ── Left group ────────────────────────────────────────────────

            Text {
                text:           "Crown & Eagle Engineering"
                color:          "white"
                font.pixelSize: 14
                font.bold:      true
            }

            // UAV identifier badge
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

            // Connection status indicator
            Row {
                spacing: 6
                Rectangle {
                    width:                  8
                    height:                 8
                    radius:                 4
                    color:                  _activeVehicle ? _clrGreen : _clrRed
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

            // ── Center: Demo selector ─────────────────────────────────────
            // Disabled (greyed out) once a demo is locked.
            // Unlocks only via Complete Demo or Return to Home.
            ComboBox {
                id:             demoComboBox
                enabled:        !demoLocked
                model:          ["Select Demonstration…"].concat(root.demoNames)
                currentIndex:   root.selectedDemoIndex + 1  // offset: index 0 = placeholder
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

                // Hide the dropdown arrow when locked (selection is frozen)
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

            // ── Right group ───────────────────────────────────────────────

            // Flight mode badge (mirrors the bottom-bar dropdown)
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
                        text:             selectedFlightMode
                        color:            "white"
                        font.pixelSize:   12
                        font.bold:        true
                    }
                }
            }

            // UTC clock (HH:MM:SS, updates every second)
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
    // • UAV identity and status
    // • Mission name (reflects locked demo)
    // • Mission phase + elapsed timer
    // • Demo-specific controls (appear only when a demo is locked)
    // • Mission planning — waypoint list + Continue/Pause
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

            // ── UAV identity ──────────────────────────────────────────────
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

            // ── Mission name ──────────────────────────────────────────────
            // Displays the locked demo name; shows a dash when no demo is active.
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

            // ── Mission phase + elapsed timer ─────────────────────────────
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

            // ── Demo-specific controls (visible only when a demo is locked) ──
            //    The divider and each block are individually gated on demoLocked.

            Rectangle {
                Layout.fillWidth: true
                height:           1
                color:            _clrCard
                visible:          demoLocked
            }

            // Demo #1 — single color dropdown
            Column {
                Layout.fillWidth: true
                spacing:          6
                visible:          demoLocked && selectedDemoIndex === 0

                Text { text: "Target Color"; color: _clrMuted; font.pixelSize: 11 }

                ComboBox {
                    id:           d1Color
                    model:        root.colorOptions
                    width:        236
                    currentIndex: root.colorOptions.indexOf(root.demo1Color)
                    onActivated:  function(i) { root.demo1Color = root.colorOptions[i] }

                    background: Rectangle { color: _clrCard; radius: 4 }
                    contentItem: Text {
                        text: d1Color.displayText; color: "white"; font.pixelSize: 12
                        verticalAlignment: Text.AlignVCenter; leftPadding: 10
                    }
                    popup: Popup {
                        y: d1Color.height; width: d1Color.width; padding: 1
                        background: Rectangle { color: _clrCard; radius: 4 }
                        contentItem: ListView {
                            clip: true; implicitHeight: contentHeight
                            model: d1Color.delegateModel
                        }
                    }
                    delegate: ItemDelegate {
                        width: d1Color.width; highlighted: d1Color.highlightedIndex === index
                        background: Rectangle { color: highlighted ? "#444" : _clrCard }
                        contentItem: Text { text: modelData; color: "white"; font.pixelSize: 12; leftPadding: 10; verticalAlignment: Text.AlignVCenter }
                    }
                }
            }

            // Demo #2 — single shape dropdown
            Column {
                Layout.fillWidth: true
                spacing:          6
                visible:          demoLocked && selectedDemoIndex === 1

                Text { text: "Target Shape"; color: _clrMuted; font.pixelSize: 11 }

                ComboBox {
                    id:           d2Shape
                    model:        root.shapeOptions
                    width:        236
                    currentIndex: root.shapeOptions.indexOf(root.demo2Shape)
                    onActivated:  function(i) { root.demo2Shape = root.shapeOptions[i] }

                    background: Rectangle { color: _clrCard; radius: 4 }
                    contentItem: Text {
                        text: d2Shape.displayText; color: "white"; font.pixelSize: 12
                        verticalAlignment: Text.AlignVCenter; leftPadding: 10
                    }
                    popup: Popup {
                        y: d2Shape.height; width: d2Shape.width; padding: 1
                        background: Rectangle { color: _clrCard; radius: 4 }
                        contentItem: ListView {
                            clip: true; implicitHeight: contentHeight
                            model: d2Shape.delegateModel
                        }
                    }
                    delegate: ItemDelegate {
                        width: d2Shape.width; highlighted: d2Shape.highlightedIndex === index
                        background: Rectangle { color: highlighted ? "#444" : _clrCard }
                        contentItem: Text { text: modelData; color: "white"; font.pixelSize: 12; leftPadding: 10; verticalAlignment: Text.AlignVCenter }
                    }
                }
            }

            // Demo #3 & #4 — multi-asset management
            // Add up to 3 assets (color + shape). Demo #4 also shows point values.
            Column {
                Layout.fillWidth: true
                spacing:          8
                visible:          demoLocked && (selectedDemoIndex === 2 || selectedDemoIndex === 3)

                Text { text: "Add Asset (max 3)"; color: _clrMuted; font.pixelSize: 11 }

                // ── Add row: color picker + shape picker + [+] button ─────
                Row {
                    spacing: 6

                    ComboBox {
                        id:           assetColorPicker
                        model:        root.colorOptions
                        width:        104
                        currentIndex: root.colorOptions.indexOf(root.pendingColor)
                        onActivated:  function(i) { root.pendingColor = root.colorOptions[i] }
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
                        id:           assetShapePicker
                        model:        root.shapeOptions
                        width:        88
                        currentIndex: root.shapeOptions.indexOf(root.pendingShape)
                        onActivated:  function(i) { root.pendingShape = root.shapeOptions[i] }
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

                    // Add button — greyed out when at 3 assets
                    Rectangle {
                        width: 30; height: 28
                        color:  assetModel.count < 3 ? _clrGreen : "#555"
                        radius: 4
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

                // ── Asset list ────────────────────────────────────────────
                Column {
                    spacing: 4
                    width:   236

                    Repeater {
                        model: assetModel
                        delegate: Rectangle {
                            width: 236; height: 30; color: _clrCard; radius: 4

                            // Color / Shape label
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left:           parent.left
                                anchors.leftMargin:     10
                                text:                   model.assetColor + " / " + model.assetShape
                                color:                  "white"
                                font.pixelSize:         11
                            }

                            // Point value (Demo #4 only)
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.right:          removeBtn.left
                                anchors.rightMargin:    8
                                visible:                selectedDemoIndex === 3
                                text:                   model.points + " pts"
                                color:                  _clrGreen
                                font.pixelSize:         11
                            }

                            // Remove (×) button
                            Rectangle {
                                id:                     removeBtn
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.right:          parent.right
                                anchors.rightMargin:    8
                                width: 20; height: 20; color: _clrKill; radius: 3
                                Text { anchors.centerIn: parent; text: "×"; color: "white"; font.pixelSize: 14 }
                                MouseArea { anchors.fill: parent; onClicked: assetModel.remove(index) }
                            }
                        }
                    }
                }

                // ── Demo #4 running point total ───────────────────────────
                Rectangle {
                    width:   236
                    height:  32
                    color:   _clrCard
                    radius:  4
                    visible: selectedDemoIndex === 3 && assetModel.count > 0

                    Row {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left:           parent.left
                        anchors.leftMargin:     10
                        spacing:                8

                        Text { text: "Total Points:"; color: _clrMuted; font.pixelSize: 12 }

                        // Reactive sum — re-evaluated whenever assetModel.count changes
                        Text {
                            color:          _clrGreen
                            font.pixelSize: 14
                            font.bold:      true
                            text: {
                                var sum = 0
                                for (var i = 0; i < assetModel.count; i++) sum += assetModel.get(i).points
                                return sum
                            }
                        }
                    }
                }
            }

            // ── Mission planning section ──────────────────────────────────
            Rectangle { Layout.fillWidth: true; height: 1; color: _clrCard }

            // Section header + waypoint counter badge
            Row {
                spacing: 6
                Text {
                    text:                   "Mission Planning"
                    color:                  "white"
                    font.pixelSize:         13
                    font.bold:              true
                    anchors.verticalCenter: parent.verticalCenter
                }
                Item { width: 8 }
                Rectangle {
                    color: _clrCard; radius: 4
                    width: wpBadge.width + 10; height: 20
                    Text { id: wpBadge; anchors.centerIn: parent; text: "2 / 6"; color: _clrMuted; font.pixelSize: 11 }
                }
            }

            // Current action card
            Rectangle {
                Layout.fillWidth: true
                color:  _clrCard
                radius: 4
                height: actionCol.height + 16

                Column {
                    id:              actionCol
                    anchors.left:    parent.left
                    anchors.right:   parent.right
                    anchors.top:     parent.top
                    anchors.margins: 8
                    spacing:         4

                    Text { text: "CURRENT ACTION"; color: _clrAmber; font.pixelSize: 10; font.bold: true }
                    Text {
                        text:           "Continue the mission from the current waypoint"
                        color:          "white"
                        font.pixelSize: 11
                        wrapMode:       Text.WordWrap
                        width:          parent.width
                    }
                    Text { text: "Slide or hold spacebar to confirm"; color: _clrMuted; font.pixelSize: 10 }
                }
            }

            // Waypoint list — fills remaining vertical space
            ListView {
                id:                waypointList
                Layout.fillWidth:  true
                Layout.fillHeight: true
                clip:              true
                spacing:           4

                model: ListModel {
                    ListElement { name: "Takeoff";    distance: "0 m";  status: "done"    }
                    ListElement { name: "Waypoint 1"; distance: "50 m"; status: "done"    }
                    ListElement { name: "Waypoint 2"; distance: "50 m"; status: "active"  }
                    ListElement { name: "Waypoint 3"; distance: "50 m"; status: "pending" }
                    ListElement { name: "Return";     distance: "30 m"; status: "pending" }
                }

                delegate: Rectangle {
                    width:  waypointList.width
                    height: 36
                    color:  status === "active" ? "#1a3a5c" : "transparent"
                    radius: 4

                    Row {
                        anchors.fill:        parent
                        anchors.leftMargin:  8
                        anchors.rightMargin: 8
                        spacing:             8

                        // Status circle
                        Rectangle {
                            width: 18; height: 18; radius: 9
                            anchors.verticalCenter: parent.verticalCenter
                            color:        status === "done"    ? _clrGreen : status === "active" ? _clrBlue : "transparent"
                            border.color: status === "pending" ? _clrMuted : "transparent"
                            border.width: 1
                            Text { anchors.centerIn: parent; text: status === "done" ? "✓" : ""; color: "white"; font.pixelSize: 10 }
                        }

                        Text {
                            text:                   name
                            color:                  status === "active" ? "white" : _clrMuted
                            font.pixelSize:         12
                            font.bold:              status === "active"
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Text {
                            text:                   distance
                            color:                  _clrMuted
                            font.pixelSize:         11
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }

            // Continue Mission (green) + Pause Mission (gray outline)
            Column {
                spacing:          8
                Layout.fillWidth: true

                Rectangle {
                    width: 236; height: 40; color: _clrGreen; radius: 6
                    Row {
                        anchors.centerIn: parent; spacing: 8
                        Text { text: "▶"; color: "white"; font.pixelSize: 14; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: "Continue Mission"; color: "white"; font.pixelSize: 13; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
                    }
                    MouseArea { anchors.fill: parent; onClicked: console.log("Continue Mission") }
                }

                Rectangle {
                    width: 236; height: 36; color: "transparent"; radius: 6
                    border.color: _clrMuted; border.width: 1
                    Text { anchors.centerIn: parent; text: "Pause Mission"; color: _clrMuted; font.pixelSize: 13 }
                    MouseArea { anchors.fill: parent; onClicked: console.log("Pause Mission") }
                }
            }
        }
    }


    // =========================================================================
    // RIGHT TELEMETRY PANEL  (260 px)
    // Stacked cards: speed, heading, altitude, lat, lon, UTC time, battery
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

            Repeater {
                model: [
                    { icon: "⟳", label: "SPEED",
                      value: _activeVehicle ? _activeVehicle.groundSpeed.value.toFixed(1) : "12.5",
                      unit: "m/s", extra: "" },
                    { icon: "⊙", label: "HEADING",
                      value: _activeVehicle ? _activeVehicle.heading.rawValue.toFixed(0) : "245",
                      unit: "°", extra: "" },
                    { icon: "▲", label: "ALTITUDE",
                      value: _activeVehicle ? _activeVehicle.altitudeRelative.value.toFixed(1) : "850.0",
                      unit: "m", extra: "AGL" },
                    { icon: "⊕", label: "LATITUDE",
                      value: _activeVehicle ? _activeVehicle.coordinate.latitude.toFixed(4) : "40.7128",
                      unit: "° N", extra: "" },
                    { icon: "⊕", label: "LONGITUDE",
                      value: _activeVehicle ? _activeVehicle.coordinate.longitude.toFixed(4) : "74.0060",
                      unit: "° W", extra: "" },
                    { icon: "⏱", label: "TIME",
                      value: "00:00:00 UTC", unit: "", extra: "" },
                    { icon: "⚡", label: "BATTERY STATUS",
                      value: _activeVehicle ? _activeVehicle.battery.percentRemaining.value + "%" : "100%",
                      unit: "", extra: "GOOD" }
                ]

                delegate: Rectangle {
                    width: 236; height: 60; color: _clrCard; radius: 6

                    Row {
                        anchors.fill:    parent
                        anchors.margins: 10
                        spacing:         8

                        Text {
                            text:                   modelData.icon
                            color:                  _clrMuted
                            font.pixelSize:         14
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Column {
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 2

                            Text { text: modelData.label; color: _clrMuted; font.pixelSize: 10 }

                            Row {
                                spacing: 6
                                Text { text: modelData.value + modelData.unit; color: "white"; font.pixelSize: 18; font.bold: true }
                                Text {
                                    text:                   modelData.extra
                                    color:                  _clrGreen
                                    font.pixelSize:         10
                                    anchors.verticalCenter: parent.verticalCenter
                                    visible:                modelData.extra !== ""
                                }
                            }
                        }
                    }
                }
            }
        }
    }


    // =========================================================================
    // COMMAND STRIP  (bottom bar)
    // Left:   ARM | "Vehicle is disarmed" | Flight Mode dropdown
    // Center: Complete Demo (green, only when demo is locked)
    // Right:  Return to Home | Kill Switch | Retrieval Mech | Settings | AI/ML
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

            // ── Left group ────────────────────────────────────────────────

            // ARM / DISARM toggle
            // Enabled only during Demo #4. Grayed-out and non-interactive otherwise.
            Rectangle {
                id:     armButton
                width:  armLbl.width + 24; height: 34
                radius: 6

                // Armed → green fill; disarmed → transparent with gray border
                color:        isArmed ? _clrGreen : "transparent"
                border.color: isArmed ? "transparent" : (_canArm ? _clrMuted : "#444")
                border.width: 1
                opacity:      _canArm ? 1.0 : 0.4     // dim when not in Demo #4

                Row {
                    anchors.centerIn: parent; spacing: 6
                    Text {
                        text:                   isArmed ? "✓" : "⊙"
                        color:                  "white"
                        font.pixelSize:         14
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        id:                     armLbl
                        text:                   isArmed ? "DISARM" : "ARM"
                        color:                  "white"
                        font.pixelSize:         13
                        font.bold:              true
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    enabled:      _canArm
                    onClicked:    isArmed = !isArmed
                    cursorShape:  _canArm ? Qt.PointingHandCursor : Qt.ArrowCursor
                }
            }

            // Status text — "Vehicle is disarmed" when not armed, blank when armed
            Text {
                text:           isArmed ? "" : "Vehicle is disarmed"
                color:          _clrMuted
                font.pixelSize: 12
            }

            // Flight Mode dropdown — selection syncs to the top bar badge
            Row {
                spacing: 6
                Text { text: "Flight Mode:"; color: "white"; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }

                ComboBox {
                    id:             flightModeCombo
                    model:          root.flightModes
                    currentIndex:   root.flightModes.indexOf(root.selectedFlightMode)
                    implicitWidth:  110
                    implicitHeight: 34

                    onActivated: function(i) { root.selectedFlightMode = root.flightModes[i] }

                    background: Rectangle { color: _clrCard; radius: 4 }

                    contentItem: Text {
                        text:              flightModeCombo.displayText
                        color:             "white"
                        font.pixelSize:    12
                        font.bold:         true
                        verticalAlignment: Text.AlignVCenter
                        leftPadding:       10
                    }

                    indicator: Text {
                        text:                   "▼"
                        color:                  _clrMuted
                        font.pixelSize:         10
                        anchors.right:          parent.right
                        anchors.rightMargin:    8
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    popup: Popup {
                        y:       flightModeCombo.height
                        width:   flightModeCombo.width
                        padding: 1
                        background: Rectangle { color: _clrCard; radius: 4 }
                        contentItem: ListView {
                            clip:           true
                            implicitHeight: contentHeight
                            model:          flightModeCombo.delegateModel
                        }
                    }

                    delegate: ItemDelegate {
                        width:       flightModeCombo.width
                        highlighted: flightModeCombo.highlightedIndex === index
                        background: Rectangle { color: highlighted ? _clrBlue : _clrCard }
                        contentItem: Text {
                            text:              modelData
                            color:             "white"
                            font.pixelSize:    12
                            font.bold:         flightModeCombo.currentIndex === index
                            leftPadding:       10
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }

            Item { Layout.fillWidth: true }

            // ── Center: Complete Demo ─────────────────────────────────────
            // Visible only when a demo is locked. Clicking resets demo state.
            Rectangle {
                visible: demoLocked
                width:   completeLbl.width + 32; height: 36
                color:   _clrGreen; radius: 6
                Text {
                    id:               completeLbl
                    anchors.centerIn: parent
                    text:             "Complete Demo"
                    color:            "white"
                    font.pixelSize:   13
                    font.bold:        true
                }
                MouseArea { anchors.fill: parent; onClicked: root.unlockDemo() }
            }

            Item { Layout.fillWidth: true }

            // ── Right group ───────────────────────────────────────────────

            // Return to Home — also unlocks any active demo
            Rectangle {
                width: rthLbl.width + 24; height: 34; color: _clrOrange; radius: 6
                Row {
                    anchors.centerIn: parent; spacing: 6
                    Text { text: "⌂"; color: "white"; font.pixelSize: 14; anchors.verticalCenter: parent.verticalCenter }
                    Text { id: rthLbl; text: "RETURN TO HOME"; color: "white"; font.pixelSize: 12; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
                }
                MouseArea { anchors.fill: parent; onClicked: { console.log("RTH"); root.unlockDemo() } }
            }

            // Kill Switch
            Rectangle {
                width: ksLbl.width + 24; height: 34; color: _clrKill; radius: 6
                Row {
                    anchors.centerIn: parent; spacing: 6
                    Text { text: "↓"; color: "white"; font.pixelSize: 14; anchors.verticalCenter: parent.verticalCenter }
                    Text { id: ksLbl; text: "KILL SWITCH"; color: "white"; font.pixelSize: 12; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
                }
                MouseArea { anchors.fill: parent; onClicked: console.log("Kill Switch") }
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
