import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts

import QGroundControl
import QGroundControl.Controls

AnalyzePage {
    id: logViewerPage
    pageComponent: pageComponent
    pageDescription: qsTr("Open and inspect DataFlash (.bin) and telemetry (.tlog) logs in a unified workflow.")

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
            property real fullMinX: 0
            property real fullMaxX: 1
            property real zoomMinX: 0
            property real zoomMaxX: 1

            function eventColor(eventType) {
                return logViewerController.eventColor(eventType)
            }

            function modeColor(modeName) {
                return logViewerController.modeColor(String(modeName))
            }

            function modeLegendEntries() {
                return logViewerController.modeLegendEntries(dataFlashParser.modeSegments)
            }

            function rebuildGroupedSignals() {
                logViewerController.setPlottableSignals(dataFlashParser.plottableSignals)
            }

            function isGroupExpanded(groupName) {
                return logViewerController.isGroupExpanded(groupName)
            }

            function toggleGroupExpanded(groupName) {
                logViewerController.toggleGroupExpanded(groupName)
            }

            function isSignalSelected(signalName) {
                return logViewerController.isSignalSelected(signalName)
            }

            function signalColor(signalName) {
                return logViewerController.signalColor(signalName)
            }

            function toggleSignal(signalName) {
                logViewerController.toggleSignal(signalName)
                refreshBinChart()
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

            function _axisXToPixel(xValue) {
                const plotX = binChart.plotArea.x
                const plotW = binChart.plotArea.width
                if (plotW <= 0 || binXAxis.max <= binXAxis.min) {
                    return plotX
                }

                const ratio = (xValue - binXAxis.min) / (binXAxis.max - binXAxis.min)
                return plotX + (Math.max(0, Math.min(1, ratio)) * plotW)
            }

            function updateCursorInfo(pixelX, pixelY, width, height) {
                const selectedSignals = logViewerController.selectedSignals
                if (selectedSignals.length === 0 || width <= 0 || height <= 0) {
                    cursorVisible = false
                    return
                }

                cursorVisible = true
                cursorPixelX = Math.max(binChart.plotArea.x, Math.min(binChart.plotArea.x + binChart.plotArea.width, pixelX))
                cursorXValue = _pixelToAxisX(cursorPixelX)
                cursorPopupY = Math.max(0, Math.min(height - (ScreenTools.defaultFontPixelHeight * 4), pixelY))
                cursorModeName = dataFlashParser.modeAt(cursorXValue)

                const rows = []
                for (let i = 0; i < selectedSignals.length; i++) {
                    const signal = selectedSignals[i]
                    const value = dataFlashParser.signalValueAt(signal, cursorXValue)
                    if (isNaN(value)) {
                        continue
                    }
                    rows.push({
                        name: signal,
                        color: signalColor(signal),
                        value: value
                    })
                }
                cursorRows = rows

                const threshold = Math.max(0.05, (binXAxis.max - binXAxis.min) / 200.0)
                const nearbyEvents = dataFlashParser.eventsNear(cursorXValue, threshold)
                const events = []
                for (let i = 0; i < nearbyEvents.length; i++) {
                    events.push({
                        color: eventColor(nearbyEvents[i].type),
                        text: nearbyEvents[i].description
                    })
                }
                cursorEventRows = events
            }

            function loadBinFile(file) {
                pendingBinFile = file
                binLoading = true
                parseStartTimer.start()
            }

            function _executePendingBinParse() {
                if (!pendingBinFile || pendingBinFile.length === 0) {
                    binLoading = false
                    return
                }

                Qt.callLater(function() {
                    const ok = dataFlashParser.parseFile(pendingBinFile)
                    if (!ok) {
                        QGroundControl.showMessageDialog(logViewerPage, qsTr("Log Viewer"), dataFlashParser.parseError)
                    }
                    rebuildGroupedSignals()
                    logViewerController.clearSelection()
                    fullMinX = 0
                    fullMaxX = 1
                    zoomMinX = 0
                    zoomMaxX = 1
                    refreshBinChart()
                    logViewerController.openBinLog(pendingBinFile)
                    binLoading = false
                    pendingBinFile = ""
                })
            }

            function refreshBinChart() {
                binXAxis.min = 0
                binXAxis.max = 1
                binYAxis.min = 0
                binYAxis.max = 1
                if (typeof binChart.removeAllSeries === "function") {
                    binChart.removeAllSeries()
                } else {
                    while (binChart.seriesCount > 0) {
                        binChart.removeSeries(binChart.series(0))
                    }
                }

                const selectedSignals = logViewerController.selectedSignals
                if (selectedSignals.length === 0) {
                    if (dataFlashParser.minTimestamp >= 0.0 && dataFlashParser.maxTimestamp > dataFlashParser.minTimestamp) {
                        fullMinX = dataFlashParser.minTimestamp
                        fullMaxX = dataFlashParser.maxTimestamp
                    } else {
                        fullMinX = 0
                        fullMaxX = 1
                    }
                    zoomMinX = fullMinX
                    zoomMaxX = fullMaxX
                    binXAxis.min = zoomMinX
                    binXAxis.max = zoomMaxX
                    // Keep chart initialized (axes visible) even with no selected signals.
                    const emptySeries = binChart.createSeries(ChartView.SeriesTypeLine, "__empty__", binXAxis, binYAxis)
                    emptySeries.visible = false
                    const eventListNoSignals = dataFlashParser.events
                    const eventSeriesByType = ({})
                    for (let e = 0; e < eventListNoSignals.length; e++) {
                        const event = eventListNoSignals[e]
                        if (event.time < binXAxis.min || event.time > binXAxis.max) {
                            continue
                        }
                        if (!eventSeriesByType[event.type]) {
                            const eventSeries = binChart.createSeries(ChartView.SeriesTypeScatter, event.type, binXAxis, binYAxis)
                            eventSeries.markerSize = 9
                            eventSeries.color = eventColor(event.type)
                            eventSeries.borderColor = eventSeries.color
                            eventSeriesByType[event.type] = eventSeries
                        }
                        eventSeriesByType[event.type].append(event.time, binYAxis.max)
                    }
                    return
                }

                let minX = Number.MAX_VALUE
                let maxX = -Number.MAX_VALUE
                let minY = Number.MAX_VALUE
                let maxY = -Number.MAX_VALUE
                for (let s = 0; s < selectedSignals.length; s++) {
                    const signalName = selectedSignals[s]
                    const points = dataFlashParser.signalSamples(signalName)
                    if (!points || points.length === 0) {
                        continue
                    }

                    const series = binChart.createSeries(ChartView.SeriesTypeLine, signalName, binXAxis, binYAxis)
                    series.color = signalColor(signalName)
                    for (let i = 0; i < points.length; i++) {
                        const p = points[i]
                        series.append(p.x, p.y)
                        minX = Math.min(minX, p.x)
                        maxX = Math.max(maxX, p.x)
                        minY = Math.min(minY, p.y)
                        maxY = Math.max(maxY, p.y)
                    }
                }

                if (minX === Number.MAX_VALUE) {
                    minX = 0
                    maxX = 1
                    minY = 0
                    maxY = 1
                } else if (minX === maxX) {
                    maxX = minX + 1
                }
                if (minY === maxY) {
                    maxY = minY + 1
                }

                fullMinX = minX
                fullMaxX = maxX
                if (zoomMaxX <= zoomMinX || zoomMinX < fullMinX || zoomMaxX > fullMaxX) {
                    zoomMinX = fullMinX
                    zoomMaxX = fullMaxX
                }
                binXAxis.min = zoomMinX
                binXAxis.max = zoomMaxX
                binYAxis.min = minY
                binYAxis.max = maxY

                const eventList = dataFlashParser.events
                const eventTypeSeries = ({})
                for (let e = 0; e < eventList.length; e++) {
                    const event = eventList[e]
                    if (event.time < binXAxis.min || event.time > binXAxis.max) {
                        continue
                    }
                    if (!eventTypeSeries[event.type]) {
                        const eventSeries = binChart.createSeries(ChartView.SeriesTypeScatter, event.type, binXAxis, binYAxis)
                        eventSeries.markerSize = 9
                        eventSeries.color = eventColor(event.type)
                        eventSeries.borderColor = eventSeries.color
                        eventTypeSeries[event.type] = eventSeries
                    }
                    eventTypeSeries[event.type].append(event.time, maxY)
                }
            }

            LogViewerController {
                id: logViewerController
            }

            DataFlashLogParser {
                id: dataFlashParser
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
                        replayController.link = null
                        dataFlashParser.clear()
                        logViewerController.setPlottableSignals([])
                        logViewerController.clearSelection()
                        cursorEventRows = []
                        refreshBinChart()
                        logViewerController.clear()
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
                            text: logViewerController.sourceType === LogViewerController.Bin
                                  ? qsTr("DataFlash message and parameter browser will appear here.")
                                  : qsTr("For telemetry replay, MAVLink message and field selection is available through the Inspector integration.")
                        }

                        QGCLabel {
                            visible: logViewerController.sourceType === LogViewerController.Bin
                            text: qsTr("Signals: %1  Parameters: %2  Events: %3")
                                  .arg(dataFlashParser.plottableSignals.length)
                                  .arg(dataFlashParser.parameters.length)
                                  .arg(dataFlashParser.events.length)
                        }

                        QGCLabel {
                            visible: logViewerController.sourceType === LogViewerController.Bin
                            text: qsTr("Signals (click to plot)")
                            font.bold: true
                        }

                        ScrollView {
                            id: signalsScroll
                            Layout.fillWidth: true
                            Layout.preferredHeight: parent.height * 0.45
                            visible: logViewerController.sourceType === LogViewerController.Bin
                            clip: true

                            ListView {
                                id: signalsListView
                                anchors.fill: parent
                                model: logViewerController.signalRows
                                spacing: ScreenTools.defaultFontPixelHeight * 0.15
                                clip: true
                                ScrollBar.vertical: ScrollBar { }

                                delegate: Item {
                                    width: signalsListView.width
                                    height: (modelData.rowType === "group")
                                            ? (groupRect.implicitHeight + (ScreenTools.defaultFontPixelHeight * 0.1))
                                            : (signalRow.implicitHeight + (ScreenTools.defaultFontPixelHeight * 0.1))

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

                                            QGCLabel { text: isGroupExpanded(modelData.group) ? "▼" : "▶" }
                                            QGCLabel { id: groupLabel; text: modelData.group; font.bold: true }
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: toggleGroupExpanded(modelData.group)
                                        }
                                    }

                                    Row {
                                        id: signalRow
                                        visible: modelData.rowType === "signal"
                                        width: parent.width
                                        height: Math.max(signalNameLabel.implicitHeight, signalCheckBox.implicitHeight)
                                        spacing: ScreenTools.defaultFontPixelWidth * 0.25

                                        QGCCheckBox {
                                            id: signalCheckBox
                                            anchors.verticalCenter: parent.verticalCenter
                                            checked: isSignalSelected(modelData.fullName)
                                            onClicked: toggleSignal(modelData.fullName)
                                        }

                                        QGCLabel {
                                            id: signalNameLabel
                                            anchors.verticalCenter: parent.verticalCenter
                                            width: Math.max(0, parent.width - (ScreenTools.defaultFontPixelWidth * 3))
                                            height: Math.max(implicitHeight, ScreenTools.defaultFontPixelHeight * 1.2)
                                            wrapMode: Text.WordWrap
                                            maximumLineCount: 2
                                            verticalAlignment: Text.AlignVCenter
                                            text: modelData.shortName ? String(modelData.shortName) : ""
                                            color: isSignalSelected(modelData.fullName) ? signalColor(modelData.fullName) : qgcPal.text
                                        }
                                    }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: qgcPal.windowShadeDark
                            visible: logViewerController.sourceType === LogViewerController.Bin
                        }

                        QGCLabel {
                            visible: logViewerController.sourceType === LogViewerController.Bin
                            text: qsTr("Parameters")
                            font.bold: true
                        }

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            visible: logViewerController.sourceType === LogViewerController.Bin
                            clip: true

                            ListView {
                                id: parametersListView
                                anchors.fill: parent
                                model: dataFlashParser.parameters
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

                        ChartView {
                            id: binChart
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredHeight: parent.height * 0.72
                            visible: logViewerController.sourceType === LogViewerController.Bin
                            antialiasing: true
                            legend.visible: false

                            ValueAxis {
                                id: binXAxis
                                titleText: qsTr("Time (s)")
                                min: 0
                                max: 1
                            }

                            ValueAxis {
                                id: binYAxis
                                titleText: qsTr("Value")
                                min: 0
                                max: 1
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
                                enabled: logViewerController.sourceType === LogViewerController.Bin && (binXAxis.max > binXAxis.min)
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
                                    if (dragWidth < 8) {
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
                                visible: cursorVisible && logViewerController.sourceType === LogViewerController.Bin
                                x: _axisXToPixel(cursorXValue)
                                y: 0
                                width: 1
                                height: parent.height
                                color: qgcPal.buttonHighlight
                                z: 1002
                            }

                            Rectangle {
                                visible: cursorVisible && cursorRows.length > 0 && logViewerController.sourceType === LogViewerController.Bin
                                x: Math.min(parent.width - width, cursorPixelX + ScreenTools.defaultFontPixelWidth)
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
                                                width: parent.parent.width - (ScreenTools.defaultFontPixelWidth * 4)
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
                                                width: parent.parent.width - (ScreenTools.defaultFontPixelWidth * 4)
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
                            visible: logViewerController.sourceType === LogViewerController.Bin
                            color: qgcPal.windowShade

                            Repeater {
                                model: dataFlashParser.modeSegments

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
                                model: dataFlashParser.events

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
                            visible: logViewerController.sourceType === LogViewerController.Bin && modeLegendEntries().length > 0
                            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.1
                            spacing: ScreenTools.defaultFontPixelWidth

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
                            visible: logViewerController.sourceType === LogViewerController.Bin && dataFlashParser.events.length > 0
                            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.1
                            spacing: ScreenTools.defaultFontPixelWidth

                            Repeater {
                                model: logViewerController.selectedSignals

                                Row {
                                    spacing: ScreenTools.defaultFontPixelWidth * 0.2

                                    Rectangle {
                                        width: ScreenTools.defaultFontPixelWidth
                                        height: ScreenTools.defaultFontPixelHeight * 0.6
                                        color: signalColor(modelData)
                                    }

                                    QGCLabel {
                                        text: modelData
                                    }
                                }
                            }
                        }

                        Row {
                            Layout.fillWidth: true
                            visible: logViewerController.sourceType === LogViewerController.Bin && dataFlashParser.events.length > 0
                            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.1
                            spacing: ScreenTools.defaultFontPixelWidth

                            Repeater {
                                model: ["mode", "event", "error"]

                                Row {
                                    spacing: ScreenTools.defaultFontPixelWidth * 0.2
                                    visible: {
                                        for (let i = 0; i < dataFlashParser.events.length; i++) {
                                            if (dataFlashParser.events[i].type === modelData) {
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
                                        text: modelData
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            visible: logViewerController.sourceType === LogViewerController.Bin
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
                        replayController.link = QGroundControl.linkManager.startLogReplay(file)
                        dataFlashParser.clear()
                        logViewerController.setPlottableSignals([])
                        logViewerController.clearSelection()
                        cursorEventRows = []
                        refreshBinChart()
                        logViewerController.openTLog(file)
                    } else {
                        replayController.link = null
                        loadBinFile(file)
                    }
                    close()
                }
            }

            Rectangle {
                anchors.fill: parent
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
        }
    }
}
