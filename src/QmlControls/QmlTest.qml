import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts          1.2

import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

Rectangle {


    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    property var palette: QGCPalette { colorGroupEnabled: true }
    color: "white"

    Component {
        id: arbBox
        Rectangle {
            width:  arbGrid.width  * 1.5
            height: arbGrid.height * 1.5
            color:  backgroundColor
            border.color: qgcPal.text
            border.width: 1
            anchors.horizontalCenter: parent.horizontalCenter
            GridLayout {
                id: arbGrid
                columns: 4
                rowSpacing: 10
                anchors.centerIn: parent
                QGCColoredImage {
                    color:                      qgcPal.colorGreen
                    width:                      ScreenTools.defaultFontPixelWidth * 2
                    height:                     width
                    sourceSize.height:          width
                    mipmap:                     true
                    fillMode:                   Image.PreserveAspectFit
                    source:                     "/qmlimages/Gears.svg"
                }
                Label { text: "colorGreen"; color: qgcPal.colorGreen; }
                QGCColoredImage {
                    color:                      qgcPal.colorOrange
                    width:                      ScreenTools.defaultFontPixelWidth * 2
                    height:                     width
                    sourceSize.height:          width
                    mipmap:                     true
                    fillMode:                   Image.PreserveAspectFit
                    source:                     "/qmlimages/Gears.svg"
                }
                Label { text: "colorOrange"; color: qgcPal.colorOrange; }
                QGCColoredImage {
                    color:                      qgcPal.colorRed
                    width:                      ScreenTools.defaultFontPixelWidth * 2
                    height:                     width
                    sourceSize.height:          width
                    mipmap:                     true
                    fillMode:                   Image.PreserveAspectFit
                    source:                     "/qmlimages/Gears.svg"
                }
                Label { text: "colorRed"; color: qgcPal.colorRed; }
                QGCColoredImage {
                    color:                      qgcPal.colorGrey
                    width:                      ScreenTools.defaultFontPixelWidth * 2
                    height:                     width
                    sourceSize.height:          width
                    mipmap:                     true
                    fillMode:                   Image.PreserveAspectFit
                    source:                     "/qmlimages/Gears.svg"
                }
                Label { text: "colorGrey"; color: qgcPal.colorGrey;  }
                QGCColoredImage {
                    color:                      qgcPal.colorBlue
                    width:                      ScreenTools.defaultFontPixelWidth * 2
                    height:                     width
                    sourceSize.height:          width
                    mipmap:                     true
                    fillMode:                   Image.PreserveAspectFit
                    source:                     "/qmlimages/Gears.svg"
                }
                Label { text: "colorBlue"; color: qgcPal.colorBlue; }
            }
        }
    }

    Column {

        Rectangle {
            width:  parent.width
            height: themeChoice.height * 2
            color:  palette.window
            QGCLabel {
                text: qsTr("Window Color")
                anchors.left:           parent.left
                anchors.leftMargin:     20
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
                    property string text: ""
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
                    property string text: "window"
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
                    property string text: "windowShade"
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
                    property string text: "windowShadeDark"
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
                    property string text: "text"
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
                    property string text: "button"
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
                    property string text: "buttonText"
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
                    property string text: "buttonHighlight"
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
                    property string text: "buttonHighlightText"
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
                    property string text: "primaryButton"
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
                    property string text: "primaryButtonText"
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
                    property string text: "textField"
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
                    property string text: "textFieldText"
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
                    property string text: "warningText"
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

                // colorGreen
                Loader {
                    sourceComponent: rowHeader
                    property string text: "colorGreen"
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    color: palette.colorGreen
                    onColorSelected: palette.colorGreen = color
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    color: palette.colorGreen
                    onColorSelected: palette.colorGreen = color
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    text: palette.colorGreen
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    text: palette.colorGreen
                }

                // colorOrange
                Loader {
                    sourceComponent: rowHeader
                    property string text: "colorOrange"
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    color: palette.colorOrange
                    onColorSelected: palette.colorOrange = color
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    color: palette.colorOrange
                    onColorSelected: palette.colorOrange = color
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    text: palette.colorOrange
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    text: palette.colorOrange
                }

                // colorRed
                Loader {
                    sourceComponent: rowHeader
                    property string text: "colorRed"
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    color: palette.colorRed
                    onColorSelected: palette.colorRed = color
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    color: palette.colorRed
                    onColorSelected: palette.colorRed = color
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    text: palette.colorRed
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    text: palette.colorRed
                }

                // colorGrey
                Loader {
                    sourceComponent: rowHeader
                    property string text: "colorGrey"
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    color: palette.colorGrey
                    onColorSelected: palette.colorGrey = color
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    color: palette.colorGrey
                    onColorSelected: palette.colorGrey = color
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    text: palette.colorGrey
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    text: palette.colorGrey
                }

                // colorBlue
                Loader {
                    sourceComponent: rowHeader
                    property string text: "colorBlue"
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    color: palette.colorBlue
                    onColorSelected: palette.colorBlue = color
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    color: palette.colorBlue
                    onColorSelected: palette.colorBlue = color
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    text: palette.colorBlue
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    text: palette.colorBlue
                }

                // alertBackground
                Loader {
                    sourceComponent: rowHeader
                    property string text: "alertBackground"
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    color: palette.alertBackground
                    onColorSelected: palette.alertBackground = color
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    color: palette.alertBackground
                    onColorSelected: palette.alertBackground = color
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    text: palette.alertBackground
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    text: palette.alertBackground
                }

                // alertBorder
                Loader {
                    sourceComponent: rowHeader
                    property string text: "alertBorder"
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    color: palette.alertBorder
                    onColorSelected: palette.alertBorder = color
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    color: palette.alertBorder
                    onColorSelected: palette.alertBorder = color
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    text: palette.alertBorder
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    text: palette.alertBorder
                }

                // alertText
                Loader {
                    sourceComponent: rowHeader
                    property string text: "alertText"
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    color: palette.alertText
                    onColorSelected: palette.alertText = color
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    color: palette.alertText
                    onColorSelected: palette.alertText = color
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    text: palette.alertText
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    text: palette.alertText
                }

                // missionItemEditor
                Loader {
                    sourceComponent: rowHeader
                    property var text: "missionItemEditor"
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    color: palette.missionItemEditor
                    onColorSelected: palette.missionItemEditor = color
                }
                ClickableColor {
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    color: palette.missionItemEditor
                    onColorSelected: palette.missionItemEditor = color
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: false }
                    text: palette.missionItemEditor
                }
                Text {
                    width: 80
                    height: 20
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    property var palette: QGCPalette { colorGroupEnabled: true }
                    text: palette.missionItemEditor
                }

            }

            Column {
                spacing: 10
                width: leftGrid.width
                Grid {
                    id: leftGrid
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
                        property string text: ""
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
                        property string text: "QGCLabel"
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
                        property string text: "QGCButton"
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
                        property string text: "QGCButton(primary)"
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
                        property string text: "QGCButton(menu)"
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
                        property string text: "QGCRadioButton"
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
                        property string text: "QGCCheckBox"
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
                        property string text: "QGCTextField"
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
                        property string text: "QGCComboBox"
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
                        property string text: "SubMenuButton"
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
                Rectangle {
                    width:  leftGrid.width
                    height: 60
                    radius: 3
                    color:  palette.alertBackground
                    border.color: palette.alertBorder
                    anchors.horizontalCenter: parent.horizontalCenter
                    Label {
                        text: "Alert Message"
                        color:  palette.alertText
                        anchors.centerIn: parent
                    }
                }
            }
        }

        Item{
            height: 10;
            width:  1;
        }

        Row {
            spacing: 10
            anchors.horizontalCenter: parent.horizontalCenter
            Loader {
                property color backgroundColor: qgcPal.window
                sourceComponent: arbBox
            }
            Loader {
                property color backgroundColor: qgcPal.windowShade
                sourceComponent: arbBox
            }
            Loader {
                property color backgroundColor: qgcPal.windowShadeDark
                sourceComponent: arbBox
            }
        }

    }
}
