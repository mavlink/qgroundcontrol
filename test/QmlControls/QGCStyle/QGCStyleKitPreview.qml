import Qt.labs.StyleKit as Labs
import QtQuick
import QtQuick.Layouts

import QGCStyle

Labs.Pane {
    id: root

    readonly property bool narrowLayout: width < StyleMetrics.controlWidth * 5

    implicitHeight: contentLayout.implicitHeight + topPadding + bottomPadding
    implicitWidth: contentLayout.implicitWidth + leftPadding + rightPadding

    ColumnLayout {
        id: contentLayout

        anchors.fill: parent

        Labs.Label {
            Layout.fillWidth: true
            font.bold: true
            text: qsTr("Qt Labs StyleKit preview")
        }

        GridLayout {
            objectName: "previewControlLayout"
            Layout.fillWidth: true
            columns: root.narrowLayout ? 1 : 5

            Labs.Button {
                Layout.fillWidth: root.narrowLayout
                text: qsTr("Button")
            }

            Labs.CheckBox {
                Layout.fillWidth: root.narrowLayout
                checked: true
                text: qsTr("Check box")
            }

            Labs.Switch {
                Layout.fillWidth: root.narrowLayout
                checked: true
                text: qsTr("Switch")
            }

            Labs.TextField {
                Layout.fillWidth: root.narrowLayout
                placeholderText: qsTr("Text field")
            }

            Labs.ComboBox {
                Layout.fillWidth: root.narrowLayout
                model: [qsTr("Automatic"), qsTr("Manual")]
            }
        }

        GridLayout {
            objectName: "previewProgressLayout"
            Layout.fillWidth: true
            columns: root.narrowLayout ? 1 : 2

            Labs.ProgressBar {
                Layout.fillWidth: true
                value: 0.65
            }

            Labs.Slider {
                Layout.fillWidth: true
                value: 0.65
            }
        }

        GridLayout {
            objectName: "previewVariationLayout"
            Layout.fillWidth: true
            columns: root.narrowLayout ? 1 : 3

            Labs.Label {
                text: qsTr("Variations")
            }

            Labs.Button {
                Layout.fillWidth: root.narrowLayout
                Labs.StyleVariation.variations: ["primary"]
                text: qsTr("Primary")
            }

            Labs.Button {
                Layout.fillWidth: root.narrowLayout
                Labs.StyleVariation.variations: ["compact"]
                text: qsTr("Compact")
            }
        }
    }
}
