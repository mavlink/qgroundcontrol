import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0

Rectangle {

    property var palette: QGCPalette { colorGroup: QGCPalette.Active }
    color: palette.window

    Column {
        spacing: 10

        Grid {
            columns: 4
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
                    width: 120
                    height: 20
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    color: palette.windowText
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
                color: palette.windowText
                horizontalAlignment: Text.AlignHCenter
                text: "Disabled"
            }
            Text {
                width: 80
                height: 20
                color: palette.windowText
                horizontalAlignment: Text.AlignHCenter
                text: "Active"
            }
            Text {
                width: 80
                height: 20
                color: palette.windowText
                horizontalAlignment: Text.AlignHCenter
                text: "Inactive"
            }

            Loader {
                sourceComponent: rowHeader
                property var text: "alternateBase"
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Disabled }
                property var color: palette.alternateBase
                sourceComponent: colorSquare
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Active }
                property var color: palette.alternateBase
                sourceComponent: colorSquare
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Inactive }
                property var color: palette.alternateBase
                sourceComponent: colorSquare
            }

            Loader {
                sourceComponent: rowHeader
                property var text: "base"
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Disabled }
                property var color: palette.base
                sourceComponent: colorSquare
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Active }
                property var color: palette.base
                sourceComponent: colorSquare
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Inactive }
                property var color: palette.base
                sourceComponent: colorSquare
            }

            Loader {
                sourceComponent: rowHeader
                property var text: "button"
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Disabled }
                property var color: palette.button
                sourceComponent: colorSquare
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Active }
                property var color: palette.button
                sourceComponent: colorSquare
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Inactive }
                property var color: palette.button
                sourceComponent: colorSquare
            }

            Loader {
                sourceComponent: rowHeader
                property var text: "buttonHighlight"
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Disabled }
                property var color: palette.buttonHighlight
                sourceComponent: colorSquare
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Active }
                property var color: palette.buttonHighlight
                sourceComponent: colorSquare
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Inactive }
                property var color: palette.buttonHighlight
                sourceComponent: colorSquare
            }

            Loader {
                sourceComponent: rowHeader
                property var text: "buttonText"
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Disabled }
                property var color: palette.buttonText
                sourceComponent: colorSquare
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Active }
                property var color: palette.buttonText
                sourceComponent: colorSquare
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Inactive }
                property var color: palette.buttonText
                sourceComponent: colorSquare
            }

            Loader {
                sourceComponent: rowHeader
                property var text: "text"
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Disabled }
                property var color: palette.text
                sourceComponent: colorSquare
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Active }
                property var color: palette.text
                sourceComponent: colorSquare
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Inactive }
                property var color: palette.text
                sourceComponent: colorSquare
            }

            Loader {
                sourceComponent: rowHeader
                property var text: "window"
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Disabled }
                property var color: palette.window
                sourceComponent: colorSquare
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Active }
                property var color: palette.window
                sourceComponent: colorSquare
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Inactive }
                property var color: palette.window
                sourceComponent: colorSquare
            }

            Loader {
                sourceComponent: rowHeader
                property var text: "windowShade"
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Disabled }
                property var color: palette.windowShade
                sourceComponent: colorSquare
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Active }
                property var color: palette.windowShade
                sourceComponent: colorSquare
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Inactive }
                property var color: palette.windowShade
                sourceComponent: colorSquare
            }

            Loader {
                sourceComponent: rowHeader
                property var text: "windowShadeDark"
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Disabled }
                property var color: palette.windowShadeDark
                sourceComponent: colorSquare
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Active }
                property var color: palette.windowShadeDark
                sourceComponent: colorSquare
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Inactive }
                property var color: palette.windowShadeDark
                sourceComponent: colorSquare
            }

            Loader {
                sourceComponent: rowHeader
                property var text: "windowText"
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Disabled }
                property var color: palette.windowText
                sourceComponent: colorSquare
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Active }
                property var color: palette.windowText
                sourceComponent: colorSquare
            }
            Loader {
                property var palette: QGCPalette { colorGroup: QGCPalette.Inactive }
                property var color: palette.windowText
                sourceComponent: colorSquare
            }

        }

        Item {
            width: parent.width
            height: 30
        }

        Grid {
            columns: 3
            spacing: 5

            Component {
                id: ctlRowHeader

                Text {
                    width: 120
                    height: 20
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    color: palette.windowText
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
                color: palette.windowText
                horizontalAlignment: Text.AlignHCenter
                text: "Enabled"
            }
            Text {
                width: 100
                height: 20
                color: palette.windowText
                horizontalAlignment: Text.AlignHCenter
                text: "Disabled"
            }

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

        }
    }
}
