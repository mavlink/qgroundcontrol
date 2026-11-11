import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap
import QGroundControl.GeoMap
import QGroundControl.LogViewer

AnalyzePage {
    id: logViewerPage
    pageComponent: pageComponent
    pageDescription: qsTr("Open and inspect DataFlash (.bin), PX4 ULog (.ulg), and telemetry (.tlog) logs in a unified workflow.")
    allowPopout: true

    // Where the cursor of the charts is along the path
    component PositionDot: Rectangle {
        width: ScreenTools.defaultFontPixelHeight * 1.2
        height: width
        radius: width / 2
        color: QGroundControl.globalPalette.colorYellow
        border.color: "white"
        border.width: 2
    }

    Component {
        id: pageComponent

        ColumnLayout {
            width: availableWidth
            height: availableHeight
            spacing: ScreenTools.defaultFontPixelHeight

            property string pendingBinFile: ""

            readonly property bool _xAxisShowLocalTime: QGroundControl.settingsManager.logViewerSettings.xAxisShowLocalTime.rawValue

            readonly property bool isFirmwareLog: logViewerController.sourceType === LogViewerController.Bin
                                             || logViewerController.sourceType === LogViewerController.ULog

            // Cancel any in-flight async parse before QML starts tearing down the tree.
            // Without this, the background thread can emit signals (parseProgressChanged,
            // parseFileFinished) into partially-destroyed QML objects, causing a
            // QQmlData::disconnectNotifiers crash.
            Component.onDestruction: logParser.clear()

            function clearLoadedLogState(clearControllerState) {
                replayController.isPlaying = false
                replayController.link = null
                logParser.clear()
                logViewerController.setPlottableFields([])
                logViewerController.clearSelection()
                _parametersTab.applyFilter()
                logViewerChart.clearMarker()
                logViewerChart.refreshBinChart()
                _mapTab._sharedCursorT = NaN
                _mapTab._sharedZoomMinX = NaN
                _mapTab._sharedZoomMaxX = NaN
                if (clearControllerState) {
                    logViewerController.clear()
                }
            }

            function loadBinFile(file) {
                if (logViewerController.hasLoadedLog) {
                    // Match explicit "Clear" behavior before loading replacement .bin file.
                    clearLoadedLogState(true)
                }
                pendingBinFile = file
                logParser.startParsingAsync(file)
            }

            LogViewerController {
                id: logViewerController
            }

            LogFileParser {
                id: logParser
            }

            Connections {
                target: logParser
                ignoreUnknownSignals: true

                function onParseFileFinished(filePath, ok, errorMessage) {
                    if (filePath !== pendingBinFile) {
                        return
                    }

                    if (!ok) {
                        QGroundControl.showMessageDialog(logViewerPage, qsTr("Log Viewer"), errorMessage)
                        pendingBinFile = ""
                        return
                    }

                    fieldsPanel.rebuildGroupedFields()
                    _parametersTab.applyFilter()
                    logViewerController.clearSelection()
                    logViewerChart.clearMarker()
                    logViewerChart.refreshBinChart()
                    Qt.callLater(logViewerChart.centerCursor)

                    const lowerPath = filePath.toLowerCase()
                    if (lowerPath.endsWith(".ulg")) {
                        logViewerController.openULogFile(filePath)
                    } else {
                        logViewerController.openBinLog(filePath)
                    }
                    pendingBinFile = ""
                }
            }

            LogReplayLinkController {
                id: replayController
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth

                QGCButton {
                    objectName: "logViewer_openBinButton"
                    text: qsTr("Open .bin")
                    visible: QGroundControl.apmFirmwareSupported
                    onClicked: {
                        openDialog.nameFilters = ["DataFlash Logs (*.bin *.BIN *.log *.LOG)"]
                        openDialog.openForLoad()
                    }
                }

                QGCButton {
                    text: qsTr("Open .ulg")
                    onClicked: {
                        openDialog.nameFilters = ["PX4 ULog Files (*.ulg *.ULG)"]
                        openDialog.openForLoad()
                    }
                }

                QGCButton {
                    text: qsTr("Open .tlog")
                    onClicked: {
                        const activeVehicle = QGroundControl.multiVehicleManager.activeVehicle
                        if (activeVehicle && !activeVehicle.isOfflineEditingVehicle) {
                            QGroundControl.showMessageDialog(logViewerPage, qsTr("Log Viewer"), qsTr("Close active vehicle connections before starting telemetry replay."))
                            return
                        }
                        openDialog.nameFilters = ["Telemetry Logs (*.tlog *.TLOG)"]
                        openDialog.openForLoad()
                    }
                }

                QGCButton {
                    text: qsTr("Clear")
                    enabled: logViewerController.hasLoadedLog
                    onClicked: {
                        clearLoadedLogState(true)
                    }
                }

                QGCLabel {
                    Layout.fillWidth: true
                    elide: Text.ElideMiddle
                    text: logViewerController.hasLoadedLog ? logViewerController.currentLogPath.replace(/.*[/\\]/, "") : qsTr("No log selected")
                }

                QGCLabel {
                    visible: logViewerController.hasLoadedLog
                    text: qsTr("Start time:")
                }

                QGCLabel {
                    readonly property bool _hasStartTime: logParser.startTime
                                                          && !isNaN(logParser.startTime.getTime())
                                                          && logParser.startTime.getTime() > 0
                    visible: logViewerController.hasLoadedLog
                    text: _hasStartTime
                             ? Qt.formatDateTime(logParser.startTime, Qt.locale().dateTimeFormat(Locale.ShortFormat))
                             : qsTr("N/A")
                }

                QGCLabel {
                    visible: logViewerController.hasLoadedLog && logParser.detectedVehicleType.length > 0
                    text: qsTr("Vehicle:")
                }

                QGCLabel {
                    visible: logViewerController.hasLoadedLog && logParser.detectedVehicleType.length > 0
                    text: logParser.detectedVehicleType
                }
            }

            RowLayout {
                Layout.fillWidth: true
                visible: logParser.parsing
                spacing: ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    text: qsTr("Loading...")
                }

                QGCSlider {
                    Layout.fillWidth: true
                    from: 0
                    to: 1
                    value: logParser.parseProgress
                    enabled: false
                }
            }

            RowLayout {
                Layout.fillWidth: true
                visible: logViewerController.sourceType === LogViewerController.TLog && replayController.link
                spacing: ScreenTools.defaultFontPixelWidth

                QGCButton {
                    text: replayController.isPlaying ? qsTr("Pause") : qsTr("Play")
                    onClicked: replayController.isPlaying = !replayController.isPlaying
                }

                QGCComboBox {
                    textRole: "text"
                    currentIndex: 3

                    model: ListModel {
                        ListElement { text: "0.1x";  value: 0.1  }
                        ListElement { text: "0.25x"; value: 0.25 }
                        ListElement { text: "0.5x";  value: 0.5  }
                        ListElement { text: "1x";    value: 1.0  }
                        ListElement { text: "2x";    value: 2.0  }
                        ListElement { text: "5x";    value: 5.0  }
                        ListElement { text: "10x";   value: 10.0 }
                    }

                    onActivated: (index) => replayController.playbackSpeed = model.get(index).value
                }

                QGCLabel { text: replayController.playheadTime }

                Slider {
                    id: _replaySlider
                    Layout.fillWidth: true
                    from: 0
                    to: 100

                    property bool _internalUpdate: false

                    Connections {
                        target: replayController
                        function onPercentCompleteChanged(percentComplete) {
                            _replaySlider._internalUpdate = true
                            _replaySlider.value = percentComplete
                            _replaySlider._internalUpdate = false
                        }
                    }

                    onValueChanged: {
                        if (!_internalUpdate) {
                            replayController.percentComplete = value
                        }
                    }
                }

                QGCLabel { text: replayController.totalTime }
            }

            QGCGroupBox {
                Layout.fillWidth: true
                Layout.fillHeight: true
                showDividers: false
                contentSpacing: 0

                QGCGroupBoxTab {
                    title: qsTr("Charting")
                    Layout.fillHeight: true

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: ScreenTools.defaultFontPixelWidth

                        // Left panel: stats + fields list
                        LogViewerFieldsPanel {
                            id: fieldsPanel
                            Layout.fillHeight: true
                            logParser: logParser
                            logViewerController: logViewerController
                            onClearSelectedRequested: logViewerChart.refreshBinChart()
                        }

                        // Right panel: chart + MAVLink inspector
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: qgcPal.windowPanel
                            radius: ScreenTools.defaultFontPixelWidth * 0.5

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: ScreenTools.defaultFontPixelWidth
                                spacing: ScreenTools.defaultFontPixelHeight * 0.5

                                LogViewerChart {
                                    id: logViewerChart
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    visible: isFirmwareLog
                                    logParser: logParser
                                    logViewerController: logViewerController
                                    xAxisShowLocalTime: _xAxisShowLocalTime

                                    onCursorMoved: (t) => {
                                        _mapTab._markerVisible = true
                                        _mapTab._markerCoord   = logParser.gpsCoordAt(t)
                                        _mapTab._sharedCursorT = t
                                        if (_altChart.visible) _altChart.setSharedCursor(t)
                                    }
                                    onZoomApplied: (minX, maxX) => {
                                        _mapTab._setSharedZoom(minX, maxX)
                                        if (_altChart.visible) _altChart.setSharedZoom(minX, maxX)
                                    }
                                }

                                Loader {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    active: logViewerController.sourceType === LogViewerController.TLog
                                    source: "qrc:/qml/QGroundControl/AnalyzeView/MAVLinkInspector/MAVLinkInspectorPage.qml"
                                }
                            }
                        }
                    }
                }

                QGCGroupBoxTab {
                    title: qsTr("Map")
                    buttonObjectName: "logViewer_mapTabButton"
                    Layout.fillHeight: true

                    Item {
                        id: _mapTab
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        // GPS path data
                        readonly property var _gpsPath: logParser.parseComplete ? logParser.gpsPath() : []
                        readonly property int _pathLen: (_gpsPath && _gpsPath.length) ? _gpsPath.length : 0
                        readonly property bool _hasPath: _pathLen >= 2
                        readonly property string _altFieldName: _hasPath ? logParser.gpsAltitudeFieldName() : ""
                        readonly property bool _hasAltField: _altFieldName.length > 0
                        readonly property bool _showAltChart: _hasAltField && _hasPath

                        // Shared cursor state (driven by altitude chart, displayed on map)
                        property bool _markerVisible: false
                        property var _markerCoord: ({})
                        readonly property bool _markerShown: _markerVisible && _hasPath && _markerCoord && _markerCoord.latitude !== undefined
                        readonly property var _markerCoordinate: _markerShown ? QtPositioning.coordinate(_markerCoord.latitude, _markerCoord.longitude)
                                                                              : QtPositioning.coordinate(0, 0)

                        // Collapsed state for the altitude chart, so the map can use the full tab height
                        property bool _altChartCollapsed: false

                        // Last shared cursor/zoom from either chart; reapplied when the altitude chart becomes visible.
                        property real _sharedCursorT: NaN
                        property real _sharedZoomMinX: NaN
                        property real _sharedZoomMaxX: NaN

                        // Smallest rectangle around the path
                        function _pathRegion() {
                            const p = _gpsPath
                            var minLat = p[0].latitude, maxLat = minLat
                            var minLon = p[0].longitude, maxLon = minLon
                            for (var i = 1; i < p.length; i++) {
                                var c = p[i]
                                if (c.latitude  < minLat) minLat = c.latitude
                                if (c.latitude  > maxLat) maxLat = c.latitude
                                if (c.longitude < minLon) minLon = c.longitude
                                if (c.longitude > maxLon) maxLon = c.longitude
                            }
                            return QtPositioning.rectangle(QtPositioning.coordinate(maxLat, minLon),
                                                           QtPositioning.coordinate(minLat, maxLon))
                        }

                        function _setSharedZoom(minX, maxX) {
                            _sharedZoomMinX = minX
                            _sharedZoomMaxX = maxX
                            // Mirror LogViewerBaseChart._applyZoomInternal, which recenters an out-of-range cursor
                            if (!isNaN(_sharedCursorT) && (_sharedCursorT < minX || _sharedCursorT > maxX)) {
                                _sharedCursorT = (minX + maxX) / 2
                                _markerCoord = logParser.gpsCoordAt(_sharedCursorT)
                            }
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 0

                            // ---- Map ----
                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true

                                // Engine choice is snapshotted at creation (the setting is qgcRebootRequired)
                                Loader {
                                    anchors.fill: parent
                                    Component.onCompleted: sourceComponent = QGroundControl.settingsManager.flyViewSettings.useGeoMapEngine.rawValue ? _geoMapComponent : _flightMapComponent
                                }

                                Component {
                                    id: _flightMapComponent

                                    FlightMap {
                                        id: _flightMap
                                        objectName: "logViewer_flightMap"
                                        mapName: "LogViewerMap"
                                        allowGCSLocationCenter: true

                                        function _fitPath() {
                                            if (_mapTab._hasPath) {
                                                setVisibleRegion(_mapTab._pathRegion())
                                            }
                                        }

                                        Connections {
                                            target: logParser
                                            function onParseCompleteChanged() {
                                                if (logParser.parseComplete) Qt.callLater(_flightMap._fitPath)
                                            }
                                        }

                                        MapPolyline {
                                            line.width: 3
                                            line.color: QGroundControl.globalPalette.colorRed
                                            path: _mapTab._gpsPath
                                        }

                                        // Position dot driven by altitude chart cursor
                                        MapQuickItem {
                                            objectName: "logViewer_positionDot"
                                            visible: _mapTab._markerShown
                                            coordinate: _mapTab._markerCoordinate
                                            anchorPoint: Qt.point(sourceItem.width / 2, sourceItem.height / 2)
                                            sourceItem: PositionDot { }
                                        }

                                        MapScale {
                                            anchors.margins: ScreenTools.defaultFontPixelWidth
                                            anchors.bottomMargin: ScreenTools.defaultFontPixelWidth + _flightMap.attributionHeight
                                            anchors.left: parent.left
                                            anchors.bottom: parent.bottom
                                            mapControl: _flightMap
                                        }
                                    }
                                }

                                Component {
                                    id: _geoMapComponent

                                    // Top-down and north-up like the QtLocation map: the log's path has no altitude to show in 3D
                                    GeoMapView {
                                        id: _geoMap

                                        // A hidden tab has no size to fit the path to: it is fitted once the map is shown
                                        property bool _fitPending: false

                                        function _fitPath() {
                                            _fitPending = true
                                            _fitPendingPath()
                                        }

                                        function _fitPendingPath() {
                                            if (!_fitPending || !visible || width <= 0 || height <= 0) {
                                                return
                                            }
                                            _fitPending = false
                                            if (_mapTab._hasPath) {
                                                setVisibleRegion(_mapTab._pathRegion())
                                            }
                                        }

                                        objectName: "logViewer_geoMap"
                                        sharedView: false
                                        terrain: false
                                        allowRotation: false
                                        allowGCSLocationCenter: true
                                        followOffscreenVehicle: false

                                        onVisibleChanged: Qt.callLater(_fitPendingPath)
                                        onWidthChanged: Qt.callLater(_fitPendingPath)
                                        onHeightChanged: Qt.callLater(_fitPendingPath)

                                        GeoMapTrack {
                                            objectName: "logViewer_geoMapTrack"
                                            scene: _geoMap.scene
                                            surfaceModel: _geoMap.surfaceModel
                                            lineColor: QGroundControl.globalPalette.colorRed
                                            path: _mapTab._gpsPath.map((point) => QtPositioning.coordinate(point.latitude, point.longitude))
                                        }

                                        // Position dot driven by altitude chart cursor
                                        GeoMapItem {
                                            objectName: "logViewer_positionDot"
                                            scene: _geoMap.scene
                                            surfaceModel: _geoMap.surfaceModel
                                            visible: _mapTab._markerShown
                                            coordinate: _mapTab._markerCoordinate
                                            width: _geoMapPositionDot.width
                                            height: _geoMapPositionDot.height
                                            anchorPoint: Qt.point(width / 2, height / 2)

                                            PositionDot {
                                                id: _geoMapPositionDot
                                            }
                                        }

                                        MapScale {
                                            anchors.margins: ScreenTools.defaultFontPixelWidth
                                            anchors.bottomMargin: ScreenTools.defaultFontPixelWidth + _geoMap.attributionHeight
                                            anchors.left: parent.left
                                            anchors.bottom: parent.bottom
                                            mapControl: _geoMap
                                        }

                                        Connections {
                                            target: logParser
                                            function onParseCompleteChanged() {
                                                if (logParser.parseComplete) Qt.callLater(_geoMap._fitPath)
                                            }
                                        }

                                        Component.onCompleted: {
                                            center = QGroundControl.flightMapPosition
                                            zoomLevel = QGroundControl.flightMapZoom
                                        }
                                    }
                                }

                                QGCLabel {
                                    anchors.centerIn: parent
                                    visible: logParser.parseComplete && !_mapTab._hasPath
                                    text: qsTr("No GPS data found in this log")
                                    font.italic: true
                                }

                                QGCLabel {
                                    anchors.centerIn: parent
                                    visible: !logParser.parseComplete
                                    text: qsTr("Load a log file to view the flight path")
                                    font.italic: true
                                }

                                QGCButton {
                                    anchors.margins: ScreenTools.defaultFontPixelWidth
                                    anchors.right: parent.right
                                    anchors.bottom: parent.bottom
                                    scale: 0.75
                                    transformOrigin: Item.BottomRight
                                    visible: _mapTab._showAltChart
                                    iconSource: _mapTab._altChartCollapsed ? "/res/chevron-double-up.svg" : "/res/chevron-double-down.svg"
                                    Accessible.name: _mapTab._altChartCollapsed ? qsTr("Expand altitude chart") : qsTr("Collapse altitude chart")
                                    ToolTip.text: Accessible.name
                                    ToolTip.visible: hovered
                                    onClicked: _mapTab._altChartCollapsed = !_mapTab._altChartCollapsed
                                }
                            }

                            // ---- Altitude chart ----
                            LogViewerAltChart {
                                id: _altChart
                                Layout.fillWidth: true
                                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 14
                                visible: _mapTab._showAltChart && !_mapTab._altChartCollapsed
                                logParser: logParser
                                altFieldName: _mapTab._altFieldName
                                xAxisShowLocalTime: _xAxisShowLocalTime

                                onMarkerChanged: (t) => {
                                    _mapTab._markerVisible = true
                                    _mapTab._markerCoord   = logParser.gpsCoordAt(t)
                                    _mapTab._sharedCursorT = t
                                    logViewerChart.setSharedCursor(t)
                                }
                                onMarkerCleared: {
                                    _mapTab._markerVisible = false
                                }
                                onZoomApplied: (minX, maxX) => {
                                    _mapTab._setSharedZoom(minX, maxX)
                                    logViewerChart.setSharedZoom(minX, maxX)
                                }
                            }

                            // Catch up on cursor/zoom changes that occurred while this chart was hidden.
                            Connections {
                                target: _altChart
                                function onVisibleChanged() {
                                    if (!_altChart.visible) {
                                        return
                                    }
                                    if (!isNaN(_mapTab._sharedZoomMinX)) {
                                        _altChart.setSharedZoom(_mapTab._sharedZoomMinX, _mapTab._sharedZoomMaxX)
                                    }
                                    if (!isNaN(_mapTab._sharedCursorT)) {
                                        _altChart.setSharedCursor(_mapTab._sharedCursorT)
                                    }
                                }
                            }
                        }
                    }
                }

                QGCGroupBoxTab {
                    title: qsTr("Parameters")
                    Layout.fillHeight: true

                    LogViewerParametersTab {
                        id: _parametersTab
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        logParser: logParser
                    }
                }

                QGCGroupBoxTab {
                    title: qsTr("Messages")
                    Layout.fillHeight: true

                    LogViewerMessagesTab {
                        id: _messagesTab
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        logParser: logParser
                    }
                }
            }

            QGCFileDialog {
                id: openDialog
                title: qsTr("Select log file")
                folder: QGroundControl.settingsManager.appSettings.logSavePath
                selectFolder: false

                onAcceptedForLoad: (file) => {
                    const fileLower = file.toLowerCase()
                    if (fileLower.endsWith(".tlog")) {
                        if (logViewerController.hasLoadedLog) {
                            clearLoadedLogState(true)
                        }
                        const replayLink = QGroundControl.linkManager.startLogReplay(file)
                        if (!replayLink) {
                            QGroundControl.showMessageDialog(
                                logViewerPage,
                                qsTr("Log Viewer"),
                                qsTr("Failed to start telemetry replay for the selected .tlog file.")
                            )
                            close()
                            return
                        }
                        replayController.link = replayLink
                        logViewerController.openTLog(file)
                    } else {
                        loadBinFile(file)
                    }
                    close()
                }
            }
        }
    }
}
