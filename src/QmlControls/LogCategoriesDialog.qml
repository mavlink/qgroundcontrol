import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

QGCPopupDialog {
    title:      qsTr("Logging Categories")
    buttons:    Dialog.Close

    ColumnLayout {
        width: maxContentAvailableWidth

        SettingsGroupLayout {
            heading:            qsTr("Search")
            Layout.fillWidth:   true

            RowLayout {
                Layout.fillWidth:   true
                spacing:            ScreenTools.defaultFontPixelHeight / 2

                QGCTextField {
                    Layout.fillWidth:   true
                    id:                 searchText
                    text:               ""
                    enabled:            true
                }

                QGCButton {
                    text:       qsTr("Clear")
                    onClicked:  searchText.text = ""
                }
            }
        }

        SettingsGroupLayout {
            heading:            qsTr("Enabled Categories")
            Layout.fillWidth:   true

            Flow {
                Layout.fillWidth:   true
                spacing:            ScreenTools.defaultFontPixelHeight / 2

                Repeater {
                    model: QGroundControl.flatLoggingCategoriesModel()

                    QGCCheckBoxSlider {
                        Layout.fillWidth:       true
                        Layout.maximumHeight:   visible ? implicitHeight : 0
                        text:                   object.fullName
                        visible:                object.enabled
                        checked:                object.enabled
                        onClicked:              object.enabled = checked
                    }
                }

                QGCButton {
                    text:       qsTr("Disable All")
                    onClicked:  QGroundControl.disableAllLoggingCategories()
                }
            }
        }

        // Shown when not filtered
        Flow {
            Layout.fillWidth:   true
            spacing:            ScreenTools.defaultFontPixelHeight / 2
            visible:            searchText.text === ""

            Repeater {
                model: QGroundControl.treeLoggingCategoriesModel()

                ColumnLayout {
                    spacing: ScreenTools.defaultFontPixelHeight / 2

                    RowLayout {
                        spacing: ScreenTools.defaultFontPixelWidth

                        QGCLabel {
                            Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth
                            text:                   object.expanded ? qsTr("-") : qsTr("+")
                            horizontalAlignment:    Text.AlignLeft
                            visible:                object.children

                            QGCMouseArea {
                                anchors.fill:   parent
                                onClicked:      object.expanded = !object.expanded
                            }
                        }

                        QGCCheckBoxSlider {
                            Layout.fillWidth:   true
                            text:               object.shortName
                            checked:            object.enabled
                            onClicked:          object.enabled = checked
                        }
                    }

                    Repeater {
                        model: object.expanded ? object.children : undefined

                        QGCCheckBoxSlider {
                            Layout.fillWidth:   true
                            text:               "   " + object.shortName
                            checked:            object.enabled
                            onClicked:          object.enabled = checked
                        }
                    }
                }
            }
        }

        // Shown when filtered
        Flow {
            Layout.fillWidth:   true
            spacing:            ScreenTools.defaultFontPixelHeight / 2
            visible:            searchText.text !== ""

            Repeater {
                model: QGroundControl.flatLoggingCategoriesModel()

                QGCCheckBoxSlider {
                    Layout.fillWidth:       true
                    Layout.maximumHeight:   visible ? implicitHeight : 0
                    text:                   object.fullName
                    visible:                text.match(`(${searchText.text})`, "i")
                    checked:                object.enabled
                    onClicked:              object.enabled = checked
                }
            }
        }
    }
}
