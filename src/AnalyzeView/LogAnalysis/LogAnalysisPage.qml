import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.qmlmodels
import QtGraphs

import QGroundControl
import QGroundControl.Controls

AnalyzePage {
    id:                 root
    pageDescription:    qsTr("Load and analyze PX4 flight logs.")
    allowPopout:        true

    property var _loader: logLoader

    PX4LogLoader {
        id: logLoader

        onLoadedChanged: {
            treeState.rebuildTreeRows()

            if (!logLoader.loaded) {
                treeState.clearSelection()
                return
            }

            if (treeState.selectedTopic.length === 0) {
                return
            }

            if (logLoader.topicNames.indexOf(treeState.selectedTopic) === -1) {
                treeState.clearSelection()
                return
            }

            if (treeState.selectedField.length > 0) {
                const topicObj = logLoader.topic(treeState.selectedTopic)
                if (!topicObj || topicObj.fieldNames.indexOf(treeState.selectedField) === -1) {
                    treeState.selectedField = ""
                }
            }
        }
    }

    QtObject {
        id: treeState
        property string roleSep: "\u001F"
        property var treeRows: []
        property string selectedTopic: ""
        property string selectedField: ""
        property var selectedSeries: []
        property real seriesMinX: 0
        property real seriesMaxX: 1
        property real seriesMinY: 0
        property real seriesMaxY: 1

        function clearSelection() {
            selectedTopic = ""
            selectedField = ""
            updateSeries()
        }

        function rebuildTreeRows() {
            const rows = []

            if (logLoader.loaded) {
                for (const topicName of logLoader.topicNames) {
                    const topicObj = logLoader.topic(topicName)
                    const fieldRows = []

                    if (topicObj) {
                        for (const fieldName of topicObj.fieldNames) {
                            fieldRows.push({
                                label: topicName + roleSep + fieldName,
                            })
                        }
                    }

                    rows.push({
                        label: topicName + roleSep + (topicObj ? topicObj.sampleCount : 0),
                        rows: fieldRows,
                    })
                }
            }

            treeRows = rows
        }

        function selectTopic(topicName) {
            selectedTopic = topicName
            selectedField = ""
            updateSeries()
        }

        function selectField(topicName, fieldName) {
            selectedTopic = topicName
            selectedField = fieldName
            updateSeries()
        }

        function updateSeries() {
            if (!currentTopic || selectedField.length === 0) {
                selectedSeries = []
                seriesMinX = 0
                seriesMaxX = 1
                seriesMinY = 0
                seriesMaxY = 1
                return
            }

            const points = currentTopic.fieldSeries(selectedField)
            selectedSeries = points

            if (!points || points.length === 0) {
                seriesMinX = 0
                seriesMaxX = 1
                seriesMinY = 0
                seriesMaxY = 1
                return
            }

            let minX = points[0].x / 1000000.0
            let maxX = minX
            let minY = points[0].y
            let maxY = minY

            for (let i = 1; i < points.length; i++) {
                const x = points[i].x / 1000000.0
                const y = points[i].y

                if (x < minX) minX = x
                if (x > maxX) maxX = x
                if (y < minY) minY = y
                if (y > maxY) maxY = y
            }

            if (Math.abs(maxX - minX) < 0.000001) {
                maxX = minX + 1.0
            }
            if (Math.abs(maxY - minY) < 0.000001) {
                maxY = minY + 1.0
            }

            const yPadding = (maxY - minY) * 0.05
            seriesMinX = minX
            seriesMaxX = maxX
            seriesMinY = minY - yPadding
            seriesMaxY = maxY + yPadding
        }

        property var currentTopic: selectedTopic.length > 0 ? logLoader.topic(selectedTopic) : null
    }

    TreeModel {
        id: topicTreeModel

        TableModelColumn {
            display: "label"
        }

        rows: treeState.treeRows
    }

    pageComponent: Component {
        ColumnLayout {
            width:      availableWidth
            height:     availableHeight
            spacing:    ScreenTools.defaultFontPixelHeight * 0.5

            // --- File selection row ---
            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    text: logLoader.loaded
                          ? qsTr("Loaded: %1 topics, %2 parameters").arg(logLoader.topicNames.length).arg(Object.keys(logLoader.parameters).length)
                          : qsTr("No log loaded")
                }

                Item { Layout.fillWidth: true }

                QGCButton {
                    text: qsTr("Open Log...")
                    onClicked: fileDialog.openForLoad()

                    QGCFileDialog {
                        id:             fileDialog
                        title:          qsTr("Select PX4 Flight Log")
                        folder:         QGroundControl.settingsManager.appSettings.logSavePath
                        nameFilters:    [qsTr("ULog files (*.ulg)"), qsTr("All Files (*)")]
                        defaultSuffix:  "ulg"
                        onAcceptedForLoad: (file) => logLoader.load(file)
                    }
                }

                QGCButton {
                    text:       qsTr("Clear")
                    enabled:    logLoader.loaded
                    onClicked:  logLoader.clear()
                }
            }

            // --- Error display ---
            QGCLabel {
                Layout.fillWidth:   true
                text:               logLoader.errorMessage
                color:              "red"
                visible:            logLoader.errorMessage.length > 0
                wrapMode:           Text.WordWrap
            }

            // --- Main content ---
            RowLayout {
                Layout.fillWidth:   true
                Layout.fillHeight:  true
                spacing:            ScreenTools.defaultFontPixelWidth
                visible:            logLoader.loaded

                // --- Unified topic/field tree ---
                Rectangle {
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 35
                    Layout.fillHeight:      true
                    color:                  qgcPal.windowShade
                    border.color:           qgcPal.groupBorder
                    border.width:           1

                    QGCFlickable {
                        anchors.fill:       parent
                        anchors.margins:    1
                        clip:               true

                        TreeView {
                            id: treeView
                            anchors.fill: parent
                            clip: true
                            model: topicTreeModel

                            delegate: TreeViewDelegate {
                                readonly property string encodedLabel: (model && model.display !== undefined) ? model.display : ""
                                readonly property var parts: encodedLabel.split(treeState.roleSep)
                                readonly property bool isTopicNode: depth === 0
                                readonly property string topicName: parts.length > 0 ? parts[0] : ""
                                readonly property string fieldName: (!isTopicNode && parts.length > 1) ? parts[1] : ""
                                readonly property int sampleCount: (isTopicNode && parts.length > 1) ? Number(parts[1]) : 0

                                text: isTopicNode
                                      ? qsTr("%1 (%2)").arg(topicName).arg(sampleCount)
                                      : fieldName
                                font.bold: isTopicNode
                                highlighted: isTopicNode
                                             ? treeState.selectedTopic === topicName && treeState.selectedField.length === 0
                                             : treeState.selectedTopic === topicName && treeState.selectedField === fieldName

                                onClicked: {
                                    if (isTopicNode) {
                                        treeState.selectTopic(topicName)
                                    } else if (topicName.length > 0 && fieldName.length > 0) {
                                        treeState.selectField(topicName, fieldName)
                                    }
                                }

                                TapHandler {
                                    onTapped: {
                                        if (isTopicNode) {
                                            treeState.selectTopic(topicName)
                                        } else if (topicName.length > 0 && fieldName.length > 0) {
                                            treeState.selectField(topicName, fieldName)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // --- Detail / chart panel ---
                ColumnLayout {
                    Layout.fillWidth:   true
                    Layout.fillHeight:  true
                    spacing:            ScreenTools.defaultFontPixelHeight * 0.25

                    QGCLabel {
                        text: {
                            if (!treeState.currentTopic)
                                return qsTr("Select a topic or field")
                            if (treeState.selectedField.length > 0)
                                return qsTr("%1 : %2").arg(treeState.selectedTopic).arg(treeState.selectedField)
                            return qsTr("%1 — %2 samples, %3 fields")
                                .arg(treeState.currentTopic.name)
                                .arg(treeState.currentTopic.sampleCount)
                                .arg(treeState.currentTopic.fieldNames.length)

                            treeState.updateSeries()
                        }
                        font.bold: true
                    }

                    // Chart / detail area
                    Rectangle {
                        Layout.fillWidth:   true
                        Layout.fillHeight:  true
                        color:              qgcPal.windowShade
                        border.color:       qgcPal.groupBorder
                        border.width:       1
                        visible:            treeState.currentTopic !== null

                        GraphsView {
                            id: fieldChart
                            anchors.fill: parent
                            anchors.margins: ScreenTools.defaultFontPixelWidth * 0.5
                            visible: treeState.selectedField.length > 0
                            antialiasing: true

                            axisX: ValueAxis {
                                id: xAxis
                                min: treeState.seriesMinX
                                max: treeState.seriesMaxX
                                tickInterval: (treeState.seriesMaxX - treeState.seriesMinX) / 5.0
                                labelFormat: "%.2f"
                            }

                            axisY: ValueAxis {
                                id: yAxis
                                min: treeState.seriesMinY
                                max: treeState.seriesMaxY
                                tickInterval: (treeState.seriesMaxY - treeState.seriesMinY) / 5.0
                                labelFormat: "%.3g"
                            }

                            LineSeries {
                                id: fieldSeries
                                axisX: xAxis
                                axisY: yAxis
                                color: qgcPal.buttonHighlight
                                width: 1.5
                            }

                            function refreshSeries() {
                                fieldSeries.clear()

                                const points = treeState.selectedSeries
                                if (!points) {
                                    return
                                }

                                for (let i = 0; i < points.length; i++) {
                                    fieldSeries.append(points[i].x / 1000000.0, points[i].y)
                                }
                            }

                            Connections {
                                target: treeState

                                function onSelectedSeriesChanged() {
                                    fieldChart.refreshSeries()
                                }
                            }

                            Component.onCompleted: refreshSeries()
                        }

                        QGCLabel {
                            anchors.centerIn: parent
                            text: treeState.selectedField.length > 0
                                  ? qsTr("No samples available for %1 : %2").arg(treeState.selectedTopic).arg(treeState.selectedField)
                                  : qsTr("Select a field to plot")
                            visible: treeState.selectedField.length === 0 || treeState.selectedSeries.length === 0
                            color: qgcPal.colorGrey
                        }
                    }

                    // Log info summary
                    GridLayout {
                        columns:        2
                        columnSpacing:  ScreenTools.defaultFontPixelWidth
                        rowSpacing:     ScreenTools.defaultFontPixelHeight * 0.25
                        visible:        logLoader.loaded

                        QGCLabel { text: qsTr("Duration:") }
                        QGCLabel { text: (logLoader.durationUs / 1000000.0).toFixed(1) + qsTr(" s") }

                        QGCLabel { text: qsTr("Log messages:") }
                        QGCLabel { text: logLoader.logMessages.length }

                        QGCLabel { text: qsTr("Dropouts:") }
                        QGCLabel { text: logLoader.dropoutCount }
                    }
                }
            }
        }
    }
}
