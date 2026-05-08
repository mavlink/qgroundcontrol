import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtGraphs

import QGroundControl
import QGroundControl.Controls

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
            property bool cursorVisible: false
            property real cursorPixelX: 0
            property real cursorXValue: 0
            property real cursorPopupY: 0
            property var cursorRows: []
            property var cursorEventRows: []
            property string cursorModeName: ""
            property string fieldSearchText: ""
            property string parameterSearchText: ""
            property var filteredFieldRows: []
            property var filteredParameters: []
            property real fullMinX: 0
            property real fullMaxX: 1
            property real zoomMinX: 0
            property real zoomMaxX: 1

            // Maps fieldName → LineSeries and fieldName → {min, max} for incremental add/remove.
            property var _seriesByField: ({})
            property var _fieldYRange: ({})
            // Maps eventType → ScatterSeries; rebuilt whenever field selection changes,
            // because Y pin position depends on binYAxis.max which is recomputed at that time.
            property var _eventSeriesByType: ({})

            // Cap decimation to keep QtGraphs rendering below ~16ms per frame.
            // Empirically, >~6000 points per series causes visible frame drops on mid-range hardware.
            property int maxChartPointsPerField: 6000

            readonly property bool isFirmwareLog: logViewerController.sourceType === LogViewerController.Bin
                                             || logViewerController.sourceType === LogViewerController.ULog

            // Visually distinct categorical palette for selected-field series.
            // Position-based picks avoid hash collisions producing similar colors.
            readonly property var _fieldChartColors: [
                "#1E88E5", // blue
                "#E53935", // red
                "#43A047", // green
                "#FB8C00", // orange
                "#8E24AA", // purple
                "#00ACC1", // cyan
                "#FDD835", // yellow
                "#D81B60", // pink
                "#6D4C41", // brown
                "#546E7A", // blue grey
                "#3949AB", // indigo
                "#00897B", // teal
            ]

            Component {
                id: lineSeriesComponent

                LineSeries { }
            }

            Component {
                id: scatterSeriesComponent

                ScatterSeries { }
            }

            function eventColor(eventType) {
                return logViewerController.eventColor(eventType)
            }

            function eventTypeLabel(eventType) {
                if (eventType === "mode") {
                    return qsTr("Mode")
                }
                if (eventType === "event") {
                    return qsTr("Event")
                }
                if (eventType === "error") {
                    return qsTr("Error")
                }
                if (eventType === "warning") {
                    return qsTr("Warning")
                }
                return eventType
            }

            function modeColor(modeName) {
                return logViewerController.modeColor(String(modeName))
            }

            function modeLegendEntries() {
                return logViewerController.modeLegendEntries(logParser.modeSegments)
            }

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
                if (query.length === 0) {
                    filteredParameters = logParser.parameters
                    return
                }

                const output = []
                for (let i = 0; i < logParser.parameters.length; i++) {
                    const item = logParser.parameters[i]
                    const name = String(item.name)
                    const value = String(item.value)
                    if ((name + " " + value).toLowerCase().indexOf(query) !== -1) {
                        output.push(item)
                    }
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

            function fieldColor(fieldName) {
                const idx = logViewerController.selectedFields.indexOf(fieldName)
                if (idx >= 0) {
                    return _fieldChartColors[idx % _fieldChartColors.length]
                }
                return logViewerController.fieldColor(fieldName)
            }

            function applyZoomRange(minX, maxX) {
                if (maxX <= minX) {
                    return
                }
                zoomMinX = minX
                zoomMaxX = maxX
                binXAxis.min = zoomMinX
                binXAxis.max = zoomMaxX
            }

            function resetZoom() {
                applyZoomRange(fullMinX, fullMaxX)
            }

            function _pixelToAxisX(pixelX) {
                const plotX = binChart.plotArea.x
                const plotW = binChart.plotArea.width
                if (plotW <= 0 || binXAxis.max <= binXAxis.min) {
                    return binXAxis.min
                }

                const clampedPixel = Math.max(plotX, Math.min(plotX + plotW, pixelX))
                const ratio = (clampedPixel - plotX) / plotW
                return binXAxis.min + (ratio * (binXAxis.max - binXAxis.min))
            }

            function updateCursorInfo(pixelX, pixelY, width, height) {
                const selectedFields = logViewerController.selectedFields
                if (selectedFields.length === 0 || width <= 0 || height <= 0) {
                    cursorVisible = false
                    return
                }

                cursorVisible = true
                cursorPixelX = Math.max(binChart.plotArea.x, Math.min(binChart.plotArea.x + binChart.plotArea.width, pixelX))
                cursorXValue = _pixelToAxisX(cursorPixelX)
                cursorPopupY = Math.max(0, Math.min(height - (ScreenTools.defaultFontPixelHeight * 4), pixelY))
                cursorModeName = logParser.modeAt(cursorXValue)

                const rows = []
                for (let i = 0; i < selectedFields.length; i++) {
                    const field = selectedFields[i]
                    const value = logParser.fieldValueAt(field, cursorXValue)
                    if (isNaN(value)) {
                        continue
                    }
                    rows.push({
                        name: field,
                        color: fieldColor(field),
                        value: value
                    })
                }
                cursorRows = rows

                const threshold = Math.max(0.05, (binXAxis.max - binXAxis.min) / 200.0)
                const nearbyEvents = logParser.eventsNear(cursorXValue, threshold)
                const events = []
                for (let i = 0; i < nearbyEvents.length; i++) {
                    events.push({
                        color: eventColor(nearbyEvents[i].type),
                        text: nearbyEvents[i].description
                    })
                }
                cursorEventRows = events
            }

            function clearLoadedLogState(clearControllerState) {
                replayController.isPlaying = false
                replayController.link = null
                logParser.clear()
                logViewerController.setPlottableFields([])
                logViewerController.clearSelection()
                cursorEventRows = []
                filteredFieldRows = []
                filteredParameters = []
                refreshBinChart()
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

            // Full reset: remove all series and reinitialise axes from the log time range.
            // Called on log load, on full clear (clearLoadedLogState), and when all selected fields
            // are cleared via the "Clear Selected" button. For incremental selection changes use
            // _syncSeriesWithSelection instead.
            function refreshBinChart() {
                while (binChart.seriesList.length > 0) {
                    binChart.removeSeries(binChart.seriesList[0])
                }
                _seriesByField = {}
                _fieldYRange = {}
                _eventSeriesByType = {}

                if (logParser.minTimestamp >= 0.0 && logParser.maxTimestamp > logParser.minTimestamp) {
                    fullMinX = logParser.minTimestamp
                    fullMaxX = logParser.maxTimestamp
                } else {
                    fullMinX = 0
                    fullMaxX = 1
                }
                zoomMinX = fullMinX
                zoomMaxX = fullMaxX
                binXAxis.min = zoomMinX
                binXAxis.max = zoomMaxX
                binYAxis.min = 0
                binYAxis.max = 1
            }

            // Incremental update: diff the current series map against the new selection,
            // remove series for deselected fields, add series for newly selected fields,
            // then recompute the Y axis and rebuild the event scatter series.
            function _syncSeriesWithSelection() {
                const newSelection = logViewerController.selectedFields

                // Build lookup of desired fields
                const desired = {}
                for (let i = 0; i < newSelection.length; i++) {
                    desired[String(newSelection[i])] = true
                }

                // Remove series for fields that are no longer selected
                const tracked = Object.keys(_seriesByField)
                for (let i = 0; i < tracked.length; i++) {
                    if (!desired[tracked[i]]) {
                        binChart.removeSeries(_seriesByField[tracked[i]])
                        delete _seriesByField[tracked[i]]
                        delete _fieldYRange[tracked[i]]
                    }
                }

                // Add a series for each newly selected field
                for (let i = 0; i < newSelection.length; i++) {
                    const fieldName = String(newSelection[i])
                    if (_seriesByField[fieldName]) {
                        continue
                    }

                    const points = logParser.fieldSamples(fieldName)
                    if (!points || points.length === 0) {
                        continue
                    }

                    const series = lineSeriesComponent.createObject(binChart, {
                        color: fieldColor(fieldName),
                        width: 2,
                        axisX: binXAxis,
                        axisY: binYAxis
                    })
                    binChart.addSeries(series)

                    let minY = Number.MAX_VALUE
                    let maxY = -Number.MAX_VALUE
                    const sampleStep = Math.max(1, Math.ceil(points.length / maxChartPointsPerField))
                    let appendedLastX = -Number.MAX_VALUE
                    for (let j = 0; j < points.length; j += sampleStep) {
                        series.append(points[j].x, points[j].y)
                        appendedLastX = points[j].x
                        if (points[j].y < minY) minY = points[j].y
                        if (points[j].y > maxY) maxY = points[j].y
                    }
                    if (sampleStep > 1) {
                        const last = points[points.length - 1]
                        if (last && last.x !== appendedLastX) {
                            series.append(last.x, last.y)
                        }
                    }

                    _seriesByField[fieldName] = series
                    _fieldYRange[fieldName] = { min: minY, max: maxY }
                }

                // Update colors for all tracked series (selection indices may have shifted)
                for (let i = 0; i < newSelection.length; i++) {
                    const fn = String(newSelection[i])
                    const s = _seriesByField[fn]
                    if (s) s.color = fieldColor(fn)
                }

                // Recompute Y axis from cached per-field ranges
                const allTracked = Object.keys(_seriesByField)
                let globalMinY = 0
                let globalMaxY = 1
                if (allTracked.length > 0) {
                    globalMinY = Number.MAX_VALUE
                    globalMaxY = -Number.MAX_VALUE
                    for (let i = 0; i < allTracked.length; i++) {
                        const r = _fieldYRange[allTracked[i]]
                        if (r) {
                            if (r.min < globalMinY) globalMinY = r.min
                            if (r.max > globalMaxY) globalMaxY = r.max
                        }
                    }
                    if (globalMinY === globalMaxY) globalMaxY = globalMinY + 1
                }
                binYAxis.min = globalMinY
                binYAxis.max = globalMaxY

                // Rebuild event scatter series (Y position is pinned to current binYAxis.max)
                const existingTypes = Object.keys(_eventSeriesByType)
                for (let i = 0; i < existingTypes.length; i++) {
                    binChart.removeSeries(_eventSeriesByType[existingTypes[i]])
                }
                _eventSeriesByType = {}
                const eventList = logParser.events
                for (let e = 0; e < eventList.length; e++) {
                    const ev = eventList[e]
                    if (ev.time < binXAxis.min || ev.time > binXAxis.max) {
                        continue
                    }
                    if (!_eventSeriesByType[ev.type]) {
                        const eventSeries = scatterSeriesComponent.createObject(binChart, {
                            color: eventColor(ev.type),
                            axisX: binXAxis,
                            axisY: binYAxis
                        })
                        binChart.addSeries(eventSeries)
                        _eventSeriesByType[ev.type] = eventSeries
                    }
                    _eventSeriesByType[ev.type].append(ev.time, binYAxis.max)
                }
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
                    applyFieldFilter()
                }
                function onSelectedFieldsChanged() {
                    Qt.callLater(_syncSeriesWithSelection)
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
                    fullMinX = 0
                    fullMaxX = 1
                    zoomMinX = 0
                    zoomMaxX = 1
                    refreshBinChart()

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
                    text: qsTr("Open .bin")
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
                Layout.fillHeight: true
                spacing: ScreenTools.defaultFontPixelWidth

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
                            text: qsTr("Messages / Parameters")
                            font.bold: true
                        }

                        QGCLabel {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            maximumLineCount: 3
                            text: (isFirmwareLog)
                                  ? qsTr("DataFlash message and parameter browser will appear here.")
                                  : qsTr("For telemetry replay, MAVLink message and field selection is available through the Inspector integration.")
                        }

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
                            visible: isFirmwareLog
                            spacing: ScreenTools.defaultFontPixelWidth * 0.5

                            QGCLabel {
                                text: qsTr("Fields (click to plot)")
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
                                    refreshBinChart()
                                }
                            }
                        }

                        ScrollView {
                            id: fieldsScroll
                            Layout.fillWidth: true
                            Layout.preferredHeight: parent.height * 0.35
                            visible: isFirmwareLog
                            clip: true

                            ListView {
                                id: fieldsListView
                                anchors.fill: parent
                                model: filteredFieldRows
                                spacing: ScreenTools.defaultFontPixelHeight * 0.15
                                clip: true
                                ScrollBar.vertical: ScrollBar { }

                                delegate: Item {
                                    width: fieldsListView.width
                                    height: (modelData.rowType === "group")
                                            ? (groupRect.implicitHeight + (ScreenTools.defaultFontPixelHeight * 0.1))
                                            : (fieldRow.implicitHeight + (ScreenTools.defaultFontPixelHeight * 0.1))

                                    Rectangle {
                                        id: groupRect
                                        visible: modelData.rowType === "group"
                                        width: parent.width
                                        implicitHeight: groupLabel.implicitHeight + (ScreenTools.defaultFontPixelHeight * 0.35)
                                        color: qgcPal.windowShadeDark
                                        radius: ScreenTools.defaultFontPixelWidth * 0.25

                                        Row {
                                            anchors.fill: parent
                                            anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 0.2
                                            anchors.verticalCenter: parent.verticalCenter
                                            spacing: ScreenTools.defaultFontPixelWidth * 0.3

                                            QGCLabel { text: (String(fieldSearchText).trim().length > 0) ? "▼" : (isGroupExpanded(modelData.group) ? "▼" : "▶") }
                                            QGCLabel { id: groupLabel; text: modelData.group; font.bold: true }
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: toggleGroupExpanded(modelData.group)
                                        }
                                    }

                                    Row {
                                        id: fieldRow
                                        visible: modelData.rowType === "field"
                                        width: parent.width
                                        height: Math.max(fieldNameLabel.implicitHeight, fieldCheckBox.implicitHeight)
                                        spacing: ScreenTools.defaultFontPixelWidth * 0.25

                                        QGCCheckBox {
                                            id: fieldCheckBox
                                            anchors.verticalCenter: parent.verticalCenter
                                            onClicked: logViewerController.setFieldSelected(modelData.fullName, checked)
                                            checked: logViewerController.selectedFields.indexOf(modelData.fullName) !== -1
                                        }

                                        QGCLabel {
                                            id: fieldNameLabel
                                            anchors.verticalCenter: parent.verticalCenter
                                            width: Math.max(0, parent.width - (ScreenTools.defaultFontPixelWidth * 3))
                                            height: Math.max(implicitHeight, ScreenTools.defaultFontPixelHeight * 1.2)
                                            wrapMode: Text.WordWrap
                                            maximumLineCount: 2
                                            verticalAlignment: Text.AlignVCenter
                                            text: modelData.shortName ? String(modelData.shortName) : ""
                                            color: isFieldSelected(modelData.fullName) ? fieldColor(modelData.fullName) : qgcPal.text
                                        }
                                    }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: qgcPal.windowShadeDark
                            visible: isFirmwareLog
                        }

                        QGCTabBar {
                            id: dataTabBar
                            Layout.fillWidth: true
                            visible: isFirmwareLog

                            QGCTabButton {
                                text: qsTr("Parameters")
                                checked: true
                            }

                            QGCTabButton {
                                text: qsTr("Messages")
                                checked: false
                            }
                        }

                        QGCTextField {
                            id: parameterSearchField
                            Layout.fillWidth: true
                            visible: isFirmwareLog && dataTabBar.currentIndex === 0
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

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.minimumHeight: 0
                            visible: isFirmwareLog && dataTabBar.currentIndex === 0
                            clip: true

                            ListView {
                                id: parametersListView
                                anchors.fill: parent
                                model: filteredParameters
                                spacing: ScreenTools.defaultFontPixelHeight * 0.2
                                clip: true
                                ScrollBar.vertical: ScrollBar { }

                                delegate: QGCLabel {
                                    width: ListView.view.width
                                    wrapMode: Text.WordWrap
                                    maximumLineCount: 2
                                    text: modelData.name + " = " + modelData.value
                                }
                            }
                        }

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.minimumHeight: 0
                            visible: isFirmwareLog && dataTabBar.currentIndex === 1
                            clip: true

                            ListView {
                                id: messagesListView
                                anchors.fill: parent
                                model: logParser.messages
                                spacing: ScreenTools.defaultFontPixelHeight * 0.2
                                clip: true
                                ScrollBar.vertical: ScrollBar { }

                                delegate: QGCLabel {
                                    width: ListView.view.width
                                    wrapMode: Text.WordWrap
                                    maximumLineCount: 3
                                    text: {
                                        const t = Number(modelData.time)
                                        const prefix = isNaN(t) || t < 0 ? "" : ("[" + t.toFixed(3) + "s] ")
                                        return prefix + String(modelData.text)
                                    }
                                }
                            }
                        }

                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: qgcPal.windowShadeDark
                    radius: ScreenTools.defaultFontPixelWidth * 0.5

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: ScreenTools.defaultFontPixelWidth
                        spacing: ScreenTools.defaultFontPixelHeight * 0.5

                        QGCLabel {
                            text: qsTr("Charts and Timeline")
                            font.bold: true
                        }

                        QGCLabel {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            maximumLineCount: 2
                            text: qsTr("Multi-series charts, event markers, and timeline controls are shown here.")
                            visible: logViewerController.sourceType !== LogViewerController.TLog
                        }

                        Item {
                            id: chartContainer
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredHeight: parent.height * 0.72
                            visible: isFirmwareLog

                            GraphsView {
                                id: binChart
                                anchors.fill: parent
                                marginTop: 0
                                marginRight: 0
                                marginBottom: 0
                                marginLeft: 0

                                theme: GraphsTheme {
                                    colorScheme: qgcPal.globalTheme === QGCPalette.Light ? GraphsTheme.ColorScheme.Light : GraphsTheme.ColorScheme.Dark
                                    backgroundColor: qgcPal.windowShadeDark
                                    backgroundVisible: true
                                    plotAreaBackgroundColor: qgcPal.windowShadeDark
                                    grid.mainColor: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.25)
                                    grid.subColor: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.12)
                                    grid.mainWidth: 1
                                    labelBackgroundVisible: false
                                    labelTextColor: qgcPal.text
                                    axisXLabelFont.family: ScreenTools.fixedFontFamily
                                    axisXLabelFont.pointSize: ScreenTools.smallFontPointSize
                                    axisYLabelFont.family: ScreenTools.fixedFontFamily
                                    axisYLabelFont.pointSize: ScreenTools.smallFontPointSize
                                }

                                axisX: ValueAxis {
                                    id: binXAxis
                                    titleText: qsTr("Time (s)")
                                    min: 0
                                    max: 1
                                }

                                axisY: ValueAxis {
                                    id: binYAxis
                                    titleText: qsTr("Value")
                                    min: 0
                                    max: 1
                                }
                            }

                            Rectangle {
                                id: zoomSelectionRect
                                visible: false
                                color: Qt.rgba(1, 1, 1, 0.2)
                                border.color: qgcPal.buttonHighlight
                                border.width: 1
                                z: 1000
                            }

                            MouseArea {
                                id: chartZoomArea
                                anchors.fill: parent
                                enabled: isFirmwareLog && (binXAxis.max > binXAxis.min)
                                hoverEnabled: true
                                acceptedButtons: Qt.LeftButton | Qt.RightButton
                                z: 1001

                                property real _dragStartX: 0

                                onPressed: (mouse) => {
                                    if (mouse.button === Qt.RightButton) {
                                        resetZoom()
                                        return
                                    }
                                    _dragStartX = mouse.x
                                    zoomSelectionRect.x = mouse.x
                                    zoomSelectionRect.y = 0
                                    zoomSelectionRect.width = 0
                                    zoomSelectionRect.height = height
                                    zoomSelectionRect.visible = true
                                    updateCursorInfo(mouse.x, mouse.y, width, height)
                                }

                                onPositionChanged: (mouse) => {
                                    if (pressed && zoomSelectionRect.visible) {
                                        const left = Math.min(_dragStartX, mouse.x)
                                        const right = Math.max(_dragStartX, mouse.x)
                                        zoomSelectionRect.x = left
                                        zoomSelectionRect.width = Math.max(0, right - left)
                                        return
                                    }

                                    if (!pressed) {
                                        updateCursorInfo(mouse.x, mouse.y, width, height)
                                        return
                                    }
                                }

                                onReleased: (mouse) => {
                                    if (!zoomSelectionRect.visible) {
                                        return
                                    }

                                    const dragWidth = zoomSelectionRect.width
                                    zoomSelectionRect.visible = false
                                    // Treat drags narrower than half a character width as accidental clicks, not zoom gestures.
                                    if (dragWidth < ScreenTools.defaultFontPixelWidth * 0.5) {
                                        return
                                    }

                                    const leftX = _pixelToAxisX(zoomSelectionRect.x)
                                    const rightX = _pixelToAxisX(zoomSelectionRect.x + zoomSelectionRect.width)
                                    applyZoomRange(Math.min(leftX, rightX), Math.max(leftX, rightX))
                                    updateCursorInfo(mouse.x, mouse.y, width, height)
                                }

                                onExited: {
                                    cursorVisible = false
                                }
                            }

                            Rectangle {
                                visible: cursorVisible && (isFirmwareLog)
                                x: cursorPixelX
                                y: binChart.plotArea.y
                                width: 1
                                height: binChart.plotArea.height
                                color: qgcPal.buttonHighlight
                                z: 1002
                            }

                            Rectangle {
                                id: cursorPopup
                                visible: cursorVisible && cursorRows.length > 0 && (isFirmwareLog)
                                x: Math.max(0, Math.min(chartContainer.width - width, cursorPixelX + ScreenTools.defaultFontPixelWidth))
                                y: cursorPopupY
                                width: ScreenTools.defaultFontPixelWidth * 30
                                color: qgcPal.windowShade
                                border.color: qgcPal.windowShadeDark
                                radius: ScreenTools.defaultFontPixelWidth * 0.3
                                z: 1003
                                implicitHeight: cursorColumn.implicitHeight + (ScreenTools.defaultFontPixelHeight * 0.6)

                                Column {
                                    id: cursorColumn
                                    anchors.fill: parent
                                    anchors.margins: ScreenTools.defaultFontPixelHeight * 0.3
                                    spacing: ScreenTools.defaultFontPixelHeight * 0.2

                                    QGCLabel {
                                        text: qsTr("t=%1 s").arg(cursorXValue.toFixed(3))
                                        font.bold: true
                                    }

                                    QGCLabel {
                                        visible: cursorModeName.length > 0
                                        text: qsTr("Mode: %1").arg(cursorModeName)
                                        font.bold: true
                                        color: cursorModeName.length > 0 ? modeColor(cursorModeName) : qgcPal.text
                                    }

                                    Repeater {
                                        model: cursorRows

                                        Row {
                                            spacing: ScreenTools.defaultFontPixelWidth * 0.2

                                            Rectangle {
                                                width: ScreenTools.defaultFontPixelWidth * 0.8
                                                height: ScreenTools.defaultFontPixelHeight * 0.6
                                                color: modelData.color
                                            }

                                            QGCLabel {
                                                width: cursorPopup.width - (ScreenTools.defaultFontPixelWidth * 4)
                                                elide: Text.ElideMiddle
                                                text: modelData.name + ": " + Number(modelData.value).toFixed(3)
                                            }
                                        }
                                    }

                                    Repeater {
                                        model: cursorEventRows

                                        Row {
                                            spacing: ScreenTools.defaultFontPixelWidth * 0.2

                                            Rectangle {
                                                width: ScreenTools.defaultFontPixelWidth * 0.8
                                                height: ScreenTools.defaultFontPixelHeight * 0.6
                                                color: modelData.color
                                            }

                                            QGCLabel {
                                                width: cursorPopup.width - (ScreenTools.defaultFontPixelWidth * 4)
                                                wrapMode: Text.WordWrap
                                                maximumLineCount: 2
                                                text: modelData.text
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 0.6
                            visible: isFirmwareLog
                            color: qgcPal.windowShade

                            Repeater {
                                model: logParser.modeSegments

                                Rectangle {
                                    visible: binXAxis.max > binXAxis.min && modelData.end >= binXAxis.min && modelData.start <= binXAxis.max
                                    y: 0
                                    height: parent.height
                                    color: modeColor(modelData.mode)
                                    opacity: 1.0
                                    x: Math.max(binChart.plotArea.x,
                                                Math.min(binChart.plotArea.x + binChart.plotArea.width,
                                                         binChart.plotArea.x + ((Math.max(modelData.start, binXAxis.min) - binXAxis.min) / (binXAxis.max - binXAxis.min)) * binChart.plotArea.width))
                                    width: Math.max(1, ((Math.min(modelData.end, binXAxis.max) - Math.max(modelData.start, binXAxis.min)) / (binXAxis.max - binXAxis.min)) * binChart.plotArea.width)
                                }
                            }

                            Repeater {
                                model: logParser.dropouts

                                Rectangle {
                                    visible: binXAxis.max > binXAxis.min && modelData.end >= binXAxis.min && modelData.start <= binXAxis.max
                                    y: 0
                                    height: parent.height
                                    color: Qt.alpha(eventColor("error"), 0.533)
                                    x: Math.max(binChart.plotArea.x,
                                                Math.min(binChart.plotArea.x + binChart.plotArea.width,
                                                         binChart.plotArea.x + ((Math.max(modelData.start, binXAxis.min) - binXAxis.min) / (binXAxis.max - binXAxis.min)) * binChart.plotArea.width))
                                    width: Math.max(2, ((Math.min(modelData.end, binXAxis.max) - Math.max(modelData.start, binXAxis.min)) / (binXAxis.max - binXAxis.min)) * binChart.plotArea.width)
                                }
                            }

                            Repeater {
                                model: logParser.events

                                Rectangle {
                                    width: 2
                                    height: parent.height
                                    visible: binXAxis.max > binXAxis.min
                                    color: eventColor(modelData.type)
                                    x: Math.max(binChart.plotArea.x,
                                                Math.min((binChart.plotArea.x + binChart.plotArea.width) - width,
                                                         binChart.plotArea.x + ((modelData.time - binXAxis.min) / (binXAxis.max - binXAxis.min)) * binChart.plotArea.width))
                                }
                            }
                        }

                        Row {
                            Layout.fillWidth: true
                            visible: isFirmwareLog && modeLegendEntries().length > 0
                            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.1
                            spacing: ScreenTools.defaultFontPixelWidth

                            QGCLabel {
                                text: qsTr("Modes:")
                                font.bold: true
                            }

                            Repeater {
                                model: modeLegendEntries()

                                Row {
                                    spacing: ScreenTools.defaultFontPixelWidth * 0.2

                                    Rectangle {
                                        width: ScreenTools.defaultFontPixelWidth
                                        height: ScreenTools.defaultFontPixelHeight * 0.6
                                        color: modeColor(modelData)
                                    }

                                    QGCLabel {
                                        text: modelData
                                    }
                                }
                            }
                        }

                        Row {
                            Layout.fillWidth: true
                            visible: isFirmwareLog && logViewerController.selectedFields.length > 0
                            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.1
                            spacing: ScreenTools.defaultFontPixelWidth

                            QGCLabel {
                                text: qsTr("Fields:")
                                font.bold: true
                            }

                            Repeater {
                                model: logViewerController.selectedFields

                                Row {
                                    spacing: ScreenTools.defaultFontPixelWidth * 0.2

                                    Rectangle {
                                        width: ScreenTools.defaultFontPixelWidth
                                        height: ScreenTools.defaultFontPixelHeight * 0.6
                                        color: fieldColor(modelData)
                                    }

                                    QGCLabel {
                                        text: modelData
                                    }
                                }
                            }
                        }

                        Row {
                            Layout.fillWidth: true
                            visible: isFirmwareLog && logParser.events.length > 0
                            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.1
                            spacing: ScreenTools.defaultFontPixelWidth

                            QGCLabel {
                                text: qsTr("Events:")
                                font.bold: true
                            }

                            Repeater {
                                model: ["mode", "event", "error", "warning"]

                                Row {
                                    spacing: ScreenTools.defaultFontPixelWidth * 0.2
                                    visible: {
                                        for (let i = 0; i < logParser.events.length; i++) {
                                            if (logParser.events[i].type === modelData) {
                                                return true
                                            }
                                        }
                                        return false
                                    }

                                    Rectangle {
                                        width: ScreenTools.defaultFontPixelWidth
                                        height: ScreenTools.defaultFontPixelHeight * 0.6
                                        color: eventColor(modelData)
                                    }

                                    QGCLabel {
                                        text: eventTypeLabel(modelData)
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            visible: isFirmwareLog
                            spacing: ScreenTools.defaultFontPixelWidth

                            QGCLabel {
                                Layout.fillWidth: true
                                text: qsTr("Drag on chart to zoom X-axis. Right click chart to reset zoom.")
                            }

                            QGCButton {
                                text: qsTr("Reset Zoom")
                                enabled: zoomMinX !== fullMinX || zoomMaxX !== fullMaxX
                                onClicked: resetZoom()
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

            QGCLabel {
                Layout.fillWidth: true
                text: logViewerController.statusText
                visible: text.length > 0
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
