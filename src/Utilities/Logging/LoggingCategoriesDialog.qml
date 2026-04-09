import QGroundControl
import QGroundControl.Controls
import QGroundControl.Logging
import QtQuick
import QtQuick.Layouts

QGCPopupDialog {
    id: catDialog

    readonly property var _logLevelNames: LogManager.categoryLogLevelNames()
    readonly property var _logLevelValues: LogManager.categoryLogLevelValues()
    readonly property var _flatModel: QGCLoggingCategoryManager.flatModel
    readonly property var _filteredModel: QGCLoggingCategoryManager.filteredFlatModel

    buttons: Dialog.Close
    title: qsTr("Logging Categories")

    QGCPalette {
        id: qgcPal

        colorGroupEnabled: enabled
    }

    ColumnLayout {
        width: maxContentAvailableWidth

        SettingsGroupLayout {
            Layout.fillWidth: true
            heading: qsTr("Search")

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelHeight / 2

                QGCTextField {
                    id: searchText

                    Layout.fillWidth: true
                    placeholderText: qsTr("Filter categories…")

                    onTextChanged: QGCLoggingCategoryManager.setFilterText(text)
                }

                QGCButton {
                    text: qsTr("Clear")

                    onClicked: searchText.text = ""
                }
            }
        }

        SettingsGroupLayout {
            Layout.fillWidth: true
            heading: qsTr("Active Categories")

            Repeater {
                model: _flatModel

                QGCLabel {
                    required property bool enabled
                    required property string fullName
                    required property int logLevel

                    Layout.fillWidth: true
                    text: fullName + "  (" + (_logLevelNames[_logLevelValues.indexOf(logLevel)] ?? qsTr("Warning")) + ")"
                    visible: enabled
                }
            }

            QGCButton {
                text: qsTr("Reset All")

                onClicked: QGCLoggingCategoryManager.disableAllCategories()
            }
        }

        // Tree view (no search)
        SettingsGroupLayout {
            Layout.fillWidth: true
            heading: qsTr("Categories")
            visible: searchText.text === ""

            TreeView {
                id: treeView

                readonly property real _rowHeight: ScreenTools.defaultFontPixelHeight * 2.2

                Layout.fillWidth: true
                Layout.maximumHeight: ScreenTools.defaultFontPixelHeight * 40
                Layout.preferredHeight: contentHeight > 0 ? Math.min(contentHeight, ScreenTools.defaultFontPixelHeight * 40)
                                                          : ScreenTools.defaultFontPixelHeight * 30
                clip: true
                model: QGCLoggingCategoryManager.treeModel

                delegate: Item {
                    id: treeDelegate

                    readonly property real _indent: ScreenTools.defaultFontPixelWidth * 1.5

                    implicitHeight: treeView._rowHeight
                    implicitWidth: treeView.width

                    required property int depth
                    required property bool expanded
                    required property string fullName
                    required property bool hasChildren
                    required property int logLevel
                    required property int row
                    required property string shortName
                    required property TreeView treeView

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: treeDelegate.depth * treeDelegate._indent
                        spacing: ScreenTools.defaultFontPixelWidth * 0.5

                        QGCLabel {
                            text: treeDelegate.expanded ? "\u25BE" : "\u25B8"
                            visible: treeDelegate.hasChildren

                            QGCMouseArea {
                                anchors.fill: parent
                                anchors.margins: -ScreenTools.defaultFontPixelWidth

                                onClicked: treeView.toggleExpanded(treeDelegate.row)
                            }
                        }

                        // Spacer when no expand arrow
                        Item {
                            implicitWidth: ScreenTools.defaultFontPixelWidth
                            visible: !treeDelegate.hasChildren
                        }

                        QGCLabel {
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            text: treeDelegate.shortName
                        }

                        QGCComboBox {
                            currentIndex: _logLevelValues.indexOf(treeDelegate.logLevel)
                            model: _logLevelNames
                            sizeToContents: true

                            onActivated: idx => {
                                const modelIndex = treeView.index(treeDelegate.row, 0);
                                treeView.model.setData(modelIndex, _logLevelValues[idx], LoggingCategoryTreeModel.LogLevelRole);
                            }
                        }
                    }
                }

                Component.onCompleted: _delayedExpand.start()

                Timer {
                    id: _delayedExpand
                    interval: 0
                    onTriggered: treeView.expandRecursively(-1, -1)
                }
            }

            QGCLabel {
                color: qgcPal.colorGrey
                text: qsTr("No categories registered")
                visible: treeView.rows === 0
            }
        }

        // Flat filtered view (with search)
        SettingsGroupLayout {
            Layout.fillWidth: true
            heading: qsTr("Search Results")
            visible: searchText.text !== ""

            Repeater {
                id: searchRepeater

                model: _filteredModel

                RowLayout {
                    required property string fullName
                    required property int index
                    required property int logLevel

                    Layout.fillWidth: true

                    QGCLabel {
                        Layout.fillWidth: true
                        text: fullName
                    }

                    QGCComboBox {
                        currentIndex: _logLevelValues.indexOf(logLevel)
                        model: _logLevelNames
                        sizeToContents: true

                        onActivated: idx => {
                            const sourceIndex = _filteredModel.mapToSource(_filteredModel.index(parent.index, 0));
                            _flatModel.setData(sourceIndex, _logLevelValues[idx], LoggingCategoryFlatModel.LogLevelRole);
                        }
                    }
                }
            }

            QGCLabel {
                color: qgcPal.colorGrey
                text: qsTr("No matching categories")
                visible: searchRepeater.count === 0 && searchText.text !== ""
            }
        }
    }
}
