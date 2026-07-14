pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGCStyle

FocusScope {
    id: root

    readonly property real margin: StyleMetrics.contentMargin
    readonly property bool mirroredLayout: LayoutMirroring.enabled
    readonly property bool narrowLayout: width < StyleMetrics.controlWidth * 6
    readonly property int uiScalePercent: Math.round(StyleTypography.scaleFactor * 100)
    readonly property color validationColor: qgcPalette.window.hslLightness < 0.5 ? "#f32836" : "#b52b2b"

    implicitHeight: contentLayout.implicitHeight + margin * 2
    implicitWidth: contentLayout.implicitWidth + margin * 2

    SystemPalette {
        id: qgcPalette

        colorGroup: SystemPalette.Active
    }

    FontInfo {
        id: bodyFontInfo

        font: StyleTypography.bodyFont
    }

    FontInfo {
        id: fixedFontInfo

        font: StyleTypography.fixedFont
    }

    Rectangle {
        anchors.fill: parent
        border.color: qgcPalette.mid
        border.width: 1
        color: qgcPalette.window
        radius: StyleMetrics.panelRadius
    }

    ColumnLayout {
        id: contentLayout

        objectName: "contentLayout"
        anchors.fill: parent
        anchors.margins: root.margin
        spacing: StyleMetrics.spacing

        Label {
            color: qgcPalette.text
            font: StyleTypography.titleFont
            text: qsTr("QGCStyle state gallery")
        }

        Label {
            Layout.fillWidth: true
            color: qgcPalette.text
            font: StyleTypography.bodyFont
            text: qsTr("Switch themes, adjust accessibility preferences, then exercise each control state.")
            wrapMode: Text.Wrap
        }

        GridLayout {
            objectName: "preferenceLayout"
            Layout.fillWidth: true
            columns: root.narrowLayout ? 1 : 4
            columnSpacing: StyleMetrics.spacing
            rowSpacing: StyleMetrics.spacing

            ComboBox {
                objectName: "densityComboBox"
                Layout.fillWidth: root.narrowLayout
                currentIndex: indexOfValue(StylePreferences.touchModeOverride)
                textRole: "text"
                valueRole: "value"
                model: [
                    { text: qsTr("Automatic density"), value: StylePreferences.Automatic },
                    { text: qsTr("Pointer density"), value: StylePreferences.Pointer },
                    { text: qsTr("Touch density"), value: StylePreferences.Touch }
                ]

                onActivated: StylePreferences.setTouchModePreference(currentValue)
            }

            Switch {
                objectName: "reducedMotionSwitch"
                Layout.fillWidth: root.narrowLayout
                checked: StylePreferences.reducedMotion
                text: qsTr("Reduced motion")

                onToggled: StylePreferences.setReducedMotion(checked)
            }

            Switch {
                Layout.fillWidth: root.narrowLayout
                checked: StylePreferences.forceHighContrast
                text: qsTr("High contrast")

                onToggled: StylePreferences.setForceHighContrast(checked)
            }

            Label {
                color: qgcPalette.text
                text: qsTr("UI scale: %1%").arg(root.uiScalePercent)
            }
        }

        Flow {
            Layout.fillWidth: true
            spacing: StyleMetrics.spacing

            Label {
                color: qgcPalette.text
                font: StyleTypography.captionFont
                text: qsTr("Caption")
            }

            Label {
                color: qgcPalette.text
                font: StyleTypography.bodyFont
                text: qsTr("Body")
            }

            Label {
                color: qgcPalette.text
                font: StyleTypography.headingFont
                text: qsTr("Heading")
            }

            Label {
                color: qgcPalette.text
                font: StyleTypography.titleFont
                text: qsTr("Title")
            }
        }

        Label {
            Layout.fillWidth: true
            color: qgcPalette.text
            font: StyleTypography.captionFont
            text: qsTr("Resolved fonts: %1; fixed: %2 (%3)")
                    .arg(bodyFontInfo.family)
                    .arg(fixedFontInfo.family)
                    .arg(fixedFontInfo.fixedPitch ? qsTr("fixed pitch") : qsTr("proportional"))
            wrapMode: Text.Wrap
        }

        GridLayout {
            objectName: "controlLayout"
            Layout.fillWidth: true
            columnSpacing: StyleMetrics.contentMargin
            columns: root.narrowLayout ? 2 : 4
            rowSpacing: StyleMetrics.spacing

            Label {
                color: qgcPalette.text
                text: qsTr("Default")
            }

            Button {
                text: down ? qsTr("Pressed") : qsTr("Button")
            }

            Label {
                color: qgcPalette.text
                text: qsTr("Disabled")
            }

            Button {
                enabled: false
                text: qsTr("Button")
            }

            Label {
                color: qgcPalette.text
                text: qsTr("Keyboard focus")
            }

            Button {
                text: qsTr("Focused")

                Component.onCompleted: forceActiveFocus(Qt.TabFocusReason)
            }

            Label {
                color: qgcPalette.text
                text: qsTr("Checked")
            }

            Button {
                checkable: true
                checked: true
                text: qsTr("Checked")
            }

            Label {
                color: qgcPalette.text
                text: qsTr("Large font")
            }

            Button {
                font: StyleTypography.titleFont
                text: qsTr("Large")
            }

            Label {
                color: qgcPalette.text
                text: qsTr("Long text")
            }

            Button {
                Layout.fillWidth: root.narrowLayout
                Layout.preferredWidth: root.narrowLayout ? 0 : StyleMetrics.controlWidth * 2
                text: qsTr("A deliberately long translated control label used to verify eliding and layout behavior")
            }

            Label {
                color: qgcPalette.text
                text: qsTr("Current scale")
            }

            Label {
                color: qgcPalette.text
                text: qsTr("%1% UI scale").arg(root.uiScalePercent)
            }

            Label {
                color: qgcPalette.text
                text: qsTr("Right-to-left")
            }

            Flow {
                Layout.fillWidth: root.narrowLayout
                LayoutMirroring.childrenInherit: true
                LayoutMirroring.enabled: true
                spacing: StyleMetrics.spacing

                Button {
                    icon.source: "/qmlimages/Gears.svg"
                    text: qsTr("RTL")
                }

                Button {
                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.text: text
                    ToolTip.visible: hovered || activeFocus
                    display: AbstractButton.IconOnly
                    icon.source: "/qmlimages/Gears.svg"
                    text: qsTr("Settings")
                }
            }

            Label {
                color: qgcPalette.text
                text: qsTr("Inputs")
            }

            Flow {
                Layout.fillWidth: root.narrowLayout
                spacing: StyleMetrics.spacing

                TextField {
                    placeholderText: qsTr("Text field")
                }

                ComboBox {
                    model: [qsTr("Automatic"), qsTr("Manual"), qsTr("Disabled")]
                }
            }

            Label {
                color: qgcPalette.text
                text: qsTr("Validation error")
            }

            TextField {
                palette.mid: root.validationColor
                text: qsTr("Invalid value")
            }

            Label {
                color: qgcPalette.text
                text: qsTr("Read only")
            }

            TextField {
                readOnly: true
                text: qsTr("Read-only value")
            }

            Label {
                color: qgcPalette.text
                text: qsTr("Multiline")
            }

            TextArea {
                Layout.fillWidth: true
                placeholderText: qsTr("Enter notes")
                text: qsTr("Styled text area")
                wrapMode: TextEdit.Wrap
            }

            Label {
                color: qgcPalette.text
                text: qsTr("Selection")
            }

            Flow {
                Layout.fillWidth: root.narrowLayout
                spacing: StyleMetrics.spacing

                CheckBox {
                    checked: true
                    text: qsTr("Check box")
                }

                RadioButton {
                    checked: true
                    text: qsTr("Radio")
                }

                Switch {
                    checked: true
                    text: qsTr("Switch")
                }

                Slider {
                    from: 0
                    to: 100
                    value: 65
                }
            }

            Label {
                color: qgcPalette.text
                text: qsTr("Menu")
            }

            Button {
                text: qsTr("Open menu")

                onClicked: stateMenu.popup()

                Menu {
                    id: stateMenu

                    MenuItem {
                        checkable: true
                        checked: true
                        text: qsTr("Checked item")
                    }

                    MenuItem {
                        text: qsTr("Regular item")
                    }

                    MenuSeparator {
                    }

                    MenuItem {
                        enabled: false
                        text: qsTr("Disabled item")
                    }

                    Menu {
                        title: qsTr("Submenu")

                        MenuItem {
                            text: qsTr("Nested item")
                        }
                    }
                }
            }

            Label {
                color: qgcPalette.text
                text: qsTr("Overlays")
            }

            Flow {
                Layout.fillWidth: root.narrowLayout
                spacing: StyleMetrics.spacing

                Button {
                    text: qsTr("Dialog")

                    onClicked: sampleDialog.open()
                }

                Button {
                    text: qsTr("Popup")

                    onClicked: samplePopup.open()
                }

                Button {
                    text: qsTr("Drawer")

                    onClicked: sampleDrawer.open()
                }
            }

            Label {
                color: qgcPalette.text
                text: qsTr("Progress")
            }

            ProgressBar {
                Layout.fillWidth: true
                value: 0.65
            }

            Label {
                color: qgcPalette.text
                text: qsTr("Indeterminate")
            }

            ProgressBar {
                Layout.fillWidth: true
                indeterminate: true
            }

            Label {
                color: qgcPalette.text
                text: qsTr("Activity")
            }

            Flow {
                Layout.fillWidth: root.narrowLayout
                spacing: StyleMetrics.spacing

                BusyIndicator {
                    running: true
                }

                BusyIndicator {
                    enabled: false
                    running: true
                }

                BusyIndicator {
                    running: false
                }
            }

            Label {
                color: qgcPalette.text
                text: qsTr("Scroll bar")
            }

            ScrollView {
                Layout.fillWidth: true
                implicitHeight: StyleMetrics.controlHeight * 2.5

                Column {
                    width: parent.width

                    Repeater {
                        model: 6

                        ItemDelegate {
                            required property int index

                            text: qsTr("Scrollable item %1").arg(index + 1)
                            width: parent.width
                        }
                    }
                }
            }

            Label {
                color: qgcPalette.text
                text: qsTr("Scroll indicator")
            }

            Flickable {
                id: indicatorFlickable

                Layout.fillWidth: true
                clip: true
                contentHeight: indicatorColumn.implicitHeight
                implicitHeight: StyleMetrics.controlHeight * 2.5

                ScrollIndicator.vertical: ScrollIndicator {
                }

                Column {
                    id: indicatorColumn

                    width: indicatorFlickable.width

                    Repeater {
                        model: 6

                        Label {
                            required property int index

                            color: qgcPalette.text
                            padding: StyleMetrics.smallSpacing
                            text: qsTr("Indicator row %1").arg(index + 1)
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: sampleDialog

        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        title: qsTr("Styled dialog")

        Label {
            text: qsTr("Dialogs share the QGCStyle palette and spacing.")
        }
    }

    Popup {
        id: samplePopup

        x: Math.round((root.width - width) * 0.5)
        y: Math.round((root.height - height) * 0.5)

        Label {
            text: qsTr("Styled popup content")
        }
    }

    Drawer {
        id: sampleDrawer

        height: root.height
        width: Math.min(root.width * 0.4, StyleMetrics.controlWidth * 2)

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: StyleMetrics.dialogPadding

            Label {
                font.bold: true
                text: qsTr("Styled drawer")
            }

            Item {
                Layout.fillHeight: true
            }

            Button {
                Layout.fillWidth: true
                text: qsTr("Close")

                onClicked: sampleDrawer.close()
            }
        }
    }
}
