import QGroundControl
import QGroundControl.Controls
import QGroundControl.Logging
import QtQuick
import QtQuick.Layouts

QGCPopupDialog {
    id: catDialog

    readonly property var _filteredModel: QGCLoggingCategoryManager.filteredFlatModel

    buttons: Dialog.Close
    title: qsTr("Logging Categories")

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    QGCCheckBoxSlider {
        id: _measureSlider
        visible: false
    }

    ColumnLayout {
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

                    onTextChanged: _searchDebounce.restart()

                    Timer {
                        id: _searchDebounce
                        interval: 200
                        repeat: false
                        onTriggered: QGCLoggingCategoryManager.setFilterText(searchText.text)
                    }
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
            showDividers: false

            Repeater {
                model: QGCLoggingCategoryManager.enabledCategories

                QGCCheckBoxSlider {
                    required property string modelData

                    Layout.fillWidth: true
                    checked: true
                    text: modelData
                    onToggled: {
                        // Display uses "ADSB.*" but manager stores "ADSB." — strip trailing *
                        const key = modelData.endsWith(".*") ? modelData.slice(0, -1) : modelData
                        QGCLoggingCategoryManager.setCategoryEnabled(key, false)
                    }
                }
            }

            QGCButton {
                text: qsTr("Reset All")

                onClicked: QGCLoggingCategoryManager.disableAllCategories()
            }
        }

        // Category list (no search)
        SettingsGroupLayout {
            Layout.fillWidth: true
            heading: qsTr("Categories")
            visible: searchText.text === ""

            TreeView {
                id: treeView
                Layout.preferredWidth: _maxRowWidth
                Layout.maximumHeight: ScreenTools.defaultFontPixelHeight * 40
                Layout.preferredHeight: contentHeight > 0 ? Math.min(contentHeight, ScreenTools.defaultFontPixelHeight * 40)
                                                          : ScreenTools.defaultFontPixelHeight * 30
                rowSpacing: ScreenTools.defaultFontPixelHeight * 0.25
                model: QGCLoggingCategoryManager.treeModel
                clip: true

                property real _maxRowWidth: ScreenTools.defaultFontPixelWidth

                delegate: RowLayout {
                    id: treeViewRowLayout
                    implicitWidth: treeView._maxRowWidth
                    spacing: ScreenTools.defaultFontPixelWidth * 0.5

                    required property int depth
                    required property bool expanded
                    required property string fullName
                    required property bool hasChildren
                    required property bool categoryEnabled
                    required property int row
                    required property string shortName
                    required property TreeView treeView

                    Component.onCompleted: _calcMaxRowWidth();
                    TableView.onReused: _calcMaxRowWidth();

                    function _calcMaxRowWidth() {
                        let indentWidth = treeViewRowLayout.depth > 0 ? rowIndent.width : 0
                        let arrowWidth = treeViewRowLayout.hasChildren ? expandArrow.width : 0
                        let checkBoxSliderWidth = _measureSlider.width + ScreenTools.defaultFontPixelWidth * treeViewRowLayout.shortName.length
                        let spacing = (rowIndent.visible ? treeViewRowLayout.spacing : 0) + (treeViewRowLayout.hasChildren ? treeViewRowLayout.spacing : 0)
                        let rowWidth = indentWidth + arrowWidth + checkBoxSliderWidth + spacing
                        //console.log("update", treeViewRowLayout.fullName, indentWidth, arrowWidth, checkBoxSliderWidth, spacing, rowWidth, treeView._maxRowWidth)
                        if (rowWidth > treeView._maxRowWidth) {
                            treeView._maxRowWidth = rowWidth
                        }
                    }

                    Item {
                        id: rowIndent
                        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * treeViewRowLayout.depth
                        Layout.preferredHeight: 1
                        visible: treeViewRowLayout.depth > 0
                    }

                    QGCColoredImage {
                        id: expandArrow
                        Layout.preferredWidth: ScreenTools.defaultFontPixelHeight * 0.5
                        Layout.preferredHeight: Layout.preferredWidth
                        color: qgcPal.text
                        fillMode: Image.PreserveAspectFit
                        rotation: treeViewRowLayout.expanded ? 0 : -90
                        source: "/qmlimages/arrow-down.png"
                        visible: treeViewRowLayout.hasChildren

                        QGCMouseArea {
                            anchors.fill: parent
                            anchors.margins: -ScreenTools.defaultFontPixelWidth

                            onClicked: treeView.toggleExpanded(treeViewRowLayout.row)
                        }
                    }

                    QGCCheckBoxSlider {
                        id: checkBoxSlider
                        Layout.fillWidth: true
                        text: treeViewRowLayout.shortName
                        checked: treeViewRowLayout.categoryEnabled
                        onToggled: QGCLoggingCategoryManager.setCategoryEnabled(treeViewRowLayout.fullName, checked)
                    }
                }
            }
        }

        // Flat filtered view (with search)
        SettingsGroupLayout {
            heading: qsTr("Search Results")
            visible: searchText.text !== ""

            QGCListView {
                id: searchResultsView
                Layout.preferredWidth: _maxRowWidth
                Layout.preferredHeight: Math.min(contentHeight, ScreenTools.defaultFontPixelHeight * 40)
                clip: true
                model: _filteredModel
                spacing: ScreenTools.defaultFontPixelHeight * 0.25

                property real _maxRowWidth: ScreenTools.defaultFontPixelWidth

                delegate: QGCCheckBoxSlider {
                    required property bool categoryEnabled
                    required property string fullName

                    width: searchResultsView._maxRowWidth
                    text: fullName
                    checked: categoryEnabled
                    onToggled: QGCLoggingCategoryManager.setCategoryEnabled(fullName, checked)

                    Component.onCompleted: _updateMaxWidth()
                    ListView.onReused: _updateMaxWidth()

                    function _updateMaxWidth() {
                        let rowWidth = _measureSlider.width + ScreenTools.defaultFontPixelWidth * fullName.length
                        if (rowWidth > searchResultsView._maxRowWidth) {
                            searchResultsView._maxRowWidth = rowWidth
                        }
                    }
                }
            }

            QGCLabel {
                color: qgcPal.colorGrey
                text: qsTr("No matching categories")
                visible: searchResultsView.count === 0 && searchText.text !== ""
            }
        }
    }
}
