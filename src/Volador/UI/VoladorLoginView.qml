/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Aerospace Grade Login Interface
 * Modern Industrial Aerospace Command Center Standard (Light Industrial Theme)
 *
 * Inspired by Airbus mission software, DJI Enterprise command systems, and
 * industrial aviation control terminals.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools

Rectangle {
    id: loginRoot
    anchors.fill: parent
    clip: true

    // =========================================================================
    // COLOR PALETTE & DESIGN TOKENS (Tech Stark White + Industrial Grey)
    // =========================================================================
    readonly property color colorWindow: "#F3F4F6"
    readonly property color colorCard: "#FFFFFF"
    readonly property color colorHeaderNav: "#E5E7EB"
    readonly property color colorPanel: "#F8FAFC"
    readonly property color colorBorder: "#CBD5E1"
    readonly property color colorInput: "#FFFFFF"
    readonly property color colorHover: "#E2E8F0"

    // Primary Action (Volador Orange)
    readonly property color colorOrange: "#FF6A00"
    readonly property color colorOrangeHover: "#FF8126"
    readonly property color colorOrangePressed: "#D95500"
    readonly property color colorButtonDisabled: "#94A3B8"

    // Typography Colors
    readonly property color colorPrimaryText: "#111827"
    readonly property color colorSecondaryText: "#475569"
    readonly property color colorPlaceholderText: "#94A3B8"

    // Status & Error Banner Colors
    readonly property color colorGreen: "#16A34A"
    readonly property color colorRed: "#DC2626"
    readonly property color colorErrorBannerBg: "#FEF2F2"
    readonly property color colorErrorBannerBorder: "#EF4444"
    readonly property color colorErrorText: "#B91C1C"

    // Geometry & Typography Tokens (Scaled via ScreenTools for High DPI)
    readonly property int cornerRadiusCard: 8
    readonly property int cornerRadiusControl: 6
    readonly property int animDuration: 80
    readonly property string fontFamily: Qt.platform.os === "windows" ? "Segoe UI" : "Noto Sans"

    // Role State Propagation
    property int selectedRoleIndex: 1

    // Real-Time UTC Clock & Date State
    property string utcTimeString: ""
    property string utcDateString: ""

    Timer {
        id: utcClockTimer
        interval: 1000
        running: true
        repeat: true
        triggeredOnStart: true
        onTriggered: {
            var now = new Date()
            loginRoot.utcTimeString = Qt.formatTime(now, "hh:mm:ss") + " UTC"
            loginRoot.utcDateString = Qt.formatDate(now, "dddd, MMMM d, yyyy").toUpperCase()
        }
    }

    color: colorWindow

    // Perform Authentication
    function doLogin() {
        if (typeof voladorAuth !== "undefined" && voladorAuth && !voladorAuth.isBusy) {
            voladorAuth.login(userField.text, passField.text)
        }
    }

    // Smooth Entrance Transition (80ms max duration)
    opacity: 0
    Component.onCompleted: opacity = 1
    Behavior on opacity {
        NumberAnimation { duration: loginRoot.animDuration; easing.type: Easing.InOutQuad }
    }

    // =========================================================================
    // BACKGROUND: Subtle Aerospace Mission HUD (8% Opacity Canvas)
    // =========================================================================
    Canvas {
        id: hudCanvas
        anchors.fill: parent
        opacity: 0.08
        renderTarget: Canvas.Image

        onPaint: {
            var ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);

            ctx.strokeStyle = "#64748B";
            ctx.fillStyle = "#64748B";
            ctx.lineWidth = 1;

            // 1. Satellite World Map Continents Outline (Vector Polygon Approximation)
            var mapPolygons = [
                // North America
                [{x:0.10,y:0.18},{x:0.25,y:0.16},{x:0.32,y:0.30},{x:0.28,y:0.45},{x:0.18,y:0.48},{x:0.12,y:0.32}],
                // South America
                [{x:0.25,y:0.52},{x:0.34,y:0.54},{x:0.30,y:0.82},{x:0.24,y:0.78},{x:0.22,y:0.60}],
                // Europe
                [{x:0.46,y:0.20},{x:0.58,y:0.18},{x:0.60,y:0.34},{x:0.48,y:0.36},{x:0.44,y:0.26}],
                // Africa
                [{x:0.45,y:0.38},{x:0.60,y:0.40},{x:0.62,y:0.72},{x:0.50,y:0.80},{x:0.43,y:0.54}],
                // Asia
                [{x:0.60,y:0.16},{x:0.88,y:0.18},{x:0.92,y:0.46},{x:0.72,y:0.50},{x:0.64,y:0.36}],
                // Australia
                [{x:0.76,y:0.64},{x:0.88,y:0.65},{x:0.86,y:0.84},{x:0.74,y:0.82}]
            ];

            for (var m = 0; m < mapPolygons.length; m++) {
                var poly = mapPolygons[m];
                ctx.beginPath();
                ctx.moveTo(poly[0].x * width, poly[0].y * height);
                for (var p = 1; p < poly.length; p++) {
                    ctx.lineTo(poly[p].x * width, poly[p].y * height);
                }
                ctx.closePath();
                ctx.stroke();
            }

            // 2. Latitude & Longitude Grid Overlay
            var gridSpacing = Math.max(20, Math.round(ScreenTools.defaultFontPixelHeight * 4.5));
            ctx.beginPath();
            ctx.setLineDash([2, 6]);
            for (var x = 0; x < width; x += gridSpacing) {
                ctx.moveTo(x, 0); ctx.lineTo(x, height);
            }
            for (var y = 0; y < height; y += gridSpacing) {
                ctx.moveTo(0, y); ctx.lineTo(width, y);
            }
            ctx.stroke();
            ctx.setLineDash([]);

            // 3. Concentric Radar Rings & Airspace Sectors
            var centerX = width * 0.16;
            var centerY = height * 0.54;
            for (var r = 90; r <= 390; r += 75) {
                ctx.beginPath();
                ctx.arc(centerX, centerY, r, 0, Math.PI * 2);
                ctx.stroke();
            }
            ctx.beginPath();
            ctx.moveTo(centerX - 440, centerY); ctx.lineTo(centerX + 440, centerY);
            ctx.moveTo(centerX, centerY - 440); ctx.lineTo(centerX, centerY + 440);
            ctx.stroke();

            // 4. Flight Paths & Waypoint Circles
            var waypoints = [
                {x: width * 0.06, y: height * 0.84},
                {x: width * 0.26, y: height * 0.30},
                {x: width * 0.56, y: height * 0.40},
                {x: width * 0.82, y: height * 0.16},
                {x: width * 0.94, y: height * 0.60}
            ];
            ctx.beginPath();
            ctx.lineWidth = 1.5;
            ctx.moveTo(waypoints[0].x, waypoints[0].y);
            for (var i = 1; i < waypoints.length; i++) {
                ctx.lineTo(waypoints[i].x, waypoints[i].y);
            }
            ctx.stroke();

            for (var w = 0; w < waypoints.length; w++) {
                var wp = waypoints[w];
                ctx.beginPath();
                ctx.arc(wp.x, wp.y, 4, 0, Math.PI * 2);
                ctx.stroke();
                ctx.beginPath();
                ctx.arc(wp.x, wp.y, 8, 0, Math.PI * 2);
                ctx.stroke();
            }

            // 5. Telemetry & Navigation Labels
            ctx.font = "10px monospace";
            ctx.fillText("37°46'29.7\"N 122°25'09.8\"W", width * 0.06 + 12, height * 0.84 + 4);
            ctx.fillText("ALT: 450m MSL | SPD: 18.5 m/s", width * 0.26 + 12, height * 0.30 + 4);
            ctx.fillText("LINK: SAT-04 ONLINE", width * 0.56 + 12, height * 0.40 + 4);

            // 6. Tactical Drone Wireframe
            var dx = width * 0.86;
            var dy = height * 0.30;
            ctx.beginPath();
            ctx.moveTo(dx - 32, dy - 32); ctx.lineTo(dx + 32, dy + 32);
            ctx.moveTo(dx + 32, dy - 32); ctx.lineTo(dx - 32, dy + 32);
            ctx.stroke();
            var rotors = [{x: dx-32,y: dy-32},{x: dx+32,y: dy-32},{x: dx-32,y: dy+32},{x: dx+36,y: dy+32}];
            for (var k = 0; k < rotors.length; k++) {
                ctx.beginPath();
                ctx.arc(rotors[k].x, rotors[k].y, 10, 0, Math.PI * 2);
                ctx.stroke();
            }
        }

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }

    // =========================================================================
    // VOLADOR LOGO WATERMARK BEHIND LOGIN CARD (3% Opacity Slate)
    // =========================================================================
    Text {
        anchors.centerIn: parent
        text: "VOLADOR"
        font.family: loginRoot.fontFamily
        font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 12.0)
        font.weight: Font.Black
        color: "#0F172A"
        opacity: 0.03
        z: 0
    }

    // =========================================================================
    // MAIN PAGE LAYOUT
    // =========================================================================
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        z: 1

        // ---------------------------------------------------------------------
        // TOP NAVIGATION BAR (Height: 64px, Background: #E5E7EB)
        // ---------------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            height: Math.round(ScreenTools.defaultFontPixelHeight * 4.0) // 64px scaled
            color: colorHeaderNav
            border.color: colorBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Math.round(ScreenTools.defaultFontPixelWidth * 2.5)
                anchors.rightMargin: Math.round(ScreenTools.defaultFontPixelWidth * 2.5)
                spacing: 20

                // Volador Logo & Brand Subtitle
                RowLayout {
                    spacing: 12
                    Image {
                        source: "qrc:/Volador/Assets/Logos/volador_primary.png"
                        implicitWidth: Math.round(ScreenTools.defaultFontPixelHeight * 7.5)
                        implicitHeight: Math.round(ScreenTools.defaultFontPixelHeight * 2.2)
                        fillMode: Image.PreserveAspectFit
                        antialiasing: true
                        mipmap: true
                    }

                    Text {
                        text: "VGCS"
                        font.family: loginRoot.fontFamily
                        font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.75)
                        font.weight: Font.Bold
                        color: colorPrimaryText
                    }
                }

                Rectangle {
                    width: 1
                    height: Math.round(ScreenTools.defaultFontPixelHeight * 1.2)
                    color: colorBorder
                }

                // System Status
                RowLayout {
                    spacing: 8
                    Rectangle {
                        width: Math.round(ScreenTools.defaultFontPixelHeight * 0.5)
                        height: width
                        radius: width / 2
                        color: colorGreen
                    }
                    Text {
                        text: "SYSTEM STATUS: OPERATIONAL"
                        font.family: loginRoot.fontFamily
                        font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.8) // 13px
                        font.weight: Font.Bold
                        color: colorGreen
                    }
                }

                Item { Layout.fillWidth: true } // Center Spacer

                // Mission Time (UTC Real-Time Updating)
                RowLayout {
                    spacing: 6
                    Canvas {
                        width: 14; height: 14
                        onPaint: {
                            var ctx = getContext("2d");
                            ctx.clearRect(0,0,14,14);
                            ctx.strokeStyle = colorSecondaryText;
                            ctx.lineWidth = 1.2;
                            ctx.beginPath();
                            ctx.arc(7,7,5.5,0,Math.PI*2);
                            ctx.stroke();
                            ctx.beginPath();
                            ctx.moveTo(7,3.5); ctx.lineTo(7,7); ctx.lineTo(9.5,7);
                            ctx.stroke();
                        }
                    }
                    Text {
                        font.family: loginRoot.fontFamily
                        font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.8) // 13px
                        color: colorPrimaryText
                        text: loginRoot.utcTimeString
                    }
                }

                Rectangle {
                    width: 1
                    height: Math.round(ScreenTools.defaultFontPixelHeight * 1.2)
                    color: colorBorder
                }

                // UTC Date (Real-Time Updating)
                Text {
                    font.family: loginRoot.fontFamily
                    font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.8) // 13px
                    color: colorSecondaryText
                    text: loginRoot.utcDateString
                }

                Rectangle {
                    width: 1
                    height: Math.round(ScreenTools.defaultFontPixelHeight * 1.2)
                    color: colorBorder
                }

                // Station Number
                Text {
                    text: "STATION #01"
                    font.family: loginRoot.fontFamily
                    font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.8) // 13px
                    font.weight: Font.Bold
                    color: colorPrimaryText
                }

                Rectangle {
                    width: 1
                    height: Math.round(ScreenTools.defaultFontPixelHeight * 1.2)
                    color: colorBorder
                }

                // Settings Capsule Icon (Canvas Only)
                Rectangle {
                    width: Math.round(ScreenTools.defaultFontPixelHeight * 2.2)
                    height: width
                    radius: loginRoot.cornerRadiusControl
                    color: colorInput
                    border.color: colorBorder
                    border.width: 1

                    Canvas {
                        anchors.centerIn: parent
                        width: Math.round(ScreenTools.defaultFontPixelHeight * 1.1)
                        height: width
                        onPaint: {
                            var ctx = getContext("2d");
                            ctx.clearRect(0, 0, width, height);
                            ctx.strokeStyle = colorSecondaryText;
                            ctx.lineWidth = 1.5;
                            ctx.beginPath();
                            ctx.arc(width * 0.5, height * 0.5, width * 0.28, 0, Math.PI * 2);
                            ctx.stroke();
                            ctx.beginPath();
                            ctx.moveTo(width * 0.5, height * 0.06); ctx.lineTo(width * 0.5, height * 0.18);
                            ctx.moveTo(width * 0.5, height * 0.82); ctx.lineTo(width * 0.5, height * 0.94);
                            ctx.moveTo(width * 0.06, height * 0.5); ctx.lineTo(width * 0.18, height * 0.5);
                            ctx.moveTo(width * 0.82, height * 0.5); ctx.lineTo(width * 0.94, height * 0.5);
                            ctx.stroke();
                        }
                    }
                }
            }
        }

        // ---------------------------------------------------------------------
        // CENTER AUTHENTICATION CARD (Responsive Width: Math.min(parent.width - 80, Math.max(760, parent.width * 0.6)))
        // ---------------------------------------------------------------------
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Rectangle {
                id: mainCard
                width: Math.min(parent.width - 80, Math.max(760, parent.width * 0.6))
                implicitHeight: cardColumn.implicitHeight + Math.round(ScreenTools.defaultFontPixelHeight * 3.5)
                anchors.centerIn: parent
                radius: loginRoot.cornerRadiusCard
                color: colorCard
                border.color: colorBorder
                border.width: 1

                ColumnLayout {
                    id: cardColumn
                    anchors.fill: parent
                    anchors.margins: Math.round(ScreenTools.defaultFontPixelHeight * 1.8) // Reduced card padding
                    spacing: Math.round(ScreenTools.defaultFontPixelHeight * 1.2) // Reduced vertical section spacing

                    // 1. HEADER (Title: 34px Bold, Subtitle: 15px & Online Badge)
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Text {
                                text: "Welcome to VGCS"
                                font.family: loginRoot.fontFamily
                                font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 2.1) // 34px Title
                                font.weight: Font.Bold
                                color: colorPrimaryText
                            }

                            Text {
                                text: "Commercial Autonomous Drone Operations Platform"
                                font.family: loginRoot.fontFamily
                                font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.95) // 15px Subtitle
                                font.weight: Font.Medium
                                color: colorSecondaryText
                            }
                        }

                        // Online Status Badge
                        Rectangle {
                            height: Math.round(ScreenTools.defaultFontPixelHeight * 1.8)
                            implicitWidth: statusRow.implicitWidth + 24
                            radius: loginRoot.cornerRadiusControl
                            color: colorInput
                            border.color: colorGreen
                            border.width: 1

                            RowLayout {
                                id: statusRow
                                anchors.centerIn: parent
                                spacing: 8

                                Rectangle {
                                    width: Math.round(ScreenTools.defaultFontPixelHeight * 0.5)
                                    height: width
                                    radius: width / 2
                                    color: colorGreen
                                }

                                Text {
                                    text: "ONLINE"
                                    font.family: loginRoot.fontFamily
                                    font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.8) // 13px
                                    font.weight: Font.Bold
                                    color: colorGreen
                                }
                            }
                        }
                    }

                    // 2. ERROR BANNER (Height: ~44px, Background: #FEF2F2, Border: #EF4444, Orange Triangle Icon)
                    Rectangle {
                        Layout.fillWidth: true
                        height: Math.round(ScreenTools.defaultFontPixelHeight * 2.8) // ~44px
                        color: colorErrorBannerBg
                        border.color: colorErrorBannerBorder
                        border.width: 1
                        radius: loginRoot.cornerRadiusControl
                        visible: (typeof voladorAuth !== "undefined" && voladorAuth && voladorAuth.loginError && voladorAuth.loginError.length > 0)

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            anchors.rightMargin: 16
                            spacing: 12

                            // Orange Warning Triangle Vector Icon
                            Canvas {
                                width: 18; height: 18
                                onPaint: {
                                    var ctx = getContext("2d");
                                    ctx.clearRect(0, 0, 18, 18);
                                    ctx.strokeStyle = colorOrange;
                                    ctx.fillStyle = colorOrange;
                                    ctx.lineWidth = 1.8;
                                    ctx.beginPath();
                                    ctx.moveTo(9, 2);
                                    ctx.lineTo(17, 16);
                                    ctx.lineTo(1, 16);
                                    ctx.closePath();
                                    ctx.stroke();
                                    // Exclamation mark
                                    ctx.fillRect(8.2, 6.5, 1.6, 4.5);
                                    ctx.fillRect(8.2, 12.5, 1.6, 1.6);
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: (typeof voladorAuth !== "undefined" && voladorAuth && voladorAuth.loginError && voladorAuth.loginError.length > 0) ? voladorAuth.loginError : "Invalid username or password"
                                font.family: loginRoot.fontFamily
                                font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.8) // 13px
                                color: colorErrorText
                                elide: Text.ElideRight
                            }
                        }
                    }

                    // 3. OPERATOR ROLES (4 Equal Width Cards, Reduced Height ~64px)
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6 // Reduced spacing

                        Text {
                            text: "OPERATOR ROLE"
                            font.family: loginRoot.fontFamily
                            font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.875) // 14px SemiBold Section Title
                            font.weight: Font.DemiBold
                            color: colorSecondaryText
                        }

                        RowLayout {
                            id: roleBar
                            Layout.fillWidth: true
                            spacing: 12
                            property int selectedIndex: loginRoot.selectedRoleIndex

                            onSelectedIndexChanged: {
                                loginRoot.selectedRoleIndex = selectedIndex
                                if (typeof voladorAuth !== "undefined" && voladorAuth) {
                                    if ("selectedRole" in voladorAuth) voladorAuth.selectedRole = selectedIndex;
                                    if ("userRole" in voladorAuth) voladorAuth.userRole = selectedIndex;
                                }
                            }

                            // Administrator Card (Shield Vector Icon)
                            Rectangle {
                                id: roleAdmin
                                Layout.fillWidth: true
                                height: Math.round(ScreenTools.defaultFontPixelHeight * 4.0) // ~64px
                                radius: loginRoot.cornerRadiusControl
                                color: roleBar.selectedIndex === 0 ? colorPanel : (adminMouse.containsMouse ? colorHover : colorCard)
                                border.color: roleBar.selectedIndex === 0 ? colorPrimaryText : colorBorder
                                border.width: roleBar.selectedIndex === 0 ? 2 : 1

                                Behavior on color { ColorAnimation { duration: loginRoot.animDuration } }
                                Behavior on border.color { ColorAnimation { duration: loginRoot.animDuration } }

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    spacing: 2

                                    RowLayout {
                                        spacing: 8
                                        Canvas {
                                            id: adminCanvas
                                            width: 18; height: 18 // 18px Icon
                                            property bool isSelected: roleBar.selectedIndex === 0
                                            onIsSelectedChanged: requestPaint()

                                            onPaint: {
                                                var ctx = getContext("2d");
                                                ctx.clearRect(0,0,18,18);
                                                ctx.strokeStyle = isSelected ? colorPrimaryText : colorSecondaryText;
                                                ctx.lineWidth = 1.5;
                                                ctx.beginPath();
                                                ctx.moveTo(9, 2);
                                                ctx.lineTo(16, 4.5);
                                                ctx.lineTo(16, 10);
                                                ctx.bezierCurveTo(16, 14.5, 9, 16.5, 9, 16.5);
                                                ctx.bezierCurveTo(9, 16.5, 2, 14.5, 2, 10);
                                                ctx.lineTo(2, 4.5);
                                                ctx.closePath();
                                                ctx.stroke();
                                            }
                                        }
                                        Text {
                                            text: "Administrator"
                                            font.family: loginRoot.fontFamily
                                            font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.95) // 15px Title
                                            font.weight: Font.Bold
                                            color: colorPrimaryText
                                        }
                                    }
                                    Text {
                                        text: "System Administrator"
                                        font.family: loginRoot.fontFamily
                                        font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.75) // 12px Description
                                        color: colorSecondaryText
                                        elide: Text.ElideRight
                                    }
                                }
                                MouseArea {
                                    id: adminMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: roleBar.selectedIndex = 0
                                }
                            }

                            // Pilot Card (Drone Vector Icon)
                            Rectangle {
                                id: rolePilot
                                Layout.fillWidth: true
                                height: Math.round(ScreenTools.defaultFontPixelHeight * 4.0) // ~64px
                                radius: loginRoot.cornerRadiusControl
                                color: roleBar.selectedIndex === 1 ? colorPanel : (pilotMouse.containsMouse ? colorHover : colorCard)
                                border.color: roleBar.selectedIndex === 1 ? colorPrimaryText : colorBorder
                                border.width: roleBar.selectedIndex === 1 ? 2 : 1

                                Behavior on color { ColorAnimation { duration: loginRoot.animDuration } }
                                Behavior on border.color { ColorAnimation { duration: loginRoot.animDuration } }

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    spacing: 2

                                    RowLayout {
                                        spacing: 8
                                        Canvas {
                                            id: pilotCanvas
                                            width: 18; height: 18 // 18px Icon
                                            property bool isSelected: roleBar.selectedIndex === 1
                                            onIsSelectedChanged: requestPaint()

                                            onPaint: {
                                                var ctx = getContext("2d");
                                                ctx.clearRect(0,0,18,18);
                                                ctx.strokeStyle = isSelected ? colorPrimaryText : colorSecondaryText;
                                                ctx.lineWidth = 1.2;
                                                ctx.beginPath();
                                                ctx.moveTo(3,3); ctx.lineTo(15,15);
                                                ctx.moveTo(15,3); ctx.lineTo(3,15);
                                                ctx.stroke();
                                                ctx.beginPath();
                                                ctx.arc(3,3,2.5,0,Math.PI*2);
                                                ctx.arc(15,3,2.5,0,Math.PI*2);
                                                ctx.arc(3,15,2.5,0,Math.PI*2);
                                                ctx.arc(15,15,2.5,0,Math.PI*2);
                                                ctx.stroke();
                                                ctx.fillStyle = isSelected ? colorPrimaryText : colorSecondaryText;
                                                ctx.fillRect(7,7,4,4);
                                            }
                                        }
                                        Text {
                                            text: "Pilot"
                                            font.family: loginRoot.fontFamily
                                            font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.95) // 15px Title
                                            font.weight: Font.Bold
                                            color: colorPrimaryText
                                        }
                                    }
                                    Text {
                                        text: "Flight Operations"
                                        font.family: loginRoot.fontFamily
                                        font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.75) // 12px Description
                                        color: colorSecondaryText
                                        elide: Text.ElideRight
                                    }
                                }
                                MouseArea {
                                    id: pilotMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: roleBar.selectedIndex = 1
                                }
                            }

                            // Mission Planner Card (Route Vector Icon)
                            Rectangle {
                                id: rolePlanner
                                Layout.fillWidth: true
                                height: Math.round(ScreenTools.defaultFontPixelHeight * 4.0) // ~64px
                                radius: loginRoot.cornerRadiusControl
                                color: roleBar.selectedIndex === 2 ? colorPanel : (plannerMouse.containsMouse ? colorHover : colorCard)
                                border.color: roleBar.selectedIndex === 2 ? colorPrimaryText : colorBorder
                                border.width: roleBar.selectedIndex === 2 ? 2 : 1

                                Behavior on color { ColorAnimation { duration: loginRoot.animDuration } }
                                Behavior on border.color { ColorAnimation { duration: loginRoot.animDuration } }

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    spacing: 2

                                    RowLayout {
                                        spacing: 8
                                        Canvas {
                                            id: plannerCanvas
                                            width: 18; height: 18 // 18px Icon
                                            property bool isSelected: roleBar.selectedIndex === 2
                                            onIsSelectedChanged: requestPaint()

                                            onPaint: {
                                                var ctx = getContext("2d");
                                                ctx.clearRect(0,0,18,18);
                                                ctx.strokeStyle = isSelected ? colorPrimaryText : colorSecondaryText;
                                                ctx.lineWidth = 1.5;
                                                ctx.beginPath();
                                                ctx.moveTo(3,15); ctx.lineTo(9,4); ctx.lineTo(15,13);
                                                ctx.stroke();
                                                ctx.beginPath();
                                                ctx.arc(3,15,2,0,Math.PI*2);
                                                ctx.arc(9,4,2,0,Math.PI*2);
                                                ctx.arc(15,13,2,0,Math.PI*2);
                                                ctx.stroke();
                                            }
                                        }
                                        Text {
                                            text: "Mission Planner"
                                            font.family: loginRoot.fontFamily
                                            font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.95) // 15px Title
                                            font.weight: Font.Bold
                                            color: colorPrimaryText
                                        }
                                    }
                                    Text {
                                        text: "Mission Planning"
                                        font.family: loginRoot.fontFamily
                                        font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.75) // 12px Description
                                        color: colorSecondaryText
                                        elide: Text.ElideRight
                                    }
                                }
                                MouseArea {
                                    id: plannerMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: roleBar.selectedIndex = 2
                                }
                            }

                            // Observer Card (Eye Vector Icon)
                            Rectangle {
                                id: roleObserver
                                Layout.fillWidth: true
                                height: Math.round(ScreenTools.defaultFontPixelHeight * 4.0) // ~64px
                                radius: loginRoot.cornerRadiusControl
                                color: roleBar.selectedIndex === 3 ? colorPanel : (observerMouse.containsMouse ? colorHover : colorCard)
                                border.color: roleBar.selectedIndex === 3 ? colorPrimaryText : colorBorder
                                border.width: roleBar.selectedIndex === 3 ? 2 : 1

                                Behavior on color { ColorAnimation { duration: loginRoot.animDuration } }
                                Behavior on border.color { ColorAnimation { duration: loginRoot.animDuration } }

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    spacing: 2

                                    RowLayout {
                                        spacing: 8
                                        Canvas {
                                            id: observerCanvas
                                            width: 18; height: 18 // 18px Icon
                                            property bool isSelected: roleBar.selectedIndex === 3
                                            onIsSelectedChanged: requestPaint()

                                            onPaint: {
                                                var ctx = getContext("2d");
                                                ctx.clearRect(0,0,18,18);
                                                ctx.strokeStyle = isSelected ? colorPrimaryText : colorSecondaryText;
                                                ctx.lineWidth = 1.5;
                                                ctx.beginPath();
                                                ctx.moveTo(2,9);
                                                ctx.bezierCurveTo(5,4.5, 13,4.5, 16,9);
                                                ctx.bezierCurveTo(13,13.5, 5,13.5, 2,9);
                                                ctx.stroke();
                                                ctx.beginPath();
                                                ctx.arc(9,9,2.5,0,Math.PI*2);
                                                ctx.stroke();
                                            }
                                        }
                                        Text {
                                            text: "Observer"
                                            font.family: loginRoot.fontFamily
                                            font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.95) // 15px Title
                                            font.weight: Font.Bold
                                            color: colorPrimaryText
                                        }
                                    }
                                    Text {
                                        text: "Mission Monitoring"
                                        font.family: loginRoot.fontFamily
                                        font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.75) // 12px Description
                                        color: colorSecondaryText
                                        elide: Text.ElideRight
                                    }
                                }
                                MouseArea {
                                    id: observerMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: roleBar.selectedIndex = 3
                                }
                            }
                        }
                    }

                    // 4. USERNAME FIELD (Height: 52-54px, id: userField, Leading User Outline Icon 18px)
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Text {
                            text: "Operator ID / Username"
                            font.family: loginRoot.fontFamily
                            font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.8) // 13px Label
                            font.weight: Font.Medium
                            color: colorSecondaryText
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: Math.round(ScreenTools.defaultFontPixelHeight * 3.3) // ~52-54px
                            radius: loginRoot.cornerRadiusControl
                            color: colorInput
                            border.color: userField.activeFocus ? colorPrimaryText : colorBorder
                            border.width: 1

                            Behavior on border.color { ColorAnimation { duration: loginRoot.animDuration } }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 14
                                anchors.rightMargin: 14
                                spacing: 10

                                Canvas {
                                    width: 18; height: 18 // 18px Icon
                                    onPaint: {
                                        var ctx = getContext("2d");
                                        ctx.clearRect(0,0,18,18);
                                        ctx.strokeStyle = userField.activeFocus ? colorPrimaryText : colorSecondaryText;
                                        ctx.lineWidth = 1.5;
                                        ctx.beginPath();
                                        ctx.arc(9, 6, 3.5, 0, Math.PI * 2);
                                        ctx.stroke();
                                        ctx.beginPath();
                                        ctx.moveTo(3, 16);
                                        ctx.bezierCurveTo(4, 11, 14, 11, 15, 16);
                                        ctx.stroke();
                                    }
                                }

                                QGCTextField {
                                    id: userField
                                    Layout.fillWidth: true
                                    text: ""
                                    font.family: loginRoot.fontFamily
                                    font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.95) // 15px Body
                                    color: colorPrimaryText
                                    placeholderText: "Enter Operator ID or Username"
                                    focus: true
                                    KeyNavigation.tab: passField
                                    Keys.onReturnPressed: passField.forceActiveFocus()
                                    Keys.onEnterPressed: passField.forceActiveFocus()
                                }
                            }
                        }
                    }

                    // 5. PASSWORD FIELD (Height: 52-54px, id: passField, Leading Lock Icon 18px, Trailing Eye Icon 18px)
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Text {
                            text: "Password"
                            font.family: loginRoot.fontFamily
                            font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.8) // 13px Label
                            font.weight: Font.Medium
                            color: colorSecondaryText
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: Math.round(ScreenTools.defaultFontPixelHeight * 3.3) // ~52-54px
                            radius: loginRoot.cornerRadiusControl
                            color: colorInput
                            border.color: passField.activeFocus ? colorPrimaryText : colorBorder
                            border.width: 1

                            Behavior on border.color { ColorAnimation { duration: loginRoot.animDuration } }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 14
                                anchors.rightMargin: 14
                                spacing: 10

                                Canvas {
                                    width: 18; height: 18 // 18px Icon
                                    onPaint: {
                                        var ctx = getContext("2d");
                                        ctx.clearRect(0,0,18,18);
                                        ctx.strokeStyle = passField.activeFocus ? colorPrimaryText : colorSecondaryText;
                                        ctx.lineWidth = 1.5;
                                        ctx.beginPath();
                                        ctx.arc(9, 6.5, 3.5, Math.PI, 0, false);
                                        ctx.lineTo(12.5, 9);
                                        ctx.lineTo(5.5, 9);
                                        ctx.stroke();
                                        ctx.strokeRect(4, 9, 10, 7);
                                    }
                                }

                                Item {
                                    id: passVisibleToggle
                                    width: 18; height: 18
                                    property bool showPassword: false

                                    Canvas {
                                        id: eyeCanvas
                                        anchors.fill: parent
                                        property bool showPassword: passVisibleToggle.showPassword
                                        onShowPasswordChanged: requestPaint()

                                        onPaint: {
                                            var ctx = getContext("2d");
                                            ctx.clearRect(0,0,18,18);
                                            ctx.strokeStyle = showPassword ? colorPrimaryText : colorSecondaryText;
                                            ctx.lineWidth = 1.5;
                                            ctx.beginPath();
                                            ctx.moveTo(2,9);
                                            ctx.bezierCurveTo(5,4.5, 13,4.5, 16,9);
                                            ctx.bezierCurveTo(13,13.5, 5,13.5, 2,9);
                                            ctx.stroke();
                                            ctx.beginPath();
                                            ctx.arc(9,9,2.5,0,Math.PI*2);
                                            ctx.stroke();
                                            if (!showPassword) {
                                                ctx.beginPath();
                                                ctx.moveTo(4,4); ctx.lineTo(14,14);
                                                ctx.stroke();
                                            }
                                        }
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: passVisibleToggle.showPassword = !passVisibleToggle.showPassword
                                    }
                                }

                                QGCTextField {
                                    id: passField
                                    Layout.fillWidth: true
                                    text: ""
                                    echoMode: passVisibleToggle.showPassword ? TextInput.Normal : TextInput.Password
                                    font.family: loginRoot.fontFamily
                                    font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.95) // 15px Body
                                    color: colorPrimaryText
                                    placeholderText: "Enter Password"
                                    KeyNavigation.tab: userField
                                    Keys.onReturnPressed: loginRoot.doLogin()
                                    Keys.onEnterPressed: loginRoot.doLogin()
                                }
                            }
                        }
                    }

                    // 6. LOGIN BUTTON - PRIMARY ACTION (Volador Orange: #FF6A00, Height: 56px)
                    Rectangle {
                        id: loginBtnRect
                        Layout.fillWidth: true
                        height: Math.round(ScreenTools.defaultFontPixelHeight * 3.5) // 56px
                        radius: loginRoot.cornerRadiusControl
                        color: btnMouseArea.enabled ? (btnMouseArea.pressed ? colorOrangePressed : (btnMouseArea.containsMouse ? colorOrangeHover : colorOrange)) : colorButtonDisabled
                        border.color: btnMouseArea.enabled ? colorOrangeHover : colorBorder
                        border.width: 1

                        Behavior on color { ColorAnimation { duration: loginRoot.animDuration } }

                        Text {
                            anchors.centerIn: parent
                            text: (typeof voladorAuth !== "undefined" && voladorAuth && voladorAuth.isBusy) ? "AUTHENTICATING OPERATOR..." : "AUTHENTICATE & UNLOCK GCS"
                            font.family: loginRoot.fontFamily
                            font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.95) // 15px SemiBold
                            font.weight: Font.DemiBold
                            color: "#FFFFFF"
                        }

                        MouseArea {
                            id: btnMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            enabled: !(typeof voladorAuth !== "undefined" && voladorAuth && voladorAuth.isBusy)
                            onClicked: {
                                userField.focus = false
                                passField.focus = false
                                loginRoot.doLogin()
                            }
                        }
                    }

                    // 7. FOOTER (Forgot Password • Diagnostics • System Logs • Restart Services • Shutdown)
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Text {
                            text: "Forgot Password?"
                            font.family: loginRoot.fontFamily
                            font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.75) // 12px Medium
                            font.weight: Font.Medium
                            color: forgotMouse.containsMouse ? colorPrimaryText : colorSecondaryText

                            Behavior on color { ColorAnimation { duration: loginRoot.animDuration } }

                            MouseArea {
                                id: forgotMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (typeof mainWindow !== "undefined" && mainWindow.showMessageDialog) {
                                        mainWindow.showMessageDialog("Password Reset", "Please contact your GCS System Administrator to issue a key reset.")
                                    }
                                }
                            }
                        }

                        Item { Layout.fillWidth: true } // Spacer

                        RowLayout {
                            spacing: 8

                            Text {
                                text: "Diagnostics"
                                font.family: loginRoot.fontFamily
                                font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.75) // 12px Medium
                                font.weight: Font.Medium
                                color: diagMouse.containsMouse ? colorPrimaryText : colorSecondaryText
                                Behavior on color { ColorAnimation { duration: loginRoot.animDuration } }
                                MouseArea {
                                    id: diagMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                }
                            }

                            Text {
                                text: "•"
                                color: colorSecondaryText
                                font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.75)
                            }

                            Text {
                                text: "System Logs"
                                font.family: loginRoot.fontFamily
                                font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.75) // 12px Medium
                                font.weight: Font.Medium
                                color: logsMouse.containsMouse ? colorPrimaryText : colorSecondaryText
                                Behavior on color { ColorAnimation { duration: loginRoot.animDuration } }
                                MouseArea {
                                    id: logsMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                }
                            }

                            Text {
                                text: "•"
                                color: colorSecondaryText
                                font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.75)
                            }

                            Text {
                                text: "Restart Services"
                                font.family: loginRoot.fontFamily
                                font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.75) // 12px Medium
                                font.weight: Font.Medium
                                color: restartMouse.containsMouse ? colorPrimaryText : colorSecondaryText
                                Behavior on color { ColorAnimation { duration: loginRoot.animDuration } }
                                MouseArea {
                                    id: restartMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                }
                            }

                            Text {
                                text: "•"
                                color: colorSecondaryText
                                font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.75)
                            }

                            Text {
                                text: "Shutdown"
                                font.family: loginRoot.fontFamily
                                font.pixelSize: Math.round(ScreenTools.defaultFontPixelHeight * 0.75) // 12px Medium
                                font.weight: Font.Medium
                                color: shutMouse.containsMouse ? colorPrimaryText : colorSecondaryText
                                Behavior on color { ColorAnimation { duration: loginRoot.animDuration } }
                                MouseArea {
                                    id: shutMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (typeof mainWindow !== "undefined" && mainWindow.close) {
                                            mainWindow.close()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
