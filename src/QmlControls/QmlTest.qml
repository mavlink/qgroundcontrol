import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0

Rectangle {

    property var palette: QGCPalette { colorGroupEnabled: true }
    color: "white"

    Column {

        Rectangle {
            width:  parent.width
            height: themeChoice.height * 2
            color:  palette.window
            QGCLabel {
                text: qsTr("Window Color")
                anchors.left:           parent.left
                anchors.leftMargin:     20
                anchors.verticalCenter: parent.horizontalCenter
            }
            Row {
                id: themeChoice
                anchors.centerIn: parent
                anchors.margins: 20
                spacing:         20
                ExclusiveGroup { id: themeGroup }
                QGCRadioButton {
                    text: qsTr("Light")
                    checked: palette.globalTheme === QGCPalette.Light
                    exclusiveGroup: themeGroup
                    onClicked: { palette.globalTheme = QGCPalette.Light }
                }
                QGCRadioButton {
                    text: qsTr("Dark")
                    checked: palette.globalTheme === QGCPalette.Dark
                    exclusiveGroup: themeGroup
                    onClicked: { palette.globalTheme = QGCPalette.Dark }
                }
            }
        }

        Row {
            spacing: 30

            Grid {
                columns: 5
                spacing: 5

                Component {
                    id: rowHeader

                    Text {
                        width: 180
                        height: 20
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter
                        color: "black"
                        text: parent.text
                    }
                }

                // Header row
                Loader {
                    sourceComponent: rowHeader
                    property var text: ""
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("Disabled")
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("Enabled")
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("Value")
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("Value")
                }

                // window
                Loader {
                    sourceComponent: rowHeader
                    property var text: "window"
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    color: palette.window
                    onColorSelected: palette.window = color
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    color: palette.window
                    onColorSelected: palette.window = color
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    text: palette.window
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    text: palette.window
                }

                // windowShade
                Loader {
                    sourceComponent: rowHeader
                    property var text: "windowShade"
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    color: palette.windowShade
                    onColorSelected: palette.windowShade = color
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    color: palette.windowShade
                    onColorSelected: palette.windowShade = color
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    text: palette.windowShade
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    text: palette.windowShade
                }

                // windowShadeDark
                Loader {
                    sourceComponent: rowHeader
                    property var text: "windowShadeDark"
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    color: palette.windowShadeDark
                    onColorSelected: palette.windowShadeDark = color
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    color: palette.windowShadeDark
                    onColorSelected: palette.windowShadeDark = color
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    text: palette.windowShadeDark
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    text: palette.windowShadeDark
                }

                // text
                Loader {
                    sourceComponent: rowHeader
                    property var text: "text"
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    color: palette.text
                    onColorSelected: palette.text = color
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    color: palette.text
                    onColorSelected: palette.text = color
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    text: palette.text
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    text: palette.text
                }

                // button
                Loader {
                    sourceComponent: rowHeader
                    property var text: "button"
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    color: palette.button
                    onColorSelected: palette.button = color
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    color: palette.button
                    onColorSelected: palette.button = color
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    text: palette.button
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    text: palette.button
                }

                // buttonText
                Loader {
                    sourceComponent: rowHeader
                    property var text: "buttonText"
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    color: palette.buttonText
                    onColorSelected: palette.buttonText = color
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    color: palette.buttonText
                    onColorSelected: palette.buttonText = color
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    text: palette.buttonText
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    text: palette.buttonText
                }

                // buttonHighlight
                Loader {
                    sourceComponent: rowHeader
                    property var text: "buttonHighlight"
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    color: palette.buttonHighlight
                    onColorSelected: palette.buttonHighlight = color
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    color: palette.buttonHighlight
                    onColorSelected: palette.buttonHighlight = color
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    text: palette.buttonHighlight
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    text: palette.buttonHighlight
                }

                // buttonHighlightText
                Loader {
                    sourceComponent: rowHeader
                    property var text: "buttonHighlightText"
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    color: palette.buttonHighlightText
                    onColorSelected: palette.buttonHighlightText = color
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    color: palette.buttonHighlightText
                    onColorSelected: palette.buttonHighlightText = color
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    text: palette.buttonHighlightText
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    text: palette.buttonHighlightText
                }

                // primaryButton
                Loader {
                    sourceComponent: rowHeader
                    property var text: "primaryButton"
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    color: palette.primaryButton
                    onColorSelected: palette.primaryButton = color
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    color: palette.primaryButton
                    onColorSelected: palette.primaryButton = color
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    text: palette.primaryButton
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    text: palette.primaryButton
                }

                // primaryButtonText
                Loader {
                    sourceComponent: rowHeader
                    property var text: "primaryButtonText"
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    color: palette.primaryButtonText
                    onColorSelected: palette.primaryButtonText = color
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    color: palette.primaryButtonText
                    onColorSelected: palette.primaryButtonText = color
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    text: palette.primaryButtonText
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    text: palette.primaryButtonText
                }

                // textField
                Loader {
                    sourceComponent: rowHeader
                    property var text: "textField"
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    color: palette.textField
                    onColorSelected: palette.textField = color
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    color: palette.textField
                    onColorSelected: palette.textField = color
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    text: palette.textField
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    text: palette.textField
                }

                // textFieldText
                Loader {
                    sourceComponent: rowHeader
                    property var text: "textFieldText"
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    color: palette.textFieldText
                    onColorSelected: palette.textFieldText = color
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    color: palette.textFieldText
                    onColorSelected: palette.textFieldText = color
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    text: palette.textFieldText
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    text: palette.textFieldText
                }

                // warningText
                Loader {
                    sourceComponent: rowHeader
                    property var text: "warningText"
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    color: palette.warningText
                    onColorSelected: palette.warningText = color
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    color: palette.warningText
                    onColorSelected: palette.warningText = color
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    text: palette.warningText
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    text: palette.warningText
                }
            }

            Grid {
                columns: 3
                spacing: 10

                Component {
                    id: ctlRowHeader

                    Text {
                        width: 120
                        height: 20
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter
                        color: "black"
                        text: parent.text
                    }
                }


                // Header row
                Loader {
                    sourceComponent: ctlRowHeader
                    property var text: ""
                }
                Text {
                    width: 100
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("Enabled")
                }
                Text {
                    width: 100
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("Disabled")
                }

                // QGCLabel
                Loader {
                    sourceComponent: ctlRowHeader
                    property var text: "QGCLabel"
                }
                QGCLabel {
                    width: 100
                    height: 20
                    text: qsTr("Label")
                }
                QGCLabel {
                    width: 100
                    height: 20
                    text: qsTr("Label")
                    enabled: false
                }

                // QGCButton
                Loader {
                    sourceComponent: ctlRowHeader
                    property var text: "QGCButton"
                }
                QGCButton {
                    width: 100
                    height: 20
                    text: qsTr("Button")
                }
                QGCButton {
                    width: 100
                    height: 20
                    text: qsTr("Button")
                    enabled: false
                }

                // QGCButton - primary
                Loader {
                    sourceComponent: ctlRowHeader
                    property var text: "QGCButton(primary)"
                }
                QGCButton {
                    width: 100
                    height: 20
                    primary: true
                    text: qsTr("Button")
                }
                QGCButton {
                    width: 100
                    height: 20
                    text: qsTr("Button")
                    primary: true
                    enabled: false
                }

                // QGCButton - menu
                Loader {
                    sourceComponent: ctlRowHeader
                    property var text: "QGCButton(menu)"
                }
                Menu {
                    id: buttonMenu
                    MenuItem {
                        text: qsTr("Item 1")
                    }
                    MenuItem {
                        text: qsTr("Item 2")
                    }
                    MenuItem {
                        text: qsTr("Item 3")
                    }
                }
                QGCButton {
                    width: 100
                    height: 20
                    text: qsTr("Button")
                    menu: buttonMenu
                }
                QGCButton {
                    width: 100
                    height: 20
                    text: qsTr("Button")
                    enabled: false
                    menu: buttonMenu
                }

                // QGCRadioButton
                Loader {
                    sourceComponent: ctlRowHeader
                    property var text: "QGCRadioButton"
                }
                QGCRadioButton {
                    width: 100
                    height: 20
                    text: qsTr("Radio")
                }
                QGCRadioButton {
                    width: 100
                    height: 20
                    text: qsTr("Radio")
                    enabled: false
                }

                // QGCCheckBox
                Loader {
                    sourceComponent: ctlRowHeader
                    property var text: "QGCCheckBox"
                }
                QGCCheckBox {
                    width: 100
                    height: 20
                    text: qsTr("Check Box")
                }
                QGCCheckBox {
                    width: 100
                    height: 20
                    text: qsTr("Check Box")
                    enabled: false
                }

                // QGCTextField
                Loader {
                    sourceComponent: ctlRowHeader
                    property var text: "QGCTextField"
                }
                QGCTextField {
                    width: 100
                    height: 20
                    text: "QGCTextField"
                }
                QGCTextField {
                    width: 100
                    height: 20
                    text: "QGCTextField"
                    enabled: false
                }

                // QGCComboBox
                Loader {
                    sourceComponent: ctlRowHeader
                    property var text: "QGCComboBox"
                }
                QGCComboBox {
                    width: 100
                    height: 20
                    model: [ qsTr("Item 1"), qsTr("Item 2"), qsTr("Item 3") ]
                }
                QGCComboBox {
                    width: 100
                    height: 20
                    model: [ qsTr("Item 1"), qsTr("Item 2"), qsTr("Item 3") ]
                    enabled: false
                }

                // SubMenuButton
                Loader {
                    sourceComponent: ctlRowHeader
                    property var text: "SubMenuButton"
                }
                SubMenuButton {
                    width: 100
                    height: 100
                    text: qsTr("SUB MENU")
                }
                SubMenuButton {
                    width: 100
                    height: 100
                    text: qsTr("SUB MENU")
                    enabled: false
                }
            }
        }
    }
}
