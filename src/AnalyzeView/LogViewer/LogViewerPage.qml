import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap
import QGroundControl.LogViewer

AnalyzePage {
    id: logViewerPage
    pageComponent: pageComponent
    pageDescription: qsTr("Open and inspect DataFlash (.bin), PX4 ULog (.ulg), and telemetry (.tlog) logs in a unified workflow.")

    Component {
        id: pageComponent

        ColumnLayout {
            width: availableWidth
            height: availableHeight
            spacing: ScreenTools.defaultFontPixelHeight

            property bool binLoading: false
            property string pendingBinFile: ""
            property string fieldSearchText: ""
            property string parameterSearchText: ""
            property bool showOnlyChangedParameters: true
            property var filteredFieldRows: []
            property var filteredParameters: []

            readonly property bool isFirmwareLog: logViewerController.sourceType === LogViewerController.Bin
                                             || logViewerController.sourceType === LogViewerController.ULog

            function rebuildGroupedFields() {
                logViewerController.setPlottableFields(logParser.plottableFields)
                applyFieldFilter()
            }

            function applyFieldFilter() {
                const query = String(fieldSearchText).trim().toLowerCase()
                if (query.length === 0) {
                    filteredFieldRows = logViewerController.fieldRows
                    return
                }

                const groupedMap = {}
                const fields = logParser.plottableFields
                for (let i = 0; i < fields.length; i++) {
                    const fullName = String(fields[i])
                    const splitIndex = fullName.indexOf(".")
                    const groupName = splitIndex > 0 ? fullName.substring(0, splitIndex) : qsTr("Other")
                    const shortName = splitIndex > 0 ? fullName.substring(splitIndex + 1) : fullName
                    const haystack = (fullName + " " + groupName + " " + shortName).toLowerCase()
                    if (haystack.indexOf(query) === -1) {
                        continue
                    }

                    if (!groupedMap[groupName]) {
                        groupedMap[groupName] = []
                    }
                    groupedMap[groupName].push({ fullName: fullName, shortName: shortName })
                }

                const groups = Object.keys(groupedMap).sort()
                const rows = []
                for (let g = 0; g < groups.length; g++) {
                    const groupName = groups[g]
                    rows.push({ rowType: "group", group: groupName })
                    groupedMap[groupName].sort((a, b) => String(a.shortName).localeCompare(String(b.shortName)))
                    for (let s = 0; s < groupedMap[groupName].length; s++) {
                        rows.push({
                            rowType: "field",
                            group: groupName,
                            fullName: groupedMap[groupName][s].fullName,
                            shortName: groupedMap[groupName][s].shortName
                        })
                    }
                }
                filteredFieldRows = rows
            }

            function applyParameterFilter() {
                const query = String(parameterSearchText).trim().toLowerCase()
                const onlyChanged = showOnlyChangedParameters

                const output = []
                for (let i = 0; i < logParser.parameters.length; i++) {
                    const item = logParser.parameters[i]
                    // "Show only changed" filter: skip parameters that equal their system default
                    if (onlyChanged && item.hasDefault && item.isDefault) {
                        continue
                    }
                    if (query.length > 0) {
                        const name = String(item.name)
                        const desc = item.shortDescription ? String(item.shortDescription) : ""
                        const value = String(item.value)
                        if ((name + " " + value + " " + desc).toLowerCase().indexOf(query) === -1) {
                            continue
                        }
                    }
                    output.push(item)
                }
                filteredParameters = output
            }

            function isGroupExpanded(groupName) {
                return logViewerController.isGroupExpanded(groupName)
            }

            function toggleGroupExpanded(groupName) {
                if (String(fieldSearchText).trim().length > 0) {
                    return
                }
                logViewerController.toggleGroupExpanded(groupName)
                applyFieldFilter()
            }

            function isFieldSelected(fieldName) {
                return logViewerController.isFieldSelected(fieldName)
            }

            function clearLoadedLogState(clearControllerState) {
                replayController.isPlaying = false
                replayController.link = null
                logParser.clear()
                logViewerController.setPlottableFields([])
                logViewerController.clearSelection()
                filteredFieldRows = []
                filteredParameters = []
                logViewerChart.clearMarker()
                logViewerChart.refreshBinChart()
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
                binLoading = true
                parseStartTimer.start()
            }

            function _executePendingBinParse() {
                if (!pendingBinFile || pendingBinFile.length === 0) {
                    binLoading = false
                    return
                }

                const file = pendingBinFile
                logParser.parseFileAsync(file)
            }

            LogViewerController {
                id: logViewerController
            }

            LogFileParser {
                id: logParser
            }

            Connections {
                target: logViewerController
                function onFieldRowsChanged() {
                    const savedY = fieldsListView.contentY
                    applyFieldFilter()
                    Qt.callLater(() => { fieldsListView.contentY = savedY })
                }
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
                        binLoading = false
                        pendingBinFile = ""
                        return
                    }

                    rebuildGroupedFields()
                    applyParameterFilter()
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
                    binLoading = false
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
                    text:    qsTr("Open .bin")
                    visible: QGroundControl.hasAPMSupport
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
                    text: logViewerController.hasLoadedLog ? logViewerController.currentLogPath : qsTr("No log selected")
                }
            }

            QGCTabBar {
                id: mainTabBar
                Layout.fillWidth: true

                QGCTabButton { text: qsTr("Charting") }
                QGCTabButton { text: qsTr("Map") }
                QGCTabButton { text: qsTr("Parameters") }
                QGCTabButton { text: qsTr("Messages") }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: mainTabBar.currentIndex

                // ---- Tab 0: Charting ----
                RowLayout {
                    spacing: ScreenTools.defaultFontPixelWidth

                    // Left panel: stats + replay controls + fields list
                    Rectangle {
                        Layout.preferredWidth: availableWidth * 0.25
                        Layout.fillHeight: true
                        color: qgcPal.windowShade
                        radius: ScreenTools.defaultFontPixelWidth * 0.5

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: ScreenTools.defaultFontPixelWidth
                            spacing: ScreenTools.defaultFontPixelHeight * 0.5

                            QGCLabel {
                                visible: isFirmwareLog
                                text: qsTr("Fields: %1  Parameters: %2  Events: %3")
                                      .arg(logParser.plottableFields.length)
                                      .arg(logParser.parameters.length)
                                      .arg(logParser.events.length)
                            }

                            QGCLabel {
                                visible: isFirmwareLog
                                text: qsTr("Detected vehicle type: %1")
                                      .arg(logParser.detectedVehicleType.length > 0
                                           ? logParser.detectedVehicleType
                                           : qsTr("Unknown"))
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
                                        ListElement { text: "0.1x"; value: 0.1 }
                                        ListElement { text: "0.25x"; value: 0.25 }
                                        ListElement { text: "0.5x"; value: 0.5 }
                                        ListElement { text: "1x"; value: 1.0 }
                                        ListElement { text: "2x"; value: 2.0 }
                                        ListElement { text: "5x"; value: 5.0 }
                                        ListElement { text: "10x"; value: 10.0 }
                                    }

                                    onActivated: (index) => replayController.playbackSpeed = model.get(index).value
                                }

                                QGCLabel { text: replayController.playheadTime }

                                Slider {
                                    id: replaySlider
                                    Layout.fillWidth: true
                                    from: 0
                                    to: 100

                                    property bool _internalUpdate: false

                                    Connections {
                                        target: replayController
                                        function onPercentCompleteChanged(percentComplete) {
                                            replaySlider._internalUpdate = true
                                            replaySlider.value = percentComplete
                                            replaySlider._internalUpdate = false
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

                            RowLayout {
                                Layout.fillWidth: true
                                visible: isFirmwareLog
                                spacing: ScreenTools.defaultFontPixelWidth * 0.5

                                QGCLabel {
                                    text: qsTr("Fields")
                                    font.bold: true
                                }

                                QGCTextField {
                                    id: fieldSearchField
                                    Layout.fillWidth: true
                                    textColor: qgcPal.textFieldText
                                    placeholderTextColor: Qt.rgba(qgcPal.textFieldText.r, qgcPal.textFieldText.g, qgcPal.textFieldText.b, 0.7)
                                    placeholderText: qsTr("Search fields")
                                    onTextChanged: {
                                        fieldSearchText = text
                                        if (text.trim().length === 0) {
                                            fieldSearchTimer.stop()
                                            applyFieldFilter()
                                        } else {
                                            fieldSearchTimer.restart()
                                        }
                                    }
                                    onAccepted: {
                                        fieldSearchText = text
                                        fieldSearchTimer.stop()
                                        applyFieldFilter()
                                    }
                                }

                                QGCButton {
                                    text: qsTr("Clear Selected")
                                    horizontalAlignment: Text.AlignHCenter
                                    Layout.preferredHeight: fieldSearchField.implicitHeight
                                    Layout.minimumHeight: fieldSearchField.implicitHeight
                                    topPadding: 0
                                    bottomPadding: 0
                                    enabled: logViewerController.selectedFields.length > 0
                                    onClicked: {
                                        logViewerController.clearSelection()
                                        applyFieldFilter()
                                        logViewerChart.refreshBinChart()
                                    }
                                }
                            }

                            ScrollView {
                                id: fieldsScroll
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                visible: isFirmwareLog
                                clip: true

                                ListView {
                                    id: fieldsListView
                                    anchors.fill: parent
                                    model: filteredFieldRows
                                    spacing: 0
                                    clip: true
                                    ScrollBar.vertical: ScrollBar { }

                                    delegate: Item {
                                        width: fieldsListView.width
                                        height: (modelData.rowType === "group")
                                                ? (groupRect.implicitHeight + (ScreenTools.defaultFontPixelHeight * 0.1))
                                                : (fieldRow.implicitHeight + (ScreenTools.defaultFontPixelHeight * 0.1))

                                        Item {
                                            id: groupRect
                                            visible: modelData.rowType === "group"
                                            width: parent.width
                                            implicitWidth: groupLayout.implicitWidth
                                            implicitHeight: groupLayout.implicitHeight

                                            RowLayout {
                                                id: groupLayout
                                                spacing: ScreenTools.defaultFontPixelWidth / 2

                                                QGCColoredImage {
                                                    Layout.preferredWidth: groupLabel.height * 0.5
                                                    Layout.preferredHeight: groupLabel.height * 0.5
                                                    source: "/qmlimages/arrow-down.png"
                                                    color: qgcPal.text
                                                    fillMode: Image.PreserveAspectFit
                                                    rotation: (String(fieldSearchText).trim().length > 0 || isGroupExpanded(modelData.group)) ? 0 : -90
                                                }

                                                QGCLabel {
                                                    id: groupLabel
                                                    text: modelData.group;
                                                    font.bold: true
                                                }
                                            }

                                            MouseArea {
                                                anchors.fill: parent
                                                onClicked: toggleGroupExpanded(modelData.group)
                                            }
                                        }

                                        Item {
                                            id: fieldRow
                                            visible: modelData.rowType === "field"
                                            width: parent.width
                                            implicitHeight: fieldSlider.implicitHeight

                                            QGCCheckBoxSlider {
                                                id: fieldSlider
                                                width: parent.width - ScreenTools.defaultFontPixelWidth * 1.5
                                                checked: logViewerController.selectedFields.indexOf(modelData.fullName) !== -1
                                                text: modelData.shortName ? String(modelData.shortName) : ""
                                                onClicked: logViewerController.setFieldSelected(modelData.fullName, checked)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Right panel: chart + MAVLink inspector
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: qgcPal.windowShadeDark
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

                                onCursorMoved: (t) => {
                                    _mapTab._markerVisible = true
                                    _mapTab._markerCoord   = logParser.gpsCoordAt(t)
                                    if (_altChart.visible) _altChart.setSharedCursor(t)
                                }
                                onZoomApplied: (minX, maxX) => {
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

                // ---- Tab 1: Map ----
                Item {
                    id: _mapTab
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    // GPS path data
                    readonly property var    _gpsPath:      logParser.parsed ? logParser.gpsPath() : []
                    readonly property int    _pathLen:      (_gpsPath && _gpsPath.length) ? _gpsPath.length : 0
                    readonly property bool   _hasPath:      _pathLen >= 2
                    readonly property string _altFieldName: _hasPath ? logParser.gpsAltitudeFieldName() : ""
                    readonly property bool   _hasAltField:  _altFieldName.length > 0

                    // Shared cursor state (driven by altitude chart, displayed on map)
                    property bool _markerVisible: false
                    property var  _markerCoord:   ({})

                    ColumnLayout {
                        anchors.fill: parent
                        spacing:      0

                        // ---- Map ----
                        Item {
                            Layout.fillWidth:  true
                            Layout.fillHeight: true

                            FlightMap {
                                id: _flightMap
                                anchors.fill:          parent
                                mapName:               "LogViewerMap"
                                allowGCSLocationCenter: true

                                readonly property var _path: _mapTab._gpsPath

                                function _fitPath() {
                                    const p = _mapTab._gpsPath
                                    if (!_mapTab._hasPath) return
                                    var minLat = p[0].latitude, maxLat = minLat
                                    var minLon = p[0].longitude, maxLon = minLon
                                    for (var i = 1; i < p.length; i++) {
                                        var c = p[i]
                                        if (c.latitude  < minLat) minLat = c.latitude
                                        if (c.latitude  > maxLat) maxLat = c.latitude
                                        if (c.longitude < minLon) minLon = c.longitude
                                        if (c.longitude > maxLon) maxLon = c.longitude
                                    }
                                    setVisibleRegion(QtPositioning.rectangle(
                                        QtPositioning.coordinate(maxLat, minLon),
                                        QtPositioning.coordinate(minLat, maxLon)))
                                }

                                Connections {
                                    target: logParser
                                    function onParsedChanged() {
                                        if (logParser.parsed) Qt.callLater(_flightMap._fitPath)
                                    }
                                }

                                MapPolyline {
                                    line.width: 3
                                    line.color: QGroundControl.globalPalette.colorRed
                                    path:       _flightMap._path
                                }

                                // Position dot driven by altitude chart cursor
                                MapQuickItem {
                                    readonly property var _coord: _mapTab._markerCoord
                                    visible:      _mapTab._markerVisible && _mapTab._hasPath
                                                  && _coord && _coord.latitude !== undefined
                                    coordinate:   (_coord && _coord.latitude !== undefined)
                                                  ? QtPositioning.coordinate(_coord.latitude, _coord.longitude)
                                                  : QtPositioning.coordinate(0, 0)
                                    anchorPoint:  Qt.point(_posDot.width / 2, _posDot.height / 2)
                                    sourceItem: Rectangle {
                                        id:           _posDot
                                        width:        ScreenTools.defaultFontPixelHeight * 1.2
                                        height:       width
                                        radius:       width / 2
                                        color:        QGroundControl.globalPalette.colorYellow
                                        border.color: "white"
                                        border.width: 2
                                    }
                                }

                                MapScale {
                                    anchors.margins: ScreenTools.defaultFontPixelWidth
                                    anchors.left:    parent.left
                                    anchors.bottom:  parent.bottom
                                    mapControl:      _flightMap
                                }
                            }

                            QGCLabel {
                                anchors.centerIn: parent
                                visible:          logParser.parsed && !_mapTab._hasPath
                                text:             qsTr("No GPS data found in this log")
                                font.italic:      true
                            }

                            QGCLabel {
                                anchors.centerIn: parent
                                visible:          !logParser.parsed
                                text:             qsTr("Load a log file to view the flight path")
                                font.italic:      true
                            }
                        }

                        // ---- Altitude chart ----
                        LogViewerAltChart {
                            id:                    _altChart
                            Layout.fillWidth:      true
                            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 14
                            visible:               _mapTab._hasAltField && _mapTab._hasPath
                            logParser:             logParser
                            altFieldName:          _mapTab._altFieldName

                            onMarkerChanged: (t) => {
                                _mapTab._markerVisible = true
                                _mapTab._markerCoord   = logParser.gpsCoordAt(t)
                                logViewerChart.setSharedCursor(t)
                            }
                            onMarkerCleared: {
                                _mapTab._markerVisible = false
                            }
                            onZoomApplied: (minX, maxX) => {
                                logViewerChart.setSharedZoom(minX, maxX)
                            }
                        }
                    }
                }

                // ---- Tab 2: Parameters ----
                ColumnLayout {
                    spacing: ScreenTools.defaultFontPixelHeight * 0.5

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: ScreenTools.defaultFontPixelWidth

                        QGCTextField {
                            id: parameterSearchField
                            Layout.fillWidth: true
                            textColor: qgcPal.textFieldText
                            placeholderTextColor: Qt.rgba(qgcPal.textFieldText.r, qgcPal.textFieldText.g, qgcPal.textFieldText.b, 0.7)
                            placeholderText: qsTr("Search parameters")

                            onTextChanged: {
                                parameterSearchText = text
                                if (text.trim().length === 0) {
                                    parameterSearchTimer.stop()
                                    applyParameterFilter()
                                } else {
                                    parameterSearchTimer.restart()
                                }
                            }

                            onAccepted: {
                                parameterSearchText = text
                                parameterSearchTimer.stop()
                                applyParameterFilter()
                            }
                        }

                        QGCCheckBoxSlider {
                            id: showOnlyChangedCheckBox
                            text: qsTr("Changed only")
                            checked: showOnlyChangedParameters
                            onToggled: {
                                showOnlyChangedParameters = checked
                                applyParameterFilter()
                            }
                        }
                    }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true

                        ListView {
                            id: parametersListView
                            anchors.fill: parent
                            model: filteredParameters
                            spacing: ScreenTools.defaultFontPixelHeight * 0.1
                            clip: true
                            ScrollBar.vertical: ScrollBar { }

                            delegate: Rectangle {
                                width: ListView.view.width
                                height: _paramRow.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.4
                                color: index % 2 === 0 ? qgcPal.windowShade : qgcPal.windowShadeDark
                                radius: 2

                                // Format value: use metadata decimalPlaces when available,
                                // fall back to isFloat heuristic. Show enum label when applicable.
                                readonly property string _formattedValue: {
                                    const v = modelData.value
                                    if (v === undefined || v === null) return qsTr("N/A")
                                    const numV = Number(v)
                                    // Enum: find matching label
                                    const eStrs = modelData.enumStrings
                                    const eVals = modelData.enumValues
                                    if (eStrs && eStrs.length > 0) {
                                        for (let ei = 0; ei < eVals.length; ei++) {
                                            if (Number(eVals[ei]) === numV) {
                                                return eStrs[ei] + " (" + Math.round(numV) + ")"
                                            }
                                        }
                                    }
                                    // Numeric: metadata decimalPlaces wins
                                    const dp = (modelData.decimalPlaces !== undefined) ? modelData.decimalPlaces : -1
                                    let formatted
                                    if (dp >= 0) {
                                        formatted = numV.toFixed(dp)
                                    } else if (modelData.isFloat) {
                                        formatted = numV.toFixed(6)
                                    } else {
                                        formatted = String(Math.round(numV))
                                    }
                                    const units = (modelData.units && modelData.units.length > 0) ? (" " + modelData.units) : ""
                                    return formatted + units
                                }

                                readonly property string _defaultText: {
                                    if (!modelData.hasDefault) return ""
                                    const d = modelData.defaultValue
                                    if (d === undefined || d === null) return ""
                                    const numD = Number(d)
                                    const dp = (modelData.decimalPlaces !== undefined) ? modelData.decimalPlaces : -1
                                    let formatted
                                    if (dp >= 0) {
                                        formatted = numD.toFixed(dp)
                                    } else if (modelData.isFloat) {
                                        formatted = numD.toFixed(6)
                                    } else {
                                        formatted = String(Math.round(numD))
                                    }
                                    return qsTr(" (default: %1)").arg(formatted)
                                }

                                RowLayout {
                                    id: _paramRow
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.margins: ScreenTools.defaultFontPixelWidth * 0.5
                                    spacing: ScreenTools.defaultFontPixelWidth * 0.5

                                    // Changed-from-default indicator dot
                                    Rectangle {
                                        visible: modelData.hasDefault && !modelData.isDefault
                                        width: ScreenTools.defaultFontPixelHeight * 0.5
                                        height: width
                                        radius: width / 2
                                        color: qgcPal.colorOrange
                                        Layout.alignment: Qt.AlignVCenter
                                    }

                                    // Spacer to keep alignment when dot is hidden
                                    Item {
                                        visible: !(modelData.hasDefault && !modelData.isDefault)
                                        width: ScreenTools.defaultFontPixelHeight * 0.5
                                        height: width
                                    }

                                    QGCLabel {
                                        text: modelData.name
                                        font.bold: modelData.hasDefault && !modelData.isDefault
                                        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 22
                                        elide: Text.ElideRight
                                    }

                                    QGCLabel {
                                        text: _formattedValue
                                        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 14
                                        horizontalAlignment: Text.AlignRight
                                    }

                                    QGCLabel {
                                        text: _defaultText
                                        color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.6)
                                        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 16
                                        elide: Text.ElideRight
                                    }

                                    QGCLabel {
                                        text: modelData.shortDescription ? String(modelData.shortDescription) : ""
                                        color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.6)
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                        }
                    }
                }

                // ---- Tab 3: Messages ----
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    ListView {
                        id: messagesListView
                        anchors.fill: parent
                        model: logParser.messages
                        spacing: ScreenTools.defaultFontPixelHeight * 0.2
                        clip: true
                        ScrollBar.vertical: ScrollBar { }

                        delegate: Rectangle {
                            width: ListView.view.width
                            height: _msgRow.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.4
                            color: index % 2 === 0 ? qgcPal.windowShade : qgcPal.windowShadeDark
                            radius: 2

                            RowLayout {
                                id: _msgRow
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.margins: ScreenTools.defaultFontPixelWidth * 0.5
                                spacing: ScreenTools.defaultFontPixelWidth * 0.5

                                QGCLabel {
                                    readonly property double _t: Number(modelData.time)
                                    text: (isNaN(_t) || _t < 0) ? "" : _t.toFixed(3) + "s"
                                    color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.6)
                                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 10
                                    horizontalAlignment: Text.AlignRight
                                }

                                QGCLabel {
                                    text: String(modelData.text)
                                    Layout.fillWidth: true
                                    wrapMode: Text.WordWrap
                                    maximumLineCount: 3
                                }
                            }
                        }
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

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: binLoading
                color: Qt.rgba(0, 0, 0, 0.4)
                z: 5000

                Column {
                    anchors.centerIn: parent
                    spacing: ScreenTools.defaultFontPixelHeight * 0.5

                    BusyIndicator {
                        anchors.horizontalCenter: parent.horizontalCenter
                        running: binLoading
                    }

                    QGCLabel {
                        text: qsTr("Parsing log file...")
                    }
                }
            }

            Timer {
                id: parseStartTimer
                interval: 50
                repeat: false
                onTriggered: _executePendingBinParse()
            }

            Timer {
                id: fieldSearchTimer
                interval: 250
                repeat: false
                onTriggered: applyFieldFilter()
            }

            Timer {
                id: parameterSearchTimer
                interval: 250
                repeat: false
                onTriggered: applyParameterFilter()
            }
        }
    }
}
