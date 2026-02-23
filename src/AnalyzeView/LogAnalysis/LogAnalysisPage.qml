import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.qmlmodels
import QtGraphs
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

AnalyzePage {
    id:                 root
    pageDescription:    qsTr("Load and analyze PX4 flight logs.")
    allowPopout:        true

    property var _loader: logLoader

    PX4LogLoader {
        id: logLoader

        onLoadedChanged: {
            treeState.rebuildTreeRows()
            flightPath.loadFromLog()

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
            const topicHierarchy = {}  // "path" -> { label, children }
            const leafTopics = new Set()  // paths that are actual topics

            if (logLoader.loaded) {
                // First pass: collect all topics and identify leaf nodes
                for (const encodedName of logLoader.topicNames) {
                    const [levelStr, topicPath] = encodedName.split(':')
                    const level = parseInt(levelStr)

                    const topicObj = logLoader.topic(topicPath)
                    if (topicObj && topicObj.fieldNames.length > 0) {
                        leafTopics.add(topicPath)
                    }
                }

                // Second pass: build hierarchy
                for (const encodedName of logLoader.topicNames) {
                    const [levelStr, topicPath] = encodedName.split(':')
                    const level = parseInt(levelStr)

                    if (!topicHierarchy[topicPath]) {
                        topicHierarchy[topicPath] = {
                            label: topicPath.split('_').pop(),
                            children: [],
                            isLeaf: leafTopics.has(topicPath)
                        }
                    }
                }

                // Third pass: build tree structure
                for (const topicPath of Object.keys(topicHierarchy).sort()) {
                    const parts = topicPath.split('_')
                    const parentPath = parts.length > 1 ? parts.slice(0, -1).join('_') : null

                    // Treat as root if no parent, or if parent doesn't exist in hierarchy (flattened)
                    if (!parentPath || !topicHierarchy[parentPath]) {
                        // Root level node
                        const treeNode = _buildTreeNode(topicPath, topicHierarchy, leafTopics)
                        if (treeNode) {
                            rows.push(treeNode)
                        }
                    }
                }
            }

            treeRows = rows
        }

        function _buildTreeNode(topicPath, topicHierarchy, leafTopics) {
            const node = topicHierarchy[topicPath]
            if (!node) return null

            const topicObj = logLoader.topic(topicPath)
            const isLeaf = leafTopics.has(topicPath)

            // Collect children
            const childPaths = []
            for (const othePath of Object.keys(topicHierarchy)) {
                const otherParts = othePath.split('_')
                const currentParts = topicPath.split('_')

                // Check if othePath is a direct child of topicPath
                if (otherParts.length === currentParts.length + 1) {
                    if (otherParts.slice(0, -1).join('_') === topicPath) {
                        childPaths.push(othePath)
                    }
                }
            }

            childPaths.sort()

            const children = []
            for (const childPath of childPaths) {
                const childNode = _buildTreeNode(childPath, topicHierarchy, leafTopics)
                if (childNode) {
                    children.push(childNode)
                }
            }

            // Determine label and node info
            // For leaf topics (actual topics), use full path. For hierarchy nodes, use last component.
            const displayLabel = isLeaf ? topicPath : topicPath.split('_').pop()

            // If this is a leaf topic, add fields as children
            if (isLeaf && topicObj) {
                for (const fieldName of topicObj.fieldNames) {
                    children.push({
                        label: fieldName,
                        topic: topicPath
                    })
                }

                // For leaf topics, include sample count in label
                const fullLabel = displayLabel + roleSep + (topicObj.sampleCount || 0)
                return {
                    label: fullLabel,
                    topic: topicPath,  // Include topic path for leaf nodes
                    rows: children.length > 0 ? children : undefined
                }
            } else {
                // For hierarchy nodes, just return the group label
                return {
                    label: displayLabel,
                    rows: children.length > 0 ? children : undefined
                }
            }
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
            seriesMinX = 0
            seriesMaxX = maxX / 60.0
            seriesMinY = minY - yPadding
            seriesMaxY = maxY + yPadding
        }

        property var currentTopic: selectedTopic.length > 0 ? logLoader.topic(selectedTopic) : null
    }

    QtObject {
        id: flightPath
        property var coordinates: []
        property var boundingBox: null

        function loadFromLog() {
            coordinates = []
            boundingBox = null

            if (!logLoader.loaded) {
                return
            }

            const posTopic = logLoader.topic("vehicle_global_position")
            if (!posTopic) {
                return
            }

            // Get lat and lon field data
            const latSeries = posTopic.fieldSeries("lat")
            const lonSeries = posTopic.fieldSeries("lon")

            if (!latSeries || !lonSeries || latSeries.length === 0) {
                return
            }

            const pathCoords = []
            let minLat = latSeries[0].y
            let maxLat = minLat
            let minLon = lonSeries[0].y
            let maxLon = minLon

            for (let i = 0; i < latSeries.length; i++) {
                const lat = latSeries[i].y
                const lon = lonSeries[i].y
                pathCoords.push(QtPositioning.coordinate(lat, lon))

                if (lat < minLat) minLat = lat
                if (lat > maxLat) maxLat = lat
                if (lon < minLon) minLon = lon
                if (lon > maxLon) maxLon = lon
            }

            coordinates = pathCoords
            boundingBox = QtPositioning.rectangle(
                QtPositioning.coordinate(maxLat, minLon),
                QtPositioning.coordinate(minLat, maxLon)
            )
        }

        function zoomToPath(map) {
            if (boundingBox && map.mapReady) {
                map.visibleRegion = boundingBox
            }
        }
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

            // --- Open/Clear buttons (always visible) ---
            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth

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

                Item { Layout.fillWidth: true }
            }

            // --- Error display ---
            QGCLabel {
                Layout.fillWidth:   true
                text:               logLoader.errorMessage
                color:              "red"
                visible:            logLoader.errorMessage.length > 0
                wrapMode:           Text.WordWrap
            }

            // --- Main content row ---
            RowLayout {
                Layout.fillWidth:   true
                Layout.fillHeight:  true
                spacing:            ScreenTools.defaultFontPixelWidth
                visible:            logLoader.loaded

                // --- Left column: tree ---
                ColumnLayout {
                    Layout.fillWidth:       false
                    Layout.fillHeight:      true
                    Layout.minimumWidth:    ScreenTools.defaultFontPixelWidth * 25
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 35
                    spacing:                ScreenTools.defaultFontPixelHeight * 0.25

                    // --- Unified topic/field tree ---
                    Rectangle {
                        Layout.fillWidth:   true
                        Layout.fillHeight:  true
                        color:              qgcPal.windowShade
                        border.color:       qgcPal.groupBorder
                        border.width:       1

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
                                    readonly property string displayText: parts[0]
                                    readonly property int sampleCount: parts.length > 1 ? Number(parts[1]) : 0
                                    readonly property string topicPath: (model && model.topic !== undefined) ? model.topic : ""
                                    readonly property bool isField: topicPath.length > 0 && sampleCount === 0  // Fields have topic path but no sample count
                                    readonly property bool isTopic: sampleCount > 0  // Topics have sample count

                                    text: {
                                        if (isTopic) {
                                            return displayText + " (" + sampleCount + ")"
                                        } else {
                                            return displayText
                                        }
                                    }
                                    font.bold: depth === 0 || isTopic
                                    highlighted: {
                                        if (isField) {
                                            return treeState.selectedTopic === topicPath && treeState.selectedField === displayText
                                        } else if (isTopic) {
                                            return treeState.selectedTopic === topicPath && treeState.selectedField.length === 0
                                        }
                                        return false
                                    }
                                    leftPadding: depth === 0 ? 0 : ScreenTools.defaultFontPixelWidth * 1.5

                                    onClicked: {
                                        if (isField) {
                                            treeState.selectField(topicPath, displayText)
                                        } else if (isTopic) {
                                            treeState.selectTopic(topicPath)
                                        }
                                    }

                                    TapHandler {
                                        onTapped: onClicked()
                                    }
                                }
                            }
                        }
                    }
                }

                // --- Right column: chart ---
                ColumnLayout {
                    Layout.fillWidth:   true
                    Layout.fillHeight:  true
                    Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 20
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
                                tickInterval: 1.0
                                labelFormat: "%.1f"
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
                                    fieldSeries.append(points[i].x / 60000000.0, points[i].y)
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

                    // Map display
                    FlightMap {
                        id: logMap
                        Layout.fillWidth:   true
                        Layout.fillHeight:  true
                        mapName:            'logAnalysisMap'

                        onMapReadyChanged: {
                            if (logMap.mapReady) {
                                flightPath.zoomToPath(logMap)
                            }
                        }

                        MapPolyline {
                            line.color: "#e74c3c"
                            line.width: 2
                            path: flightPath.coordinates
                        }

                        Connections {
                            target: flightPath
                            function onCoordinatesChanged() {
                                flightPath.zoomToPath(logMap)
                            }
                        }
                    }
                }
            }
        }
    }
}
