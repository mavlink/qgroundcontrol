import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

QGCFlickable {
    id: root

    required property var controller

    contentWidth: gridLayout.width
    contentHeight: gridLayout.height

    GridLayout {
        id: gridLayout
        rows: root.controller.model.count + 1
        columns: 5
        flow: GridLayout.TopToBottom
        columnSpacing: ScreenTools.defaultFontPixelWidth
        rowSpacing: 0

        QGCCheckBox { enabled: false }

        Repeater {
            model: root.controller.model

            QGCCheckBox {
                Binding on checkState {
                    value: object.selected ? Qt.Checked : Qt.Unchecked
                }
                onClicked: object.selected = checked
            }
        }

        QGCLabel { text: qsTr("Id") }

        Repeater {
            model: root.controller.model
            QGCLabel { text: object.id }
        }

        QGCLabel { text: qsTr("Date") }

        Repeater {
            model: root.controller.model

            QGCLabel {
                text: {
                    if (!object.received) {
                        return ""
                    }
                    if (object.time.getUTCFullYear() < 2010) {
                        return qsTr("Date Unknown")
                    }
                    return object.time.toLocaleString(undefined)
                }
            }
        }

        QGCLabel { text: qsTr("Size") }

        Repeater {
            model: root.controller.model
            QGCLabel { text: object.sizeStr }
        }

        QGCLabel { text: qsTr("Status") }

        Repeater {
            model: root.controller.model
            QGCLabel { text: object.status }
        }
    }
}
