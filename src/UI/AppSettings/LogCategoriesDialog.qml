import QGroundControl
import QGroundControl.Controls
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

QGCPopupDialog {
    buttons: Dialog.Close
    title: qsTr("Logging Categories")

    QGCPalette {
        id: qgcPal

        colorGroupEnabled: true
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

                    Accessible.name: qsTr("Search categories")
                    Layout.fillWidth: true
                }

                QGCButton {
                    text: qsTr("Clear")

                    onClicked: searchText.text = ""
                }
            }
        }

        SettingsGroupLayout {
            Layout.fillWidth: true
            heading: qsTr("Enabled Categories")

            Flow {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelHeight / 2

                Repeater {
                    model: QGroundControl.flatLoggingCategoriesModel()

                    QGCCheckBoxSlider {
                        Layout.fillWidth: true
                        Layout.maximumHeight: visible ? implicitHeight : 0
                        text: object.fullName
                        visible: object.enabled

                        Binding on checked {
                            value: object.enabled
                        }

                        onClicked: object.enabled = checked
                    }
                }

                QGCButton {
                    text: qsTr("Disable All")

                    onClicked: QGroundControl.disableAllLoggingCategories()
                }
            }
        }

        // Shown when not filtered
        Flow {
            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelHeight / 2
            visible: searchText.text === ""

            Repeater {
                id: treeRepeater

                model: QGroundControl.treeLoggingCategoriesModel()

                ColumnLayout {
                    id: parentDelegate

                    required property var object

                    spacing: ScreenTools.defaultFontPixelHeight / 2

                    RowLayout {
                        spacing: ScreenTools.defaultFontPixelWidth

                        QGCLabel {
                            Layout.alignment: Qt.AlignVCenter
                            font.pointSize: ScreenTools.defaultFontPointSize
                            text: parentDelegate.object.expanded ? "\u25BE" : "\u25B8"
                            visible: parentDelegate.object.children.count > 0

                            QGCMouseArea {
                                anchors.fill: parent
                                anchors.margins: -ScreenTools.defaultFontPixelWidth
                                cursorShape: Qt.PointingHandCursor

                                onClicked: parentDelegate.object.expanded = !parentDelegate.object.expanded
                            }
                        }

                        QGCCheckBoxSlider {
                            Layout.fillWidth: true
                            text: parentDelegate.object.shortName

                            Binding on checked {
                                value: parentDelegate.object.enabled
                            }

                            onClicked: parentDelegate.object.enabled = checked
                        }
                    }

                    Repeater {
                        model: parentDelegate.object.expanded ? parentDelegate.object.children : undefined

                        QGCCheckBoxSlider {
                            Layout.fillWidth: true
                            leftPadding: ScreenTools.defaultFontPixelWidth * 3
                            text: object.shortName

                            Binding on checked {
                                value: object.enabled
                            }

                            onClicked: object.enabled = checked
                        }
                    }
                }
            }

            QGCLabel {
                color: qgcPal.colorGrey
                text: qsTr("No categories registered")
                visible: treeRepeater.count === 0
            }
        }

        // Shown when filtered
        Flow {
            id: searchFlow

            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelHeight / 2
            visible: searchText.text !== ""

            Repeater {
                id: searchRepeater

                model: QGroundControl.flatLoggingCategoriesModel()

                QGCCheckBoxSlider {
                    Layout.fillWidth: true
                    Layout.maximumHeight: visible ? implicitHeight : 0
                    text: object.fullName
                    visible: object.fullName.toLowerCase().includes(searchText.text.toLowerCase())

                    Binding on checked {
                        value: object.enabled
                    }

                    onClicked: object.enabled = checked
                }
            }

            QGCLabel {
                property bool _hasSearchResults: {
                    var needle = searchText.text.toLowerCase();
                    for (var i = 0; i < searchRepeater.count; i++) {
                        if (searchRepeater.itemAt(i) && searchRepeater.itemAt(i).visible)
                            return true;
                    }
                    return false;
                }

                color: qgcPal.colorGrey
                text: qsTr("No matching categories")
                visible: searchText.text !== "" && !_hasSearchResults
            }
        }
    }
}
