import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
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
                    applyFieldFilter()
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
                            Layout.preferredHeight: parent.height * 0.35
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
                                            color: isFieldSelected(modelData.fullName) ? logViewerChart.fieldColor(modelData.fullName) : qgcPal.text
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

                        LogViewerChart {
                            id: logViewerChart
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            visible: isFirmwareLog
                            logParser: logParser
                            logViewerController: logViewerController
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
