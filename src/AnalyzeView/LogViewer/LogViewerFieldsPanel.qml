import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Left panel for the Log Viewer charting tab.
/// Shows log stats and the grouped fields list.
Rectangle {
    id: control

    required property var logParser
    required property var logViewerController

    signal clearSelectedRequested

    property bool xAxisShowLocalTime: QGroundControl.settingsManager.logViewerSettings.xAxisShowLocalTime.rawValue

    color: qgcPal.windowShade
    radius: ScreenTools.defaultFontPixelWidth * 0.5
    Layout.preferredWidth: mainLayout.implicitWidth + (mainLayout.anchors.margins * 2)

    // -------------------------------------------------------------------------
    // Internal state
    // -------------------------------------------------------------------------
    property string _fieldSearchText: ""
    property var _filteredFieldRows: []
    property real _maxFieldRowWidth: _minFieldRowWidth

    readonly property real _minFieldRowWidth: ScreenTools.defaultFontPixelWidth
    readonly property bool _isFirmwareLog: logViewerController.sourceType === LogViewerController.Bin
                                        || logViewerController.sourceType === LogViewerController.ULog

    QGCPalette { id: qgcPal }

    // -------------------------------------------------------------------------
    // Public API
    // -------------------------------------------------------------------------

    /// Called by the parent after a log finishes loading.
    function rebuildGroupedFields() {
        _maxFieldRowWidth = _minFieldRowWidth
        logViewerController.setPlottableFields(logParser.plottableFields)
        _applyFieldFilter()
    }

    // -------------------------------------------------------------------------
    // Internal helpers
    // -------------------------------------------------------------------------

    function _applyFieldFilter() {
        const query = String(_fieldSearchText).trim().toLowerCase()
        if (query.length === 0) {
            _filteredFieldRows = logViewerController.fieldRows
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
        _filteredFieldRows = rows
    }

    function _isGroupExpanded(groupName) {
        return logViewerController.isGroupExpanded(groupName)
    }

    function _toggleGroupExpanded(groupName) {
        if (String(_fieldSearchText).trim().length > 0) {
            return
        }
        logViewerController.toggleGroupExpanded(groupName)
        _applyFieldFilter()
    }

    // -------------------------------------------------------------------------
    // Connections
    // -------------------------------------------------------------------------

    Connections {
        target: logViewerController
        function onFieldRowsChanged() {
            const savedY = _fieldsListView.contentY
            _applyFieldFilter()
            Qt.callLater(() => { _fieldsListView.contentY = savedY })
        }
    }

    // -------------------------------------------------------------------------
    // UI
    // -------------------------------------------------------------------------

    Component {
        id: _groupRowComponent

        Item {
            width: _maxFieldRowWidth
            implicitWidth: _groupLayout.implicitWidth
            implicitHeight: _groupLayout.implicitHeight

            Component.onCompleted: _maxFieldRowWidth = Math.max(_maxFieldRowWidth, implicitWidth)

            RowLayout {
                id: _groupLayout
                spacing: ScreenTools.defaultFontPixelWidth / 2

                QGCColoredImage {
                    Layout.preferredWidth: _groupLabel.height * 0.5
                    Layout.preferredHeight: _groupLabel.height * 0.5
                    source: "/qmlimages/arrow-down.png"
                    color: qgcPal.text
                    fillMode: Image.PreserveAspectFit
                    rotation: (String(control._fieldSearchText).trim().length > 0 || control._isGroupExpanded(rowData.group)) ? 0 : -90
                }

                QGCLabel {
                    id: _groupLabel
                    text: rowData.group
                    font.bold: true
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: control._toggleGroupExpanded(rowData.group)
            }
        }
    }

    Component {
        id: _fieldRowComponent

        QGCCheckBoxSlider {
            id: _fieldSlider
            width: _maxFieldRowWidth
            checked: logViewerController.selectedFields.indexOf(rowData.fullName) !== -1
            text: rowData.shortName ? " " + String(rowData.shortName) : ""
            onClicked: logViewerController.setFieldSelected(rowData.fullName, checked)

            Component.onCompleted: _maxFieldRowWidth = Math.max(_maxFieldRowWidth, implicitWidth)
        }
    }

    ColumnLayout {
        id: mainLayout
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.margins: ScreenTools.defaultFontPixelWidth / 2
        spacing: ScreenTools.defaultFontPixelHeight * 0.5

        QGCLabel {
            visible: _isFirmwareLog
            text: qsTr("Fields: %1  Parameters: %2  Events: %3")
                  .arg(logParser.plottableFields.length)
                  .arg(logParser.parameters.length)
                  .arg(logParser.events.length)
        }

        RowLayout {
            visible: _isFirmwareLog
                     && logParser.startTime
                     && !isNaN(logParser.startTime.getTime())
                     && logParser.startTime.getTime() > 0
            spacing: ScreenTools.defaultFontPixelWidth

            QGCLabel { text: qsTr("X axis:") }

            ButtonGroup { id: _xAxisButtonGroup }

            QGCRadioButton {
                text: qsTr("Elapsed")
                checked: !control.xAxisShowLocalTime
                ButtonGroup.group: _xAxisButtonGroup
                onClicked: QGroundControl.settingsManager.logViewerSettings.xAxisShowLocalTime.rawValue = false
            }

            QGCRadioButton {
                text: qsTr("Local time")
                checked: control.xAxisShowLocalTime
                ButtonGroup.group: _xAxisButtonGroup
                onClicked: QGroundControl.settingsManager.logViewerSettings.xAxisShowLocalTime.rawValue = true
            }
        }

        RowLayout {
            visible: _isFirmwareLog
            spacing: ScreenTools.defaultFontPixelWidth * 0.5

            QGCLabel {
                text: qsTr("Fields")
                font.bold: true
            }

            QGCTextField {
                id: _fieldSearchField
                Layout.fillWidth: true
                textColor: qgcPal.textFieldText
                placeholderTextColor: Qt.rgba(qgcPal.textFieldText.r, qgcPal.textFieldText.g, qgcPal.textFieldText.b, 0.7)
                placeholderText: qsTr("Search fields")

                onTextChanged: {
                    control._fieldSearchText = text
                    if (text.trim().length === 0) {
                        _fieldSearchTimer.stop()
                        control._applyFieldFilter()
                    } else {
                        _fieldSearchTimer.restart()
                    }
                }

                onAccepted: {
                    control._fieldSearchText = text
                    _fieldSearchTimer.stop()
                    control._applyFieldFilter()
                }
            }

            QGCButton {
                text: qsTr("Clear Selected")
                enabled: logViewerController.selectedFields.length > 0

                onClicked: {
                    logViewerController.clearSelection()
                    control._applyFieldFilter()
                    control.clearSelectedRequested()
                }
            }
        }

        QGCListView {
            id: _fieldsListView
            Layout.fillHeight: true
            Layout.preferredWidth: _maxFieldRowWidth + ScreenTools.defaultFontPixelWidth
            visible: _isFirmwareLog
            model: _filteredFieldRows
            spacing: ScreenTools.defaultFontPixelHeight * 0.25

            delegate: Loader {
                sourceComponent: modelData.rowType === "group" ? _groupRowComponent : _fieldRowComponent
                property var rowData: modelData
            }
        }
    }

    Timer {
        id: _fieldSearchTimer
        interval: 250
        repeat: false
        onTriggered: control._applyFieldFilter()
    }
}
