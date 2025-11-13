import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.Palette

Rectangle {
    id: root
    color: qgcPal.window

    property var metricsManager: TileMetricsManager
    property var qualitySettings: TileQualitySettings

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: ScreenTools.defaultFontPixelWidth
        spacing: ScreenTools.defaultFontPixelWidth

        // Header
        QGCLabel {
            text: qsTr("Map Tile Metrics Dashboard")
            font.pointSize: ScreenTools.largeFontPointSize
            font.bold: true
        }

        // Overview Stats
        GridLayout {
            Layout.fillWidth: true
            columns: 3
            rowSpacing: ScreenTools.defaultFontPixelHeight
            columnSpacing: ScreenTools.defaultFontPixelWidth * 2

            // Overall Success Rate
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 6
                color: qgcPal.windowShade
                radius: ScreenTools.defaultFontPixelWidth * 0.5

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: ScreenTools.defaultFontPixelWidth

                    QGCLabel {
                        text: qsTr("Success Rate")
                        font.bold: true
                    }

                    QGCLabel {
                        text: (metricsManager.overallSuccessRate * 100).toFixed(1) + "%"
                        font.pointSize: ScreenTools.largeFontPointSize * 1.5
                        color: metricsManager.overallSuccessRate > 0.95 ? "green" :
                               metricsManager.overallSuccessRate > 0.80 ? "orange" : "red"
                    }
                }
            }

            // Cache Hit Rate
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 6
                color: qgcPal.windowShade
                radius: ScreenTools.defaultFontPixelWidth * 0.5

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: ScreenTools.defaultFontPixelWidth

                    QGCLabel {
                        text: qsTr("Cache Hit Rate")
                        font.bold: true
                    }

                    QGCLabel {
                        text: (metricsManager.cacheHitRate * 100).toFixed(1) + "%"
                        font.pointSize: ScreenTools.largeFontPointSize * 1.5
                        color: metricsManager.cacheHitRate > 0.80 ? "green" :
                               metricsManager.cacheHitRate > 0.50 ? "orange" : "red"
                    }
                }
            }

            // Average Response Time
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 6
                color: qgcPal.windowShade
                radius: ScreenTools.defaultFontPixelWidth * 0.5

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: ScreenTools.defaultFontPixelWidth

                    QGCLabel {
                        text: qsTr("Avg Response Time")
                        font.bold: true
                    }

                    QGCLabel {
                        text: metricsManager.overallAvgResponseTimeMs + " ms"
                        font.pointSize: ScreenTools.largeFontPointSize * 1.5
                        color: metricsManager.overallAvgResponseTimeMs < 500 ? "green" :
                               metricsManager.overallAvgResponseTimeMs < 1000 ? "orange" : "red"
                    }
                }
            }
        }

        // Network Stats
        GroupBox {
            Layout.fillWidth: true
            title: qsTr("Network")

            GridLayout {
                anchors.fill: parent
                columns: 2
                rowSpacing: ScreenTools.defaultFontPixelHeight * 0.5
                columnSpacing: ScreenTools.defaultFontPixelWidth

                QGCLabel { text: qsTr("Download Speed:") }
                QGCLabel {
                    text: formatBytes(metricsManager.networkMetrics.currentDownloadBytesPerSec) + "/s"
                    color: metricsManager.networkMetrics.throttlingActive ? "orange" : qgcPal.text
                }

                QGCLabel { text: qsTr("Active Connections:") }
                QGCLabel { text: metricsManager.networkMetrics.activeConnections.toString() }

                QGCLabel { text: qsTr("Queued Requests:") }
                QGCLabel { text: metricsManager.networkMetrics.queuedRequests.toString() }

                QGCLabel { text: qsTr("Throttling:") }
                QGCLabel {
                    text: metricsManager.networkMetrics.throttlingActive ? qsTr("Active") : qsTr("Inactive")
                    color: metricsManager.networkMetrics.throttlingActive ? "orange" : "green"
                    font.bold: metricsManager.networkMetrics.throttlingActive
                }
            }
        }

        // Cache Stats
        GroupBox {
            Layout.fillWidth: true
            title: qsTr("Cache")

            GridLayout {
                anchors.fill: parent
                columns: 2
                rowSpacing: ScreenTools.defaultFontPixelHeight * 0.5
                columnSpacing: ScreenTools.defaultFontPixelWidth

                QGCLabel { text: qsTr("Total Hits:") }
                QGCLabel { text: metricsManager.cacheMetrics.totalHits.toString() }

                QGCLabel { text: qsTr("Total Misses:") }
                QGCLabel { text: metricsManager.cacheMetrics.totalMisses.toString() }

                QGCLabel { text: qsTr("Size:") }
                QGCLabel { text: formatBytes(metricsManager.cacheMetrics.currentSizeBytes) }

                QGCLabel { text: qsTr("Tile Count:") }
                QGCLabel { text: metricsManager.cacheMetrics.tileCount.toString() }
            }
        }

        // QoS Settings
        GroupBox {
            Layout.fillWidth: true
            title: qsTr("Quality Settings")

            GridLayout {
                anchors.fill: parent
                columns: 2
                rowSpacing: ScreenTools.defaultFontPixelHeight * 0.5
                columnSpacing: ScreenTools.defaultFontPixelWidth

                QGCLabel { text: qsTr("Tile Quality:") }
                ComboBox {
                    model: ["Low (256px)", "Medium (512px)", "High (1024px)", "Adaptive"]
                    currentIndex: qualitySettings.quality
                    onCurrentIndexChanged: qualitySettings.quality = currentIndex
                }

                QGCLabel { text: qsTr("Adaptive Quality:") }
                QGCCheckBox {
                    checked: qualitySettings.adaptiveQuality
                    onCheckedChanged: qualitySettings.adaptiveQuality = checked
                }

                QGCLabel { text: qsTr("Bandwidth Limit:") }
                RowLayout {
                    TextField {
                        id: bandwidthField
                        placeholderText: "0 = Unlimited"
                        text: (qualitySettings.bandwidthLimit / (1024 * 1024)).toFixed(1)
                        validator: DoubleValidator { bottom: 0; decimals: 1 }
                        onEditingFinished: {
                            qualitySettings.bandwidthLimit = parseFloat(text) * 1024 * 1024
                        }
                    }
                    QGCLabel { text: qsTr("MB/s") }
                }
            }
        }

        Item { Layout.fillHeight: true }
    }

    function formatBytes(bytes) {
        if (bytes < 1024) return bytes + " B"
        if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + " KB"
        if (bytes < 1024 * 1024 * 1024) return (bytes / (1024 * 1024)).toFixed(1) + " MB"
        return (bytes / (1024 * 1024 * 1024)).toFixed(1) + " GB"
    }
}
