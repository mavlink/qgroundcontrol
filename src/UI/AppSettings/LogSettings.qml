import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root

    property var _categoriesDialog: null
    property bool _exportFiltered: false
    property int _exportFormatIndex: 0
    property var _exportFormats: [qsTr("Text (.txt)"), qsTr("JSON (.json)"), qsTr("CSV (.csv)"), qsTr("JSON Lines (.jsonl)")]
    property var _exportNameFilters: [[qsTr("Text files (*.txt)"), qsTr("All Files (*)")], [qsTr("JSON files (*.json)"), qsTr("All Files (*)")], [qsTr("CSV files (*.csv)"), qsTr("All Files (*)")], [qsTr("JSON Lines files (*.jsonl)"), qsTr("All Files (*)")]]
    property bool _filtersActive: qgcLogging.model.filterLevel !== -1 || qgcLogging.model.filterCategory !== "" || qgcLogging.model.filterText !== ""
    property bool _listViewLoadCompleted: false
    property bool _showSource: false
    property var _settingsDialog: null

    function _openCategoriesDialog() {
        if (!_categoriesDialog) {
            _categoriesDialog = categoriesDialogComponent.createObject(mainWindow);
            _categoriesDialog.closed.connect(function () {
                _categoriesDialog = null;
            });
            _categoriesDialog.open();
        }
    }

    function _openSettingsDialog() {
        if (!_settingsDialog) {
            _settingsDialog = settingsDialogComponent.createObject(mainWindow);
            _settingsDialog.closed.connect(function () {
                _settingsDialog = null;
            });
            _settingsDialog.open();
        }
    }

    spacing: 0

    QGCPalette {
        id: qgcPal

        colorGroupEnabled: true
    }

    Rectangle {
        id: errorBanner

        Layout.fillWidth: true
        Layout.preferredHeight: visible ? errorRow.height + ScreenTools.defaultFontPixelHeight : 0
        color: qgcPal.warningText
        visible: qgcLogging.hasError

        RowLayout {
            id: errorRow

            anchors.centerIn: parent
            spacing: ScreenTools.defaultFontPixelWidth

            QGCLabel {
                color: "white"
                text: qgcLogging.lastError || qsTr("Logging error occurred")
            }

            QGCButton {
                text: qsTr("Clear")

                onClicked: qgcLogging.clearError()
            }
        }
    }

    Component {
        id: delegateItem

        Rectangle {
            color: index % 2 == 0 ? qgcPal.window : qgcPal.windowShade
            height: Math.round(ScreenTools.defaultFontPixelHeight * 0.5 + field.height)
            width: listView.contentItem.width

            QGCLabel {
                id: field

                anchors.verticalCenter: parent.verticalCenter
                color: {
                    switch (level) {
                    case 0:
                        return qgcPal.colorGrey;
                    case 1:
                        return qgcPal.text;
                    case 2:
                        return qgcPal.colorOrange;
                    case 3:
                        return qgcPal.colorRed;
                    case 4:
                        return qgcPal.colorRed;
                    default:
                        return qgcPal.text;
                    }
                }
                text: _showSource && file !== "" ? formatted + "  \u27F5 " + file + ":" + line + " " + model["function"] : formatted
                width: parent.width
                wrapMode: Text.Wrap
            }
        }
    }

    QGCListView {
        id: listView

        function scrollToEnd() {
            if (_listViewLoadCompleted && followTail.checked) {
                listView.positionViewAtEnd();
            }
        }

        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.margins: ScreenTools.defaultFontPixelWidth
        clip: true
        delegate: delegateItem
        model: qgcLogging.model

        Component.onCompleted: {
            _listViewLoadCompleted = true;
            listView.scrollToEnd();
        }

        Timer {
            id: scrollTimer

            interval: 50

            onTriggered: listView.scrollToEnd()
        }

        Connections {
            function onCountChanged() {
                if (_listViewLoadCompleted && followTail.checked) {
                    scrollTimer.restart();
                }
            }

            target: qgcLogging.model
        }
    }

    Rectangle {
        id: filterBar

        Layout.fillWidth: true
        Layout.preferredHeight: filterRow.height + ScreenTools.defaultFontPixelWidth
        color: qgcPal.windowShade

        RowLayout {
            id: filterRow

            anchors.left: parent.left
            anchors.margins: ScreenTools.defaultFontPixelWidth
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: ScreenTools.defaultFontPixelWidth

            QGCComboBox {
                id: levelCombo

                currentIndex: qgcLogging.model.filterLevel + 1
                model: [qsTr("All Levels"), qsTr("Debug"), qsTr("Info"), qsTr("Warning"), qsTr("Critical"), qsTr("Fatal")]
                sizeToContents: true

                onActivated: index => {
                    qgcLogging.model.filterLevel = index - 1;
                }
            }

            QGCComboBox {
                id: categoryCombo

                property var categoryList: []

                function rebuildModel() {
                    const cats = qgcLogging.model.categories();
                    const prev = categoryCombo.currentText;
                    categoryList = [qsTr("All Categories")].concat(cats);
                    categoryCombo.model = categoryList;
                    const idx = categoryList.indexOf(prev);
                    categoryCombo.currentIndex = idx >= 0 ? idx : 0;
                }

                sizeToContents: true

                Component.onCompleted: rebuildModel()
                onActivated: index => {
                    qgcLogging.model.filterCategory = index === 0 ? "" : categoryList[index];
                }

                Connections {
                    function onTotalCountChanged() {
                        categoryCombo.rebuildModel();
                    }

                    target: qgcLogging.model
                }
            }

            QGCTextField {
                id: searchField

                Layout.fillWidth: true
                placeholderText: qsTr("Search…")

                onTextChanged: qgcLogging.model.filterText = text
            }

            QGCButton {
                enabled: _filtersActive
                text: qsTr("Clear")

                onClicked: {
                    qgcLogging.model.clearFilters();
                    levelCombo.currentIndex = 0;
                    categoryCombo.currentIndex = 0;
                    searchField.text = "";
                }
            }

            QGCLabel {
                text: {
                    const total = qgcLogging.model.totalCount;
                    const shown = qgcLogging.model.count;
                    if (_filtersActive)
                        return qsTr("%1 of %2").arg(shown).arg(total);
                    return qsTr("%1 entries").arg(total);
                }
            }
        }
    }

    QGCFileDialog {
        id: writeDialog

        folder: QGroundControl.settingsManager.appSettings.crashSavePath
        nameFilters: _exportNameFilters[_exportFormatIndex]
        title: qsTr("Select log save file")

        onAcceptedForSave: file => {
            if (_exportFiltered)
                qgcLogging.exportFilteredToFile(file, _exportFormatIndex);
            else
                qgcLogging.exportToFile(file, _exportFormatIndex);
        }
    }

    Connections {
        function onExportFinished(success) {
            writeButton.enabled = true;
            saveFilteredButton.enabled = true;
            if (success) {
                mainWindow.showMessageDialog(qsTr("Export Complete"), qsTr("Log file saved successfully."));
            } else {
                mainWindow.showMessageDialog(qsTr("Export Failed"), qsTr("Failed to save log file. Check disk space and permissions."));
            }
        }

        function onExportStarted() {
            writeButton.enabled = false;
            saveFilteredButton.enabled = false;
        }

        target: qgcLogging
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: buttonRow.height + ScreenTools.defaultFontPixelWidth
        color: qgcPal.window

        RowLayout {
            id: buttonRow

            anchors.left: parent.left
            anchors.margins: ScreenTools.defaultFontPixelWidth
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: ScreenTools.defaultFontPixelWidth

            QGCComboBox {
                id: formatCombo

                model: _exportFormats
                sizeToContents: true

                onActivated: index => {
                    _exportFormatIndex = index;
                }
            }

            QGCButton {
                id: writeButton

                text: qsTr("Save App Log")

                onClicked: {
                    _exportFiltered = false;
                    writeDialog.openForSave();
                }
            }

            QGCButton {
                id: saveFilteredButton

                text: qsTr("Save Filtered")
                visible: _filtersActive

                onClicked: {
                    _exportFiltered = true;
                    writeDialog.openForSave();
                }
            }

            QGCLabel {
                text: qsTr("GStreamer Debug Level")
                visible: QGroundControl.settingsManager.appSettings.gstDebugLevel.visible
            }

            FactComboBox {
                fact: QGroundControl.settingsManager.appSettings.gstDebugLevel
                sizeToContents: true
                visible: QGroundControl.settingsManager.appSettings.gstDebugLevel.visible
            }

            Item {
                Layout.fillWidth: true
            }

            RowLayout {
                spacing: ScreenTools.defaultFontPixelWidth / 2

                Rectangle {
                    Accessible.description: qsTr("Disk Logging: %1").arg(qgcLogging.diskLoggingEnabled ? qsTr("On") : qsTr("Off"))
                    Accessible.name: qsTr("Toggle disk logging")
                    Accessible.role: Accessible.Button
                    border.color: qgcPal.text
                    border.width: 1
                    color: qgcLogging.diskLoggingEnabled ? "green" : qgcPal.windowShade
                    implicitHeight: implicitWidth
                    implicitWidth: ScreenTools.defaultFontPixelHeight * 0.75
                    radius: implicitWidth / 2

                    QGCMouseArea {
                        ToolTip.text: qsTr("Disk Logging: %1").arg(qgcLogging.diskLoggingEnabled ? qsTr("On") : qsTr("Off"))
                        ToolTip.visible: containsMouse
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        hoverEnabled: true

                        onClicked: qgcLogging.diskLoggingEnabled = !qgcLogging.diskLoggingEnabled
                    }
                }

                Rectangle {
                    Accessible.description: qsTr("Remote Logging: %1").arg(qgcLogging.remoteLoggingEnabled ? qsTr("On") : qsTr("Off"))
                    Accessible.name: qsTr("Open remote logging settings")
                    Accessible.role: Accessible.Button
                    border.color: qgcPal.text
                    border.width: 1
                    color: qgcLogging.remoteLoggingEnabled ? "blue" : qgcPal.windowShade
                    implicitHeight: implicitWidth
                    implicitWidth: ScreenTools.defaultFontPixelHeight * 0.75
                    radius: implicitWidth / 2

                    QGCMouseArea {
                        ToolTip.text: qsTr("Remote Logging: %1").arg(qgcLogging.remoteLoggingEnabled ? qsTr("On") : qsTr("Off"))
                        ToolTip.visible: containsMouse
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        hoverEnabled: true

                        onClicked: root._openSettingsDialog()
                    }
                }
            }

            QGCButton {
                id: followTail

                checkable: true
                checked: true
                text: qsTr("Show Latest")

                onCheckedChanged: {
                    if (checked && _listViewLoadCompleted) {
                        listView.positionViewAtEnd();
                    }
                }
            }

            QGCButton {
                checkable: true
                checked: _showSource
                text: qsTr("Source")

                onClicked: _showSource = !_showSource
            }

            QGCButton {
                text: qsTr("Categories")

                onClicked: root._openCategoriesDialog()
            }

            QGCButton {
                text: qsTr("Settings")

                onClicked: root._openSettingsDialog()
            }
        }
    }

    Component {
        id: settingsDialogComponent

        LogSettingsDialog {}
    }

    Component {
        id: categoriesDialogComponent

        LogCategoriesDialog {}
    }
}
