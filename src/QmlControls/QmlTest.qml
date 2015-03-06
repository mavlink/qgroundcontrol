import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0

Rectangle {

    property var palette: QGCPalette { colorGroupEnabled: true }
    color: palette.window

    Column {

        Row {
            ExclusiveGroup { id: themeGroup }

            QGCRadioButton {
                text: "Light"
                exclusiveGroup: themeGroup
                onClicked: { palette.globalTheme = QGCPalette.Light }
            }

            QGCRadioButton {
                text: "Dark"
                exclusiveGroup: themeGroup
                onClicked: { palette.globalTheme = QGCPalette.Dark }
            }
        }

        Row {
            spacing: 30

            Grid {
                columns: 3
                spacing: 5

                Component {
                    id: colorSquare

                    Rectangle {
                        width: 80
                        height: 20
                        border.width: 1
                        border.color: "white"
                        color: parent.color
                    }
                }

                Component {
                    id: rowHeader

                    Text {
                        width: 180
                        height: 20
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter
                        color: palette.text
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
                    color: palette.text
                    horizontalAlignment: Text.AlignHCenter
                    text: "Disabled"
                }
                Text {
                    width: 80
                    height: 20
                    color: palette.text
                    horizontalAlignment: Text.AlignHCenter
                    text: "Enabled"
                }

                // window
                Loader {
                    sourceComponent: rowHeader
                    property var text: "window"
                }
                Loader {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    property var color: palette.window
                    sourceComponent: colorSquare
                }
                Loader {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    property var color: palette.window
                    sourceComponent: colorSquare
                }

                // windowShade
                Loader {
                    sourceComponent: rowHeader
                    property var text: "windowShade"
                }
                Loader {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    property var color: palette.windowShade
                    sourceComponent: colorSquare
                }
                Loader {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    property var color: palette.windowShade
                    sourceComponent: colorSquare
                }

                // windowShadeDark
                Loader {
                    sourceComponent: rowHeader
                    property var text: "windowShadeDark"
                }
                Loader {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    property var color: palette.windowShadeDark
                    sourceComponent: colorSquare
                }
                Loader {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    property var color: palette.windowShadeDark
                    sourceComponent: colorSquare
                }

                // text
                Loader {
                    sourceComponent: rowHeader
                    property var text: "text"
                }
                Loader {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    property var color: palette.text
                    sourceComponent: colorSquare
                }
                Loader {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    property var color: palette.text
                    sourceComponent: colorSquare
                }

                // button
                Loader {
                    sourceComponent: rowHeader
                    property var text: "button"
                }
                Loader {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    property var color: palette.button
                    sourceComponent: colorSquare
                }
                Loader {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    property var color: palette.button
                    sourceComponent: colorSquare
                }

                // buttonText
                Loader {
                    sourceComponent: rowHeader
                    property var text: "buttonText"
                }
                Loader {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    property var color: palette.buttonText
                    sourceComponent: colorSquare
                }
                Loader {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    property var color: palette.buttonText
                    sourceComponent: colorSquare
                }

                // buttonHighlight
                Loader {
                    sourceComponent: rowHeader
                    property var text: "buttonHighlight"
                }
                Loader {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    property var color: palette.buttonHighlight
                    sourceComponent: colorSquare
                }
                Loader {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    property var color: palette.buttonHighlight
                    sourceComponent: colorSquare
                }

                // buttonHighlightText
                Loader {
                    sourceComponent: rowHeader
                    property var text: "buttonHighlightText"
                }
                Loader {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    property var color: palette.buttonHighlightText
                    sourceComponent: colorSquare
                }
                Loader {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    property var color: palette.buttonHighlightText
                    sourceComponent: colorSquare
                }

                // primaryButton
                Loader {
                    sourceComponent: rowHeader
                    property var text: "primaryButton"
                }
                Loader {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    property var color: palette.primaryButton
                    sourceComponent: colorSquare
                }
                Loader {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    property var color: palette.primaryButton
                    sourceComponent: colorSquare
                }

                // primaryButtonText
                Loader {
                    sourceComponent: rowHeader
                    property var text: "primaryButtonText"
                }
                Loader {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    property var color: palette.primaryButtonText
                    sourceComponent: colorSquare
                }
                Loader {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    property var color: palette.primaryButtonText
                    sourceComponent: colorSquare
                }

                // textField
                Loader {
                    sourceComponent: rowHeader
                    property var text: "textField"
                }
                Loader {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    property var color: palette.textField
                    sourceComponent: colorSquare
                }
                Loader {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    property var color: palette.textField
                    sourceComponent: colorSquare
                }

                // textFieldText
                Loader {
                    sourceComponent: rowHeader
                    property var text: "textFieldText"
                }
                Loader {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    property var color: palette.textFieldText
                    sourceComponent: colorSquare
                }
                Loader {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    property var color: palette.textFieldText
                    sourceComponent: colorSquare
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
                        color: palette.text
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
                    color: palette.text
                    horizontalAlignment: Text.AlignHCenter
                    text: "Enabled"
                }
                Text {
                    width: 100
                    height: 20
                    color: palette.text
                    horizontalAlignment: Text.AlignHCenter
                    text: "Disabled"
                }

                // QGCLabel
                Loader {
                    sourceComponent: ctlRowHeader
                    property var text: "QGCLabel"
                }
                QGCLabel {
                    width: 100
                    height: 20
                    text: "Label"
                }
                QGCLabel {
                    width: 100
                    height: 20
                    text: "Label"
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
                    text: "Button"
                }
                QGCButton {
                    width: 100
                    height: 20
                    text: "Button"
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
                    text: "Button"
                }
                QGCButton {
                    width: 100
                    height: 20
                    text: "Button"
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
                        text: "Item 1"
                    }
                    MenuItem {
                        text: "Item 2"
                    }
                    MenuItem {
                        text: "Item 3"
                    }
                }
                QGCButton {
                    width: 100
                    height: 20
                    text: "Button"
                    menu: buttonMenu
                }
                QGCButton {
                    width: 100
                    height: 20
                    text: "Button"
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
                    text: "Radio"
                }
                QGCRadioButton {
                    width: 100
                    height: 20
                    text: "Radio"
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
                    text: "Check Box"
                }
                QGCCheckBox {
                    width: 100
                    height: 20
                    text: "Check Box"
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
                    model: [ "Item 1", "Item 2", "Item 3" ]
                }
                QGCComboBox {
                    width: 100
                    height: 20
                    model: [ "Item 1", "Item 2", "Item 3" ]
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
                    text: "SUB MENU"
                }
                SubMenuButton {
                    width: 100
                    height: 100
                    text: "SUB MENU"
                    enabled: false
                }
            }
        }
    }
}
