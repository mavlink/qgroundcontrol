import QGroundControl
import QGroundControl.Controls
import QGroundControl.Logging
import QtQuick
import QtQuick.Layouts

Item {
    id: root

    readonly property real _indicatorWidth: ScreenTools.defaultFontPixelWidth * 0.4
    readonly property real _margin: ScreenTools.defaultFontPixelWidth
    readonly property real _rowHeight: ScreenTools.defaultFontPixelHeight * 1.6

    function _levelColor(lvl) {
        switch (lvl) {
        case LogEntry.Debug:    return qgcPal.colorGrey
        case LogEntry.Info:     return qgcPal.text
        case LogEntry.Warning:  return qgcPal.colorOrange
        case LogEntry.Critical: return qgcPal.colorRed
        case LogEntry.Fatal:    return qgcPal.colorRed
        default:                return qgcPal.text
        }
    }

    QGCPalette {
        id: qgcPal

        colorGroupEnabled: enabled
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Column header ────────────────────────────────────────
        HorizontalHeaderView {
            id: headerView

            Layout.fillWidth: true
            clip: true
            syncView: tableView

            delegate: Rectangle {
                required property var display

                color: qgcPal.windowShade
                implicitHeight: headerLabel.contentHeight + ScreenTools.defaultFontPixelHeight * 0.4
                implicitWidth: headerLabel.contentWidth + ScreenTools.defaultFontPixelWidth * 2

                QGCLabel {
                    id: headerLabel

                    anchors.left: parent.left
                    anchors.leftMargin: _margin * 0.5
                    anchors.verticalCenter: parent.verticalCenter
                    font.bold: true
                    font.family: ScreenTools.fixedFontFamily
                    font.pointSize: ScreenTools.defaultFontPointSize
                    text: parent.display ?? ""
                }

                Rectangle {
                    anchors.bottom: parent.bottom
                    color: qgcPal.groupBorder
                    height: 1
                    width: parent.width
                }

                Rectangle {
                    anchors.right: parent.right
                    color: qgcPal.groupBorder
                    height: parent.height
                    width: 1
                }
            }
        }

        // ── Log table ────────────────────────────────────────────
        TableView {
            id: tableView

            property bool _loadCompleted: false
            property real _cachedWidth: 0

            Layout.fillHeight: true
            Layout.fillWidth: true
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            columnSpacing: 0
            columnWidthProvider: function (col) {
                const explicit_ = tableView.explicitColumnWidth(col)
                if (explicit_ > 0) {
                    return explicit_;
                }
                const cw = ScreenTools.defaultFontPixelWidth
                const w = _cachedWidth
                const compact = w < cw * 60
                switch (col) {
                case LogEntry.TimestampColumn:
                    return compact ? cw * 9 : cw * 12;
                case LogEntry.LevelColumn:
                    return compact ? cw * 5 : cw * 7;
                case LogEntry.CategoryColumn:
                    return compact ? cw * 14 : cw * 22;
                case LogEntry.SourceColumn:
                    return compact ? cw * 16 : cw * 22;
                case LogEntry.MessageColumn: {
                    const fixedWidth = compact ? cw * 44 : cw * 63
                    return Math.max(cw * 30, w - fixedWidth)
                }
                }
                return -1;
            }
            model: LogManager.model
            resizableColumns: true
            rowSpacing: 0
            reuseItems: true

            onWidthChanged: {
                _cachedWidth = width;
                _layoutTimer.restart();
            }

            Timer {
                id: _layoutTimer

                interval: 0

                onTriggered: tableView.forceLayout()
            }

            delegate: Rectangle {
                required property int column
                required property var display
                required property int level
                required property int row

                color: row % 2 === 0 ? qgcPal.window : qgcPal.windowShade
                implicitHeight: _rowHeight

                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.top: parent.top
                    color: root._levelColor(parent.level)
                    visible: parent.column === 0 && parent.level >= LogEntry.Warning
                    width: _indicatorWidth
                }

                QGCLabel {
                    anchors.left: parent.left
                    anchors.leftMargin: parent.column === 0 ? _indicatorWidth + _margin * 0.3 : _margin * 0.3
                    anchors.right: parent.right
                    anchors.rightMargin: _margin * 0.3
                    anchors.verticalCenter: parent.verticalCenter
                    color: root._levelColor(parent.level)
                    elide: Text.ElideRight
                    font.family: ScreenTools.fixedFontFamily
                    font.pointSize: ScreenTools.defaultFontPointSize
                    text: parent.display ?? ""
                }
            }

            QGCLabel {
                color: qgcPal.colorGrey
                text: qsTr("No log entries")
                visible: tableView.rows === 0
                x: tableView.contentX + (tableView.width - width) / 2
                y: tableView.contentY + (tableView.height - height) / 2
            }

            Component.onCompleted: {
                _cachedWidth = width;
                _loadCompleted = true;
                if (rows > 0)
                    positionViewAtRow(rows - 1, TableView.AlignBottom);
            }

            Timer {
                id: scrollTimer

                interval: 50

                onTriggered: {
                    if (tableView.rows > 0)
                        tableView.positionViewAtRow(tableView.rows - 1, TableView.AlignBottom);
                }
            }

            Connections {
                function onRowsInserted() {
                    if (tableView._loadCompleted && followTail.checked)
                        scrollTimer.restart();
                }

                target: LogManager.model
            }
        }

        // ── Separator ─────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: qgcPal.groupBorder
        }

        // ── Filter bar ────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: filterRow.implicitHeight + _margin
            color: qgcPal.windowShade

            RowLayout {
                id: filterRow

                anchors.fill: parent
                anchors.leftMargin: _margin
                anchors.rightMargin: _margin
                spacing: _margin * 0.75

                QGCComboBox {
                    id: levelCombo

                    model: [qsTr("All Levels"), qsTr("Debug"), qsTr("Info"), qsTr("Warning"), qsTr("Critical"), qsTr("Fatal")]
                    sizeToContents: true

                    Component.onCompleted: currentIndex = LogManager.model.filterLevel + 1
                    onActivated: index => {
                        LogManager.model.filterLevel = index - 1;
                    }
                }

                QGCComboBox {
                    id: categoryCombo

                    model: [qsTr("All Categories")].concat(LogManager.model.categoriesList)
                    sizeToContents: true

                    onActivated: index => {
                        LogManager.model.filterCategory = index === 0 ? "" : model[index];
                    }
                }

                QGCTextField {
                    id: searchField

                    Layout.fillWidth: true
                    Layout.minimumWidth: _margin * 10
                    placeholderText: qsTr("Search…")

                    onTextChanged: LogManager.model.setFilterTextDeferred(text)
                }

                QGCButton {
                    ToolTip.text: qsTr("Regex search")
                    ToolTip.visible: hovered
                    checkable: true
                    checked: LogManager.model.filterRegex
                    text: qsTr(".*")

                    onClicked: LogManager.model.filterRegex = checked
                }

                QGCLabel {
                    color: qgcPal.colorRed
                    font.bold: true
                    text: qsTr("\u26A0 Disk Error")
                    visible: LogManager.hasError

                    QGCMouseArea {
                        anchors.fill: parent

                        onClicked: LogManager.clearError()
                    }
                }

                QGCButton {
                    id: followTail

                    checkable: true
                    checked: true
                    text: qsTr("Follow")

                    onCheckedChanged: {
                        if (checked && tableView._loadCompleted && tableView.rows > 0)
                            tableView.positionViewAtRow(tableView.rows - 1, TableView.AlignBottom);
                    }
                }

                QGCButton {
                    text: qsTr("Categories")

                    onClicked: filtersDialogFactory.open()
                }

                QGCButton {
                    text: qsTr("Settings")

                    onClicked: settingsDialogFactory.open()
                }
            }
        }
    }

    QGCPopupDialogFactory {
        id: filtersDialogFactory

        dialogComponent: Component {
            LoggingCategoriesDialog {
            }
        }
    }

    QGCPopupDialogFactory {
        id: settingsDialogFactory

        dialogComponent: Component {
            LoggingSettingsDialog {
            }
        }
    }
}
