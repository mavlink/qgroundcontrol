import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts
import QGroundControl
import QGroundControl.Controls
import QGroundControl.Palette
import QGroundControl.ScreenTools
import Custom.Widgets
import QGroundControl.Toolbar
import Qt.labs.folderlistmodel 2.15

Item {
    id: _root
    anchors.fill: parent

    property var parentToolInsets
    property var mapControl

    property var totalToolInsets: _totalToolInsets

    QGCToolInsets {
        id: _totalToolInsets
    }

    // ══════════════════════════════════════
    // RESPONSIVE SCALE (1.0 at 1920x1080)
    // ══════════════════════════════════════
    readonly property real _u: Math.max(0.7, Math.min(1.4, Math.min(width / 1920, height / 1080)))

    // Font sizes
    readonly property int _fsXS: Math.max(9,  Math.round(10 * _u))
    readonly property int _fsS:  Math.max(10, Math.round(11 * _u))
    readonly property int _fsM:  Math.max(11, Math.round(13 * _u))
    readonly property int _fsL:  Math.max(13, Math.round(15 * _u))
    readonly property int _fsXL: Math.max(15, Math.round(18 * _u))

    // Colors
    readonly property color _cPanel:  "#111E2B"
    readonly property color _cCard:   "#0D151F"
    readonly property color _cBorder: "#263649"
    readonly property color _cHover:  "#1A2A3A"
    readonly property color _cLine:   "#1E2C3C"
    readonly property color _cText:   "#F1F5F9"
    readonly property color _cSoft:   "#B3C0D0"
    readonly property color _cMuted:  "#8191A5"
    readonly property color _cAccent: "#4DA3FF"
    readonly property color _cGood:   "#27BF89"
    readonly property color _cWarn:   "#F7B24A"
    readonly property color _cBad:    "#E1544C"

    // Layout sizes
    readonly property real _sidebarW_Collapsed: Math.max(56,  Math.round(70 * _u))
    readonly property real _sidebarW_Expanded:  Math.max(180, Math.round(220 * _u))
    readonly property real _leftPanelW:  Math.max(250, Math.min(width * 0.20, 440))
    readonly property real _rightPanelW: Math.max(250, Math.min(width * 0.20, 440))
    readonly property real _footerBaseH: Math.max(170, Math.min(height * 0.28, 320))   // normal height, also the minimum
    readonly property real _footerMaxH:  Math.max(_footerBaseH, height * 0.70)         // 70% of the window
    property real _footerExtra: 0                                                      // extra height added by dragging
    readonly property real _footerH: Math.min(_footerMaxH, _footerBaseH + _footerExtra)
    readonly property color _boxBorder: _cBorder

    readonly property real customLeftInset: 0
    readonly property real customRightInset: 0
    readonly property real customBottomInset: _footerbaseH
    readonly property real customTopInset: topNavbar.height

    property var   _activeVehicle:      QGroundControl.multiVehicleManager.activeVehicle
    property bool  _communicationLost:  _activeVehicle ? _activeVehicle.vehicleLinkManager.communicationLost : false
    property color _mainStatusBGColor:  "transparent"

    property real _heading:           _activeVehicle ? _activeVehicle.heading.rawValue : 0
    property real _indicatorsHeight:  ScreenTools.defaultFontPixelHeight

    property var _battery: (_activeVehicle && _activeVehicle.batteries.count > 0) ? _activeVehicle.batteries.get(0) : null
    QGCPalette { id: qgcPal }



    function _fmt(fact, digits) {
        if (!fact) return qsTr("--")
        var v = fact.value
        if (v === undefined || isNaN(v)) return qsTr("--")
        return v.toFixed(digits) + " " + fact.units
    }

    function _findbattery() {
        if (!_activeVehicle) return null
        if (_activeVehicle.battery) return _activeVehicle.battery
        if (_activeVehicle.batteries && _activeVehicle.batteries.count > 0) return _activeVehicle.batteries.get(0)
        return null
    }

    function _statusValue(key) {
        if (!_activeVehicle) {
            return key === "connection" ? qsTr("Disconnected") : qsTr("No status")
        }
        var b = _findbattery()
        switch (key) {
        case "connection": return _communicationLost ? qsTr("Comms Lost") : qsTr("Connected")
        case "type":
            if (_activeVehicle.vtol)       return qsTr("VTOL")
            if (_activeVehicle.fixedWing)  return qsTr("Fixed Wing")
            if (_activeVehicle.multiRotor) return qsTr("Multirotor")
            if (_activeVehicle.rover)      return qsTr("Rover")
            if (_activeVehicle.sub)        return qsTr("Submarine")
            return qsTr("--")
        case "mode":       return _activeVehicle.flightMode
        case "armed":      return _activeVehicle.armed ? qsTr("Yes") : qsTr("No")
        case "battery":    return b ? _fmt(b.percentRemaining, 0) : qsTr("--")
        case "gps":        return qsTr("HDOP ") + _activeVehicle.gps.hdop.rawValue.toFixed(1)
        case "sats":       return _activeVehicle.gps.count.rawValue
        case "alt":        return _fmt(_activeVehicle.altitudeRelative, 1)
        case "speed":      return _fmt(_activeVehicle.groundSpeed, 1)
        case "heading":    return _activeVehicle.heading.rawValue.toFixed(0) + "°"
        case "lat": return _activeVehicle.coordinate.isValid ? _activeVehicle.coordinate.latitude.toFixed(6)  : qsTr("--")
        case "lon": return _activeVehicle.coordinate.isValid ? _activeVehicle.coordinate.longitude.toFixed(6) : qsTr("--")
        }
        return qsTr("--")
    }

    // Visual only: color of each status value
    function _statusColor(key) {
        if (!_activeVehicle) return key === "connection" ? _cWarn : _cMuted
        switch (key) {
        case "connection": return _communicationLost ? _cBad : _cGood
        case "armed":      return _activeVehicle.armed ? _cWarn : _cText
        case "mode":       return _cAccent
        }
        return _cText
    }

    // ---- Mission summary (footer right) ----
    property var  _missionCtrl: (parent && parent.planController) ? parent.planController.missionController : null
    property real _mbPerPhoto:  14.4    // average size of one photo in MB, change to match your camera

    function _surveyItem() {
        if (!_missionCtrl || !_missionCtrl.visualItems) return null
        var items = _missionCtrl.visualItems
        for (var i = 0; i < items.count; i++) {
            var it = items.get(i)
            if (it && it.cameraShots !== undefined) return it
        }
        return null
    }

    function _fmtDist(m) {
        if (m === undefined || isNaN(m) || m <= 0) return qsTr("--")
        return m >= 1000 ? (m / 1000).toFixed(2) + " km" : Math.round(m) + " m"
    }

    function _fmtTime(s) {
        if (s === undefined || isNaN(s) || s <= 0) return qsTr("--")
        return Math.floor(s / 60) + "m " + Math.round(s % 60) + "s"
    }

    function _summaryValue(key) {
        var mc = _missionCtrl
        var sv = _surveyItem()
        switch (key) {
        case "gsd":      return (sv && sv.cameraCalc && sv.cameraCalc.imageDensity) ? _fmt(sv.cameraCalc.imageDensity, 1) : qsTr("--")
        case "photos":   return sv ? sv.cameraShots : qsTr("--")
        case "lines":    return (sv && sv.visualTransectPoints) ? Math.round(sv.visualTransectPoints.length / 2) : qsTr("--")
        case "distance": return mc ? _fmtDist(mc.missionDistance) : qsTr("--")
        case "time":     return mc ? _fmtTime(mc.missionTime) : qsTr("--")
        case "area":     return (sv && sv.coveredArea > 0) ? Math.round(sv.coveredArea) + " m²" : qsTr("--")
        case "data":     return (sv && sv.cameraShots > 0) ? Math.round(sv.cameraShots * _mbPerPhoto) + " MB" : qsTr("--")
        }
        return qsTr("--")
    }

    function _firstMode(names) {
        if (!_activeVehicle || !_activeVehicle.flightModes) return ""
        for (var i = 0; i < names.length; i++) {
            if (_activeVehicle.flightModes.indexOf(names[i]) >= 0) return names[i]
        }
        return ""
    }
    function _pauseModeName()   { return _firstMode(["Hold", "Brake", "Loiter", "Pause"]) }
    function _missionModeName() { return _firstMode(["Mission", "Auto"]) }
    function _isPaused()        { return !!_activeVehicle && _pauseModeName()   !== "" && _activeVehicle.flightMode === _pauseModeName() }
    function _isMissionMode()   { return !!_activeVehicle && _missionModeName() !== "" && _activeVehicle.flightMode === _missionModeName() }


    // ---- Footer: telemetry page + drone control helpers ----
    readonly property bool _canMove: !!_activeVehicle && _activeVehicle.armed && _activeVehicle.flying

    function _telUnit(key) {
        return (key === "vs" || key === "gs") ? "m/s" : (key === "time" ? "" : "m")
    }

    function _tel(key) {
        if (key === "time")
            return { v: _activeVehicle ? _activeVehicle.flightTime.valueString : "00:00:00", u: "" }
        var f = null
        if (_activeVehicle) {
            switch (key) {
            case "alt":  f = _activeVehicle.altitudeRelative; break
            case "vs":   f = _activeVehicle.climbRate;        break
            case "msl":  f = _activeVehicle.altitudeAMSL;     break
            case "gs":   f = _activeVehicle.groundSpeed;      break
            case "home": f = _activeVehicle.distanceToHome;   break
            }
        }
        if (!f || f.value === undefined || isNaN(f.value))
            return { v: "0.0", u: _telUnit(key) }
        return { v: f.value.toFixed(1), u: f.units }
    }

    // bearingOffset: 0 = forward, 90 = right, -90 = left, 180 = back (relative to vehicle heading)
    function _moveVehicle(bearingOffset, meters) {
        if (!_canMove || !_activeVehicle.coordinate.isValid) return
        var az = (_heading + bearingOffset + 360) % 360
        _activeVehicle.guidedModeGotoLocation(_activeVehicle.coordinate.atDistanceAndAzimuth(meters, az))
    }

    function _changeAlt(delta) {
        if (!_canMove) return
        _activeVehicle.guidedModeChangeAltitude(delta, false)
    }

    function _openStatusTab() {
        for (var i = 0; i < missionCard._tabs.length; i++) {
            if (missionCard._tabs[i].key === "status") { missionCard._tab = i; break }
        }
        // OPTIONAL, only if you added the drag-resize footer: open it to about half the window
        _footerExtra = Math.max(_footerExtra, Math.max(0, Math.min(_footerMaxH, height * 0.5) - _footerBaseH))
    }

    // ══════════════════════════════════════
    // 0. TOP NAVBAR
    // ══════════════════════════════════════
    Rectangle {
        id: topNavbar
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width
        height: Math.max(44, Math.round(52 * _root._u))
        color: _cCard
        border.color: _cBorder
        border.width: 1
        z: 10

        // Hamburger (opens the slide-in menu)
        Rectangle {
            id: burger
            readonly property real _s: Math.round(parent.height * 0.7)
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            width: _s
            height: _s
            radius: 8
            color: burgerMouse.containsMouse ? _cHover : "transparent"
            border.color: burgerMouse.containsMouse ? _cBorder : "transparent"
            border.width: 1

            Column {
                anchors.centerIn: parent
                spacing: 5
                Repeater {
                    model: 3
                    Rectangle {
                        width: Math.round(burger._s * 0.5)
                        height: 2
                        radius: 1
                        color: _cText
                    }
                }
            }

            MouseArea {
                id: burgerMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: drawer.open()
            }
        }

        // Logo
        Image {
            id: navLogo
            readonly property real _size: Math.round(parent.height * 0.7)
            anchors.left: burger.right
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            width: _size
            height: _size
            source: "/Custom/res/QGCLogoFull.svg"
            fillMode: Image.PreserveAspectFit
        }

        MainStatusIndicator {
            anchors.left:       navLogo.right
            anchors.leftMargin: 15
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
        }

        MouseArea {
            anchors.fill: mainStatus
            enabled: !!_root._activeVehicle
            cursorShape: Qt.PointingHandCursor
            onClicked: _root._openStatusTab()
        }
    }

    // ══════════════════════════════════════
    // 1. HAMBURGER (map top-left) + SLIDE-IN MENU
    // ══════════════════════════════════════





    // Slide-in menu (drawn in the window overlay, above the map and all its widgets)
    Popup {
        id: drawer
        parent: Overlay.overlay
        x: 0
        y: 0
        width: _sidebarW_Expanded
        height: parent ? parent.height : _root.height
        modal: true            // the outside click only closes the menu
        dim: false             // map stays fully visible
        padding: 0
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        // slide in from the left
        enter: Transition {
            NumberAnimation { property: "x"; from: -drawer.width; to: 0; duration: 220; easing.type: Easing.OutCubic }
        }
        exit: Transition {
            NumberAnimation { property: "x"; from: 0; to: -drawer.width; duration: 180; easing.type: Easing.InCubic }
        }


        background: Rectangle {
            color: _cCard
            border.color: _cBorder
            border.width: 1
        }

        contentItem: Item {

            // Header: logo + close button
            Item {
                id: drawerHeader
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: topNavbar.height

                Image {
                    id: logo
                    readonly property real _size: Math.max(32, Math.round(40 * _root._u))
                    anchors.left: parent.left
                    anchors.leftMargin: 14
                    anchors.verticalCenter: parent.verticalCenter
                    width: _size
                    height: _size
                    source: "/Custom/res/QGCLogoFull.svg"
                    fillMode: Image.PreserveAspectFit
                }


            }

            Rectangle {
                id: drawerDivider
                anchors.top: drawerHeader.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                height: 1
                color: _cLine
            }

            // Navigation items: icon + name
            Column {
                anchors.top: drawerDivider.bottom
                anchors.topMargin: Math.round(12 * _root._u)
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: Math.round(8 * _root._u)

                Repeater {
                    model: [
                        { text: "Plan Flight",    iconSource: "/qmlimages/Plan.svg",    page: "PlanView" },
                        { text: "Analyze Tools",  iconSource: "/qmlimages/Analyze.svg", page: "Analyzeview" },
                        { text: "Vehicle Config", iconSource: "/qmlimages/Gears.svg",   page: "VehicleSetupView" },
                        { text: "Settings",       iconSource: "/res/gear-white.svg",    page: "AppSettingsView" }
                    ]

                    delegate: Rectangle {
                        width: parent.width
                        height: Math.max(38, Math.round(45 * _root._u))
                        radius: 6
                        color: navMouse.containsMouse ? _cHover : "transparent"
                        Behavior on color { ColorAnimation { duration: 120 } }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            width: 3
                            height: parent.height * 0.55
                            radius: 2
                            color: _cText
                            visible: navMouse.containsMouse
                        }

                        Row {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 14
                            spacing: 14

                            Image {
                                source: modelData.iconSource
                                width: Math.max(20, Math.round(24 * _root._u))
                                height: width
                                fillMode: Image.PreserveAspectFit
                            }

                            Text {
                                text: modelData.text
                                color: _cText
                                font.pixelSize: _fsM
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        MouseArea {
                            id: navMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                drawer.close()
                                if (modelData.page === "PlanView")               mainWindow.showPlanView()
                                else if (modelData.page === "Analyzeview")       mainWindow.showAnalyzeTool()
                                else if (modelData.page === "VehicleSetupView")  mainWindow.showVehicleConfig()
                                else if (modelData.page === "AppSettingsView")   mainWindow.showSettingsTool()
                            }
                        }
                    }
                }
            }
        }
    }


    // ══════════════════════════════════════
    // 4. FOOTER
    // ══════════════════════════════════════
    Rectangle {
        id: footer
        height: _root._footerH
        anchors.left:   parent.left
        anchors.leftMargin: 0
        anchors.right:  parent.right
        anchors.bottom: parent.bottom
        color: _cCard
        border.color: _boxBorder
        border.width: 1
        z: 5

        property string _actionFeedback: _activeVehicle ? "Ready" : "Vehicle Disconnected"

        // Stops clicks and scrolling from reaching the map under the footer
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.AllButtons
            onWheel: (wheel) => { wheel.accepted = true }
        }

        // Drag handle on the top edge: drag up = taller, drag down = shorter, double-click = reset
        Item {
            id: resizeHandle
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 14
            z: 20

            Rectangle {
                anchors.centerIn: parent
                width: 56
                height: 4
                radius: 2
                color: (dragArea.containsMouse || dragArea.pressed) ? _cAccent : _cMuted
            }

            MouseArea {
                id: dragArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.SizeVerCursor

                property real _startY: 0
                property real _startExtra: 0

                onPressed: (mouse) => {
                    _startY = mapToItem(_root, mouse.x, mouse.y).y
                    _startExtra = _root._footerExtra
                }
                onPositionChanged: (mouse) => {
                    if (!pressed) return
                    var y = mapToItem(_root, mouse.x, mouse.y).y
                    var extra = _startExtra + (_startY - y)
                    _root._footerExtra = Math.max(0, Math.min(_root._footerMaxH - _root._footerBaseH, extra))
                }
                onDoubleClicked: _root._footerExtra = 0
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 8

            // ══════════════════════════════════════════
            // LEFT: Recent Missions & Live Mission Info
            // ══════════════════════════════════════════
            Rectangle {
                id: missionCard
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: footer.width * 0.45
                color: _cPanel
                radius: 8
                border.color: _cBorder
                border.width: 1
                clip: true

                // 0 = default telemetry page, 1..6 = embedded pages
                property int _tab: 0
                readonly property var _tabs: [
                    { text: "Fly View",   key: "fly" },
                    { text: "Status",     key:"status"},
                    { text: "Remote ID",  key: "remoteid" },
                    { text: "Parameters", key: "params" },
                    { text: "Console",    key: "console" },
                    { text: "Analyze",    key: "analyze" },
                    { text: "Comm Links", key: "links" },
                    { text: "Missions",   key: "missions" }
                ]
                readonly property string _tabKey: _tabs[_tab].key

                function _pageComponent() {
                    switch (_tabKey) {
                    case "remoteid": return remoteIdComp
                    case "params":   return paramsComp
                    case "console":  return consoleComp
                    case "analyze":  return analyzeComp
                    case "links":    return linksComp
                    case "missions": return missionsComp
                    case "status":   return statusComp
                    }
                    return null
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 6

                    // ---- Tab bar (scrollable) ----
                    ListView {
                        id: tabStrip
                        readonly property real _tabH: Math.max(26, Math.round(30 * _root._u))
                        Layout.fillWidth: true
                        Layout.preferredHeight: _tabH + 6
                        orientation: ListView.Horizontal
                        spacing: 4
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds
                        model: missionCard._tabs

                        // mouse wheel scrolls the tabs sideways
                        WheelHandler {
                            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                            onWheel: (event) => {
                                var d = event.angleDelta.y !== 0 ? event.angleDelta.y : event.angleDelta.x
                                tabStrip.contentX = Math.max(0, Math.min(tabStrip.contentWidth - tabStrip.width, tabStrip.contentX - d))
                            }
                        }

                        ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

                        delegate: Rectangle {
                            readonly property bool _sel: index === missionCard._tab
                            width: tabText.implicitWidth + 28
                            height: tabStrip._tabH
                            radius: 5
                            color: _sel ? _cCard : (tabMouse.containsMouse ? _cHover : "transparent")
                            border.color: _sel ? _cAccent : _cBorder
                            border.width: 1

                            Label {
                                id: tabText
                                anchors.centerIn: parent
                                text: modelData.text
                                color: parent._sel ? _cText : _cMuted
                                font.pixelSize: _fsS
                                font.weight: Font.DemiBold
                            }

                            MouseArea {
                                id: tabMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: missionCard._tab = index
                            }
                        }
                    }

                    // ---- Page area ----
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true

                        // DEFAULT PAGE: live telemetry tiles
                        GridLayout {
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            height: Math.min(parent.height, _root._footerBaseH - 42 - (tabStrip._tabH + 6))
                            visible: missionCard._tab === 0
                            columns: 3
                            columnSpacing: 8
                            rowSpacing: 8

                            Repeater {
                                model: [
                                    { label: "ALTITUDE (REL)", key: "alt",  color: _cAccent },
                                    { label: "CLIMB RATE",     key: "vs",   color: _cGood   },
                                    { label: "FLIGHT TIME",    key: "time", color: _cWarn   },
                                    { label: "ALTITUDE (MSL)", key: "msl",  color: _cAccent },
                                    { label: "GROUND SPEED",   key: "gs",   color: _cGood   },
                                    { label: "HOME DISTANCE",  key: "home", color: _cWarn   }
                                ]

                                delegate: Rectangle {
                                    property var _t: _root._tel(modelData.key)
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    Layout.minimumHeight: Math.round(40 * _root._u)
                                    radius: 8
                                    color: _cCard
                                    border.color: _cBorder
                                    border.width: 1

                                    Rectangle {
                                        anchors.left: parent.left
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: 3
                                        height: parent.height * 0.55
                                        radius: 2
                                        color: modelData.color
                                    }

                                    Column {
                                        anchors.centerIn: parent
                                        spacing: 2

                                        Label {
                                            anchors.horizontalCenter: parent.horizontalCenter
                                            text: modelData.label
                                            color: _cMuted
                                            font.pixelSize: _fsXS
                                            font.letterSpacing: 1
                                        }
                                        Row {
                                            anchors.horizontalCenter: parent.horizontalCenter
                                            spacing: 4
                                            Label {
                                                text: _t.v
                                                color: _cText
                                                font.pixelSize: _fsXL + 6
                                                font.weight: Font.DemiBold
                                            }
                                            Label {
                                                y: parent.height - height - 3
                                                text: _t.u
                                                color: modelData.color
                                                font.pixelSize: _fsS
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        Label {
                            anchors.centerIn: parent
                            visible: (missionCard._tabKey === "params" || missionCard._tabKey === "status") && !_activeVehicle
                            text: missionCard._tabKey === "status" ? "Connect a vehicle to view status" : "Connect a vehicle to view parameters"
                            color: _cMuted
                            font.pixelSize: _fsM
                        }

                        Loader {
                            anchors.fill: parent
                            active: missionCard._tab > 0 && !((missionCard._tabKey === "params" || missionCard._tabKey === "status") && !_activeVehicle)
                            sourceComponent: missionCard._pageComponent()
                        }
                    }
                }

                // ---- Embedded pages (loaded only while their tab is open) ----
                Component { id: paramsComp;  ParameterEditor { } }
                Component {
                    id: statusComp

                    Flickable {
                        id: statusFlick
                        anchors.fill: parent
                        contentWidth: width
                        contentHeight: statusCol.implicitHeight
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                        property bool _healthSupported: _activeVehicle ? _activeVehicle.healthAndArmingCheckReport.supported : false
                        property bool _armed:           _activeVehicle ? _activeVehicle.armed : false
                        property real _sp:              ScreenTools.defaultFontPixelWidth / 2

                        ColumnLayout {
                            id: statusCol
                            width: statusFlick.width - 12
                            spacing: 10

                            // Arm / Disarm (same rules as QGC's own drawer; press and hold = Force Arm)
                            QGCButton {
                                Layout.alignment: Qt.AlignLeft
                                enabled: statusFlick._armed || !statusFlick._healthSupported || _activeVehicle.healthAndArmingCheckReport.canArm
                                text: statusFlick._armed ? qsTr("Disarm") : (forceArm ? qsTr("Force Arm") : qsTr("Arm"))

                                property bool forceArm: false

                                onPressAndHold: forceArm = true
                                onClicked: {
                                    if (statusFlick._armed)  mainWindow.disarmVehicleRequest()
                                    else if (forceArm)       mainWindow.forceArmVehicleRequest()
                                    else                     mainWindow.armVehicleRequest()
                                    forceArm = false
                                }
                            }

                            SettingsGroupLayout {
                                Layout.fillWidth: true
                                heading: qsTr("Vehicle Messages")
                                visible: !vehicleMessageList.noMessages

                                VehicleMessageList { id: vehicleMessageList }
                            }

                            // Older autopilots (like your screenshot): sensor list
                            SettingsGroupLayout {
                                Layout.fillWidth: true
                                heading: qsTr("Sensor Status")
                                visible: !statusFlick._healthSupported

                                GridLayout {
                                    rowSpacing: statusFlick._sp
                                    columnSpacing: statusFlick._sp * 4
                                    rows: _activeVehicle ? _activeVehicle.sysStatusSensorInfo.sensorNames.length : 0
                                    flow: GridLayout.TopToBottom

                                    Repeater {
                                        model: _activeVehicle ? _activeVehicle.sysStatusSensorInfo.sensorNames : null
                                        QGCLabel { text: modelData }
                                    }
                                    Repeater {
                                        model: _activeVehicle ? _activeVehicle.sysStatusSensorInfo.sensorStatus : null
                                        QGCLabel {
                                            text: modelData
                                            color: modelData === "Normal" ? _cGood : (modelData === "Disabled" ? _cMuted : _cBad)
                                        }
                                    }
                                }
                            }

                            // Newer PX4: list of problems that stop arming
                            SettingsGroupLayout {
                                Layout.fillWidth: true
                                heading: qsTr("Overall Status")
                                visible: statusFlick._healthSupported && _activeVehicle.healthAndArmingCheckReport.problemsForCurrentMode.count > 0

                                Repeater {
                                    model: _activeVehicle ? _activeVehicle.healthAndArmingCheckReport.problemsForCurrentMode : null
                                    delegate: QGCLabel {
                                        Layout.fillWidth: true
                                        text: object.message
                                        textFormat: TextEdit.RichText
                                        wrapMode: Text.WordWrap
                                        color: object.severity == 'error' ? _cBad : (object.severity == 'warning' ? _cWarn : _cText)
                                    }
                                }
                            }
                        }
                    }
                }
                Component { id: consoleComp; Loader { source: "qrc:/qml/QGroundControl/AnalyzeView/MAVLinkConsole/MAVLinkConsolePage.qml" } }
                Component { id: analyzeComp; Loader { source: "qrc:/qml/QGroundControl/AnalyzeView/AnalyzeView.qml" } }
                Component {
                    id: remoteIdComp
                    AppSettings { Component.onCompleted: Qt.callLater(function() { showSettingsPage("Remote ID") }) }
                }
                Component {
                    id: linksComp
                    AppSettings { Component.onCompleted: Qt.callLater(function() { showSettingsPage("Comm Links") }) }
                }
                // Your original Saved Missions list, unchanged, now a tab
                Component {
                    id: missionsComp
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 6

                            // Top Title + Live Distance to Home / Climb Rate + Plan Button
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: false
                                spacing: 10

                                Rectangle { width: 3; height: 16; radius: 2; color: _cText }

                                Label {
                                    text: "Saved Missions (" + missionFolderModel.count + ")"
                                    color: _cText
                                    font.weight: Font.DemiBold
                                    font.pixelSize: _fsM
                                }

                                // Live Distance to Home pill
                                Rectangle {
                                    color: _cCard
                                    radius: 4
                                    border.color: _cBorder
                                    Layout.preferredHeight: Math.round(22 * _root._u)
                                    Layout.preferredWidth: distLabel.implicitWidth + 14
                                    Label {
                                        id: distLabel
                                        anchors.centerIn: parent
                                        text: "Home: " + (_activeVehicle ? _root._fmt(_activeVehicle.distanceToHome, 1) : "--")
                                        color: _cSoft
                                        font.pixelSize: _fsS
                                    }
                                }

                                // Live Climb Rate pill
                                Rectangle {
                                    color: _cCard
                                    radius: 4
                                    border.color: _cBorder
                                    Layout.preferredHeight: Math.round(22 * _root._u)
                                    Layout.preferredWidth: vsLabel.implicitWidth + 14
                                    Label {
                                        id: vsLabel
                                        anchors.centerIn: parent
                                        text: "V/S: " + (_activeVehicle ? _root._fmt(_activeVehicle.climbRate, 1) : "--")
                                        color: _cSoft
                                        font.pixelSize: _fsS
                                    }
                                }

                                Item { Layout.fillWidth: true }

                                QGCButton {
                                    text: "Open Planner"
                                    onClicked: mainWindow.showPlanView()
                                }
                            }

                            // Table Header Row (column widths follow the card width)
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: false
                                Label { text: "Mission Name";  color: _cMuted; font.pixelSize: _fsS; Layout.preferredWidth: (missionCard.width - 28) * 0.36 }
                                Label { text: "Date Modified"; color: _cMuted; font.pixelSize: _fsS; Layout.preferredWidth: (missionCard.width - 28) * 0.28 }
                                Label { text: "Size";          color: _cMuted; font.pixelSize: _fsS; Layout.preferredWidth: (missionCard.width - 28) * 0.14 }
                                Label { text: "Action";        color: _cMuted; font.pixelSize: _fsS; Layout.fillWidth: true }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                Layout.fillHeight: false
                                color: _cBorder
                            }

                            // Scrollable Mission File List
                            ListView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                boundsBehavior: Flickable.StopAtBounds
                                spacing: 2
                                model: missionFolderModel

                                delegate: Rectangle {
                                    width: ListView.view.width
                                    height: Math.max(26, Math.round(28 * _root._u))
                                    radius: 4
                                    color: rowHover.containsMouse ? _cHover : "transparent"

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 4
                                        anchors.rightMargin: 4

                                        Label {
                                            text: fileBaseName
                                            color: _cText
                                            font.pixelSize: _fsM
                                            Layout.preferredWidth: (missionCard.width - 28) * 0.36
                                            elide: Text.ElideRight
                                        }
                                        Label {
                                            text: fileModified ? Qt.formatDateTime(fileModified, "yyyy-MM-dd hh:mm") : "--"
                                            color: _cSoft
                                            font.pixelSize: _fsM
                                            Layout.preferredWidth: (missionCard.width - 28) * 0.28
                                            elide: Text.ElideRight
                                        }
                                        Label {
                                            text: fileSize !== undefined ? (fileSize / 1024).toFixed(1) + " KB" : "--"
                                            color: _cMuted
                                            font.pixelSize: _fsS
                                            Layout.preferredWidth: (missionCard.width - 28) * 0.14
                                            elide: Text.ElideRight
                                        }
                                        Label {
                                            text: "Open in Plan →"
                                            color: _cAccent
                                            font.pixelSize: _fsS
                                            font.weight: Font.DemiBold
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                    }

                                    MouseArea {
                                        id: rowHover
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: mainWindow.showPlanView()
                                    }
                                }

                                // Empty State when no .plan files exist yet
                                ColumnLayout {
                                    anchors.centerIn: parent
                                    visible: missionFolderModel.count === 0
                                    spacing: 6

                                    Label {
                                        Layout.alignment: Qt.AlignHCenter
                                        text: "No saved .plan mission files found"
                                        color: _cMuted
                                        font.pixelSize: _fsM
                                    }
                                    QGCButton {
                                        Layout.alignment: Qt.AlignHCenter
                                        text: "Create New Mission"
                                        onClicked: mainWindow.showPlanView()
                                    }
                                }
                            }
                        }
                }
            }


            // ══════════════════════════════════════════
            // RIGHT: Flight Instruments (moved from the old right panel)
            // ══════════════════════════════════════════
            Rectangle {
                id: rightPanel
                Layout.fillWidth: true
                Layout.fillHeight: false
                Layout.alignment: Qt.AlignTop
                Layout.preferredHeight: _root._footerBaseH - 16
                Layout.preferredWidth: footer.width * 0.55
                color: _cPanel
                radius: 8
                border.color: _cBorder
                border.width: 1
                clip: true

                // ---- helpers (same as before) ----
                readonly property real _pitch: _activeVehicle ? _activeVehicle.pitch.rawValue : 0
                readonly property real _roll:  _activeVehicle ? _activeVehicle.roll.rawValue  : 0
                // one shared diameter so the attitude circle and the compass are identical
                readonly property real _dial:  Math.max(0, Math.min(attitudeHolder.width, attitudeHolder.height, compassHolder.width, compassHolder.height) - 6)
                readonly property real _sideW: Math.round(76 * _u)
                readonly property bool _compact: width < 700
                readonly property real _wAtt: _compact ? 0.25 : 0.29
                readonly property real _wCmp: _compact ? 0.25 : 0.29
                readonly property real _wBat: _compact ? 0.25 : 0.22
                readonly property real _wGps: _compact ? 0.25 : 0.20

                function _signed(v) {
                    if (!_activeVehicle || isNaN(v)) return "--"
                    return (v >= 0 ? "+" : "") + v.toFixed(1) + "°"
                }
                function _readout(key) {
                    switch (key) {
                    case "pitch":   return _signed(_pitch)
                    case "roll":    return _signed(_roll)
                    case "heading": return _activeVehicle ? _heading.toFixed(0) + "°" : "--"
                    }
                    return "--"
                }
                function _batPct() {
                    return (_battery && !isNaN(_battery.percentRemaining.value)) ? _battery.percentRemaining.value : NaN
                }
                function _batText(key) {
                    var b = _battery
                    if (!b) return "--"
                    switch (key) {
                    case "voltage": return isNaN(b.voltage.value) ? "--" : b.voltage.value.toFixed(1) + " V"
                    case "current": return isNaN(b.current.value) ? "--" : b.current.value.toFixed(1) + " A"
                    case "time":
                        var t = b.timeRemainingStr
                        if (t === undefined || t === null) return "--"
                        var s = (t.valueString !== undefined) ? t.valueString : String(t)
                        return s === "" ? "--" : s
                    }
                    return "--"
                }
                function _gpsText(key) {
                    if (!_activeVehicle) return "--"
                    switch (key) {
                    case "sats": return String(_activeVehicle.gps.count.rawValue)
                    case "fix":  return String(_activeVehicle.gps.lock.enumStringValue)
                    case "hdop": return isNaN(_activeVehicle.gps.hdop.rawValue) ? "--" : _activeVehicle.gps.hdop.rawValue.toFixed(1)
                    }
                    return "--"
                }
                function _gpsColor(key) {
                    if (!_activeVehicle) return _cMuted
                    if (key === "fix")  return _gpsText("fix").indexOf("3D") >= 0 ? _cGood : _cWarn
                    if (key === "hdop") {
                        var h = _activeVehicle.gps.hdop.rawValue
                        return h <= 2 ? _cGood : (h <= 5 ? _cWarn : _cBad)
                    }
                    return _cText
                }

                RowLayout {
                    id: dashRow
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 8

                    // ---------- Attitude + Pitch / Roll ----------
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredWidth: dashRow.width * rightPanel._wAtt
                        color: _cCard
                        radius: 10
                        border.color: _cBorder
                        border.width: 1

                        GridLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            columns: rightPanel._compact ? 1 : 2
                            columnSpacing: 6
                            rowSpacing: 4

                            Item {
                                id: attitudeHolder
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.preferredWidth: 3

                                CustomAttitudeWidget {
                                    anchors.centerIn: parent
                                    width:       rightPanel._dial
                                    height:      rightPanel._dial
                                    size:        rightPanel._dial
                                    vehicle:     _activeVehicle
                                    showHeading: false
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: false
                                Layout.fillHeight: !rightPanel._compact
                                Layout.preferredWidth: rightPanel._compact ? -1 : rightPanel._sideW
                                Layout.alignment: Qt.AlignHCenter
                                spacing: 6

                                Item { Layout.fillHeight: true }

                                Repeater {
                                    model: [
                                        { label: "PITCH", key: "pitch" },
                                        { label: "ROLL",  key: "roll"  }
                                    ]

                                    delegate: Column {
                                        Layout.fillWidth: true
                                        spacing: 0

                                        Label {
                                            text: modelData.label
                                            color: _cMuted
                                            font.pixelSize: _fsXS
                                            font.letterSpacing: 1
                                        }
                                        Label {
                                            text: rightPanel._readout(modelData.key)
                                            color: _cGood
                                            font.pixelSize: _fsL
                                            font.weight: Font.DemiBold
                                        }
                                    }
                                }

                                Item { Layout.fillHeight: true }
                            }
                        }
                    }

                    // ---------- Compass + Heading ----------
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredWidth: dashRow.width * rightPanel._wCmp
                        color: _cCard
                        radius: 10
                        border.color: _cBorder
                        border.width: 1

                        GridLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            columns: rightPanel._compact ? 1 : 2
                            columnSpacing: 6
                            rowSpacing: 4

                            Item {
                                id: compassHolder
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.preferredWidth: 3

                                Rectangle {
                                    id: compassBezel
                                    anchors.centerIn: parent
                                    width:  rightPanel._dial
                                    height: width
                                    radius: width / 2
                                    color: "transparent"
                                    border.color: _cMuted
                                    border.width: 1

                                    // rotating dial
                                    Item {
                                        id: compassDial
                                        anchors.fill: parent
                                        rotation: -_heading

                                        // ticks every 10 degrees (skipped at N/E/S/W)
                                        Repeater {
                                            model: 36
                                            delegate: Item {
                                                anchors.fill: parent
                                                rotation: index * 10
                                                Rectangle {
                                                    visible: index % 9 !== 0
                                                    width: 1
                                                    height: index % 3 === 0 ? 8 : 4
                                                    color: _cMuted
                                                    anchors.horizontalCenter: parent.horizontalCenter
                                                    anchors.top: parent.top
                                                    anchors.topMargin: 2
                                                }
                                            }
                                        }

                                        Label { text: "N"; color: _cAccent; font.pixelSize: _fsM; font.weight: Font.DemiBold
                                                anchors.top: parent.top; anchors.topMargin: 12; anchors.horizontalCenter: parent.horizontalCenter }
                                        Label { text: "S"; color: _cSoft; font.pixelSize: _fsM; font.weight: Font.DemiBold
                                                anchors.bottom: parent.bottom; anchors.bottomMargin: 12; anchors.horizontalCenter: parent.horizontalCenter }
                                        Label { text: "E"; color: _cSoft; font.pixelSize: _fsM; font.weight: Font.DemiBold
                                                anchors.right: parent.right; anchors.rightMargin: 12; anchors.verticalCenter: parent.verticalCenter }
                                        Label { text: "W"; color: _cSoft; font.pixelSize: _fsM; font.weight: Font.DemiBold
                                                anchors.left: parent.left; anchors.leftMargin: 12; anchors.verticalCenter: parent.verticalCenter }
                                    }

                                    // fixed red marker at the top
                                    Rectangle {
                                        width: 9; height: 9
                                        rotation: 45
                                        color: _cBad
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        anchors.top: parent.top
                                        anchors.topMargin: -4
                                    }

                                    // heading number in the center
                                    Label {
                                        anchors.centerIn: parent
                                        text: _activeVehicle ? _heading.toFixed(0) + "°" : "--"
                                        color: _cText
                                        font.pixelSize: _fsXL
                                        font.weight: Font.DemiBold
                                    }
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: false
                                Layout.fillHeight: !rightPanel._compact
                                Layout.preferredWidth: rightPanel._compact ? -1 : rightPanel._sideW
                                Layout.alignment: Qt.AlignHCenter
                                spacing: 2

                                Item { Layout.fillHeight: true }
                                Label { text: "HEADING"; color: _cMuted; font.pixelSize: _fsXS; font.letterSpacing: 1 }
                                Label {
                                    text: rightPanel._readout("heading")
                                    color: _cText
                                    font.pixelSize: _fsL
                                    font.weight: Font.DemiBold
                                }
                                Item { Layout.fillHeight: true }
                            }
                        }
                    }

                    // ---------- Battery ----------
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredWidth: dashRow.width * rightPanel._wBat
                        color: _cCard
                        radius: 10
                        border.color: _cBorder
                        border.width: 1
                        clip: true

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: rightPanel._compact ? 6 : 10
                            spacing: 4

                            Label {
                                text: "Battery"
                                color: _cText
                                font.pixelSize: _fsM
                                font.weight: Font.DemiBold
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Label {
                                    text: isNaN(rightPanel._batPct()) ? "--%" : rightPanel._batPct().toFixed(0) + "%"
                                    color: isNaN(rightPanel._batPct()) ? _cMuted
                                           : (rightPanel._batPct() > 50 ? _cGood : (rightPanel._batPct() > 20 ? _cWarn : _cBad))
                                    font.pixelSize: _fsXL
                                    font.weight: Font.DemiBold
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 6
                                    radius: 3
                                    color: _cLine

                                    Rectangle {
                                        height: parent.height
                                        radius: 3
                                        width: isNaN(rightPanel._batPct()) ? 0
                                               : parent.width * Math.max(0, Math.min(100, rightPanel._batPct())) / 100
                                        color: rightPanel._batPct() > 50 ? _cGood : (rightPanel._batPct() > 20 ? _cWarn : _cBad)
                                    }
                                }
                            }

                            Repeater {
                                model: [
                                    { label: "Voltage",        key: "voltage" },
                                    { label: "Current",        key: "current" },
                                    { label: "Est. time left", key: "time" }
                                ]

                                delegate: RowLayout {
                                    Layout.fillWidth: true
                                    Label { text: modelData.label; color: _cMuted; font.pixelSize: _fsS; elide: Text.ElideRight }
                                    Item { Layout.fillWidth: true }
                                    Label {
                                        text: rightPanel._batText(modelData.key)
                                        color: modelData.key === "time" ? _cWarn : _cText
                                        font.pixelSize: _fsS
                                        font.weight: Font.DemiBold
                                    }
                                }
                            }

                            Item { Layout.fillHeight: true }
                        }
                    }

                    // ---------- GPS ----------
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredWidth: dashRow.width * rightPanel._wGps
                        color: _cCard
                        radius: 10
                        border.color: _cBorder
                        border.width: 1
                        clip: true

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 6

                            Label {
                                text: "GPS"
                                color: _cText
                                font.pixelSize: _fsM
                                font.weight: Font.DemiBold
                            }

                            Repeater {
                                model: [
                                    { label: "Satellites", key: "sats" },
                                    { label: "Fix Type",   key: "fix"  },
                                    { label: "HDOP",       key: "hdop" }
                                ]

                                delegate: RowLayout {
                                    Layout.fillWidth: true
                                    Label { text: modelData.label; color: _cMuted; font.pixelSize: _fsS }
                                    Item { Layout.fillWidth: true }
                                    Label {
                                        text: rightPanel._gpsText(modelData.key)
                                        color: rightPanel._gpsColor(modelData.key)
                                        font.pixelSize: _fsS
                                        font.weight: Font.DemiBold
                                    }
                                }
                            }

                            Item { Layout.fillHeight: true }
                        }
                    }
                }
            }


        }

        // Flight mode list (opens above the Flight Mode button)
        Popup {
            id: modePopup
            padding: 4
            modal: false
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

            background: Rectangle {
                color: _cCard
                border.color: _cAccent
                radius: 5
            }

            contentItem: ListView {
                implicitHeight: Math.min(contentHeight, 240)
                clip: true
                model: _activeVehicle ? _activeVehicle.flightModes : []

                delegate: Rectangle {
                    width: ListView.view.width
                    height: 30
                    radius: 3
                    color: modeMouse.containsMouse ? _cHover : "transparent"

                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 8
                        text: modelData
                        color: (_activeVehicle && modelData === _activeVehicle.flightMode) ? _cAccent : _cText
                        font.pixelSize: _fsM
                    }

                    MouseArea {
                        id: modeMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            _activeVehicle.flightMode = modelData
                            footer._actionFeedback = "Mode changed to " + modelData
                            modePopup.close()
                        }
                    }
                }
            }
        }

        // Fixed FolderListModel with proper file:/// URL conversion for Windows & Linux
        FolderListModel {
            id: missionFolderModel
            folder: {
                var p = QGroundControl.settingsManager.appSettings.missionSavePath.replace(/\\/g, "/")
                return p.startsWith("/") ? ("file://" + p) : ("file:///" + p)
            }
            nameFilters: ["*.plan"]
            showDirs: false
            sortField: FolderListModel.Time
            sortReversed: true
        }
    }
}