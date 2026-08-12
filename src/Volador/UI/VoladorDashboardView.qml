/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station - Primary Command Dashboard Grid
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Palette
import VoladorTheme 1.0
import VoladorComponents 1.0

Rectangle {
    id: dashRoot
    anchors.fill: parent
    color: ThemeController.background

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: parent.width

        ColumnLayout {
            width: dashRoot.width
            spacing: 16
            Layout.margins: 16

            // Top Quick Mission Stat Banner (4 Summary Cards)
            RowLayout {
                Layout.fillWidth: true
                spacing: 16

                DashboardCard {
                    Layout.fillWidth: true
                    implicitHeight: 100
                    title: "Active Missions"
                    icon: "🎯"
                    badgeText: "3 RUNNING"
                    badgeColor: ThemeController.accent

                    Column {
                        anchors.centerIn: parent
                        Text { text: "03 / 12"; font.family: "Inter"; font.pixelSize: 22; font.weight: Font.Bold; color: ThemeController.textPrimary }
                        Text { text: "Autonomous Flight Plans"; font.family: "Inter"; font.pixelSize: 12; color: ThemeController.textSecondary }
                    }
                }

                DashboardCard {
                    Layout.fillWidth: true
                    implicitHeight: 100
                    title: "Drone Health Index"
                    icon: "🚁"
                    badgeText: "OPTIMAL"
                    badgeColor: ThemeController.success

                    Column {
                        anchors.centerIn: parent
                        Text { text: "98.4%"; font.family: "Inter"; font.pixelSize: 22; font.weight: Font.Bold; color: ThemeController.success }
                        Text { text: "Motors, ESC & Sensors Nominal"; font.family: "Inter"; font.pixelSize: 12; color: ThemeController.textSecondary }
                    }
                }

                DashboardCard {
                    Layout.fillWidth: true
                    implicitHeight: 100
                    title: "Total Airtime"
                    icon: "⏱️"
                    badgeText: "THIS MONTH"
                    badgeColor: ThemeController.secondary

                    Column {
                        anchors.centerIn: parent
                        Text { text: "142h 28m"; font.family: "Inter"; font.pixelSize: 22; font.weight: Font.Bold; color: ThemeController.textPrimary }
                        Text { text: "+14.2% vs last month"; font.family: "Inter"; font.pixelSize: 12; color: ThemeController.success }
                    }
                }

                DashboardCard {
                    Layout.fillWidth: true
                    implicitHeight: 100
                    title: "Fleet Battery Pool"
                    icon: "🔋"
                    badgeText: "8 PACKS"
                    badgeColor: ThemeController.accent

                    Column {
                        anchors.centerIn: parent
                        Text { text: "24.8V"; font.family: "JetBrains Mono"; font.pixelSize: 22; font.weight: Font.Bold; color: ThemeController.success }
                        Text { text: "Avg Temp: 28.4°C"; font.family: "Inter"; font.pixelSize: 12; color: ThemeController.textSecondary }
                    }
                }
            }

            // Main Dashboard Content Grid: 2 Columns (65% / 35%)
            RowLayout {
                Layout.fillWidth: true
                spacing: 16

                // LEFT MAIN COLUMN (65% width)
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: parent.width * 0.65
                    spacing: 16

                    // Live Drone Telemetry & Mission Map Preview Box
                    DashboardCard {
                        Layout.fillWidth: true
                        implicitHeight: 280
                        title: "Tactical GIS Map & Live Flight Track"
                        icon: "🛰️"
                        badgeText: "GPS 3D LOCK"

                        Rectangle {
                            anchors.fill: parent
                            color: ThemeController.isDark ? "#12151B" : "#EAEFF5"
                            radius: 8
                            border.color: ThemeController.border

                            // Grid Lines Graphic
                            Canvas {
                                anchors.fill: parent
                                onPaint: {
                                    var ctx = getContext("2d");
                                    ctx.strokeStyle = ThemeController.isDark ? "#1E242E" : "#D0D7E0";
                                    for (var x = 0; x < width; x += 30) { ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, height); ctx.stroke(); }
                                    for (var y = 0; y < height; y += 30) { ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(width, y); ctx.stroke(); }
                                }
                            }

                            // Flight Path Overlay
                            Canvas {
                                anchors.fill: parent
                                onPaint: {
                                    var ctx = getContext("2d");
                                    ctx.strokeStyle = "#40464D";
                                    ctx.lineWidth = 3;
                                    ctx.beginPath();
                                    ctx.moveTo(40, 200);
                                    ctx.lineTo(140, 120);
                                    ctx.lineTo(260, 160);
                                    ctx.lineTo(400, 80);
                                    ctx.lineTo(520, 140);
                                    ctx.stroke();
                                }
                            }

                            // Active Drone Marker
                            Rectangle {
                                width: 24; height: 24; radius: 12
                                x: 508; y: 128
                                color: ThemeController.accent
                                border.color: "#FFFFFF"
                                border.width: 2
                                Text { anchors.centerIn: parent; text: "✈️"; font.pixelSize: 12 }
                            }

                            // Map Controls Overlay
                            Column {
                                anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 12
                                spacing: 4
                                Rectangle { width: 32; height: 32; radius: 6; color: ThemeController.cards; Text { anchors.centerIn: parent; text: "➕"; font.pixelSize: 14 } }
                                Rectangle { width: 32; height: 32; radius: 6; color: ThemeController.cards; Text { anchors.centerIn: parent; text: "➖"; font.pixelSize: 14 } }
                                Rectangle { width: 32; height: 32; radius: 6; color: ThemeController.cards; Text { anchors.centerIn: parent; text: "🎯"; font.pixelSize: 14 } }
                            }
                        }
                    }

                    // RTSP Video Preview & Telemetry Chart
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        DashboardCard {
                            Layout.fillWidth: true
                            implicitHeight: 220
                            title: "HD RTSP Video Stream"
                            icon: "📹"
                            badgeText: "1080p 60FPS"

                            Rectangle {
                                anchors.fill: parent
                                color: "#000000"
                                radius: 8

                                Text {
                                    anchors.centerIn: parent
                                    text: "LIVE RTSP STREAM ONLINE\n[GStreamer Pipeline Active]"
                                    font.family: "JetBrains Mono"
                                    font.pixelSize: 13
                                    color: ThemeController.success
                                    horizontalAlignment: Text.AlignHCenter
                                }

                                Rectangle {
                                    anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.margins: 10
                                    height: 24; implicitWidth: 120; radius: 4; color: "#CC000000"
                                    Text { anchors.centerIn: parent; text: "BITRATE: 4.8 Mbps"; font.family: "JetBrains Mono"; font.pixelSize: 10; color: "#FFFFFF" }
                                }
                            }
                        }

                        DashboardCard {
                            Layout.fillWidth: true
                            implicitHeight: 220
                            title: "Live Airspeed & Altitude Chart"
                            icon: "📈"
                            badgeText: "REALTIME"

                            Canvas {
                                anchors.fill: parent
                                onPaint: {
                                    var ctx = getContext("2d");
                                    ctx.strokeStyle = "#40464D";
                                    ctx.lineWidth = 2;
                                    ctx.beginPath();
                                    for (var x = 0; x < width; x += 10) {
                                        var y = height/2 + Math.sin(x/20) * 40;
                                        if (x === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
                                    }
                                    ctx.stroke();
                                }
                            }
                        }
                    }
                }

                // RIGHT SIDEBAR COLUMN (35% width)
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: parent.width * 0.35
                    spacing: 16

                    // Weather & Microclimate Widget
                    DashboardCard {
                        Layout.fillWidth: true
                        implicitHeight: 180
                        title: "Microclimate & Solar WX"
                        icon: "🌤️"

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 8

                            RowLayout {
                                Text { text: "24°C"; font.family: "Inter"; font.pixelSize: 28; font.weight: Font.Bold; color: ThemeController.textPrimary }
                                Column {
                                    Text { text: "Sunny / Clear Skies"; font.family: "Inter"; font.pixelSize: 12; color: ThemeController.textPrimary }
                                    Text { text: "Visibility: 10 km"; font.family: "Inter"; font.pixelSize: 11; color: ThemeController.textSecondary }
                                }
                            }

                            Rectangle { Layout.fillWidth: true; height: 1; color: ThemeController.border }

                            RowLayout {
                                Layout.fillWidth: true
                                Column { Text { text: "Wind"; font.pixelSize: 11; color: ThemeController.textSecondary } Text { text: "8 km/h NW"; font.weight: Font.Bold; font.pixelSize: 12; color: ThemeController.textPrimary } }
                                Column { Text { text: "Humidity"; font.pixelSize: 11; color: ThemeController.textSecondary } Text { text: "42%"; font.weight: Font.Bold; font.pixelSize: 12; color: ThemeController.textPrimary } }
                                Column { Text { text: "KP-Index"; font.pixelSize: 11; color: ThemeController.textSecondary } Text { text: "1.2 (Low)"; font.weight: Font.Bold; font.pixelSize: 12; color: ThemeController.success } }
                            }
                        }
                    }

                    // System Alerts Feed
                    DashboardCard {
                        Layout.fillWidth: true
                        implicitHeight: 334
                        title: "System Telemetry Alerts"
                        icon: "🚨"
                        badgeText: "2 ALERTS"
                        badgeColor: ThemeController.warning

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 8

                            Rectangle {
                                Layout.fillWidth: true; height: 50; radius: 6
                                color: ThemeController.isDark ? "#2A2017" : "#FFF8EC"
                                border.color: ThemeController.warning
                                RowLayout {
                                    anchors.fill: parent; anchors.margins: 8; spacing: 8
                                    Text { text: "⚠️"; font.pixelSize: 16 }
                                    Column {
                                        Text { text: "Geofence Warning"; font.family: "Inter"; font.pixelSize: 12; font.weight: Font.Bold; color: ThemeController.warning }
                                        Text { text: "Vehicle approaching 200m radius border"; font.family: "Inter"; font.pixelSize: 11; color: ThemeController.textSecondary }
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true; height: 50; radius: 6
                                color: ThemeController.isDark ? "#172A20" : "#ECFFF5"
                                border.color: ThemeController.success
                                RowLayout {
                                    anchors.fill: parent; anchors.margins: 8; spacing: 8
                                    Text { text: "✅"; font.pixelSize: 16 }
                                    Column {
                                        Text { text: "Compass Calibration Passed"; font.family: "Inter"; font.pixelSize: 12; font.weight: Font.Bold; color: ThemeController.success }
                                        Text { text: "Mag 1 & Mag 2 offset within limits"; font.family: "Inter"; font.pixelSize: 11; color: ThemeController.textSecondary }
                                    }
                                }
                            }

                            Item { Layout.fillHeight: true }
                        }
                    }
                }
            }
        }
    }
}
