import QtQuick                  2.11
import QtQuick.Controls         2.4
import QtQuick.Controls.Styles  1.4
import QtQuick.Layouts          1.11

import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

Rectangle {
    id: _root
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth
    color:              "white"

    property var palette:           QGCPalette { colorGroupEnabled: true }
    property var enabledPalette:    QGCPalette { colorGroupEnabled: true }
    property var disabledPalette:   QGCPalette { colorGroupEnabled: false }

    function exportPaletteColors(pal) {
        var objToExport = {}
        for(var clrName in pal) {
            if(pal[clrName].r !== undefined) {
                objToExport[clrName] = pal[clrName].toString();
            }
        }
        return objToExport;
    }

    function fillPalette(pal, colorsObj) {
        for(var clrName in colorsObj) {
            pal[clrName] = colorsObj[clrName];
        }
    }

    function exportTheme() {
        var themeObj = {"light": {}, "dark":{}}
        var oldTheme = palette.globalTheme;

        palette.globalTheme = QGCPalette.Light
        palette.colorGroupEnabled = true
        themeObj.light["enabled"] = exportPaletteColors(palette);
        palette.colorGroupEnabled = false
        themeObj.light["disabled"] = exportPaletteColors(palette);
        palette.globalTheme = QGCPalette.Dark
        palette.colorGroupEnabled = true
        themeObj.dark["enabled"] = exportPaletteColors(palette);
        palette.colorGroupEnabled = false
        themeObj.dark["disabled"] = exportPaletteColors(palette);

        palette.globalTheme = oldTheme;
        palette.colorGroupEnabled = true;

        var jsonString = JSON.stringify(themeObj, null, 4);

        themeImportExportEdit.text = jsonString
    }

    function exportThemeCPP() {
        var palToExport = ""
        for(var i = 0; i < palette.colors.length; i++) {
            var cs = palette.colors[i]
            var csc = cs + 'Colors'
            palToExport += 'DECLARE_QGC_COLOR(' + cs + ', \"' + palette[csc][1] + '\", \"' + palette[csc][0] + '\", \"' + palette[csc][3] + '\", \"' + palette[csc][2] + '\")\n'
        }
        themeImportExportEdit.text = palToExport
    }

    function exportThemePlugin() {
        var palToExport = ""
        for(var i = 0; i < palette.colors.length; i++) {
            var cs = palette.colors[i]
            var csc = cs + 'Colors'
            if(i > 0) {
                palToExport += '\nelse '
            }
            palToExport +=
            'if (colorName == QStringLiteral(\"' + cs + '\")) {\n' +
            '    colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor(\"' + palette[csc][2] + '\");\n' +
            '    colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor(\"' + palette[csc][3] + '\");\n' +
            '    colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor(\"' + palette[csc][0] + '\");\n' +
            '    colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor(\"' + palette[csc][1] + '\");\n' +
            '}'
        }
        themeImportExportEdit.text = palToExport
    }

    function importTheme(jsonStr) {
        var jsonObj = JSON.parse(jsonStr)
        var themeObj = {"light": {}, "dark":{}}
        var oldTheme = palette.globalTheme;

        palette.globalTheme = QGCPalette.Light
        palette.colorGroupEnabled = true
        fillPalette(palette, jsonObj.light.enabled)
        palette.colorGroupEnabled = false
        fillPalette(palette, jsonObj.light.disabled);
        palette.globalTheme = QGCPalette.Dark
        palette.colorGroupEnabled = true
        fillPalette(palette, jsonObj.dark.enabled);
        palette.colorGroupEnabled = false
        fillPalette(palette, jsonObj.dark.disabled);

        palette.globalTheme = oldTheme;
        palette.colorGroupEnabled = true;

        paletteImportExportPopup.close()
    }

    //-------------------------------------------------------------------------
    //-- Export/Import
    Popup {
        id:             paletteImportExportPopup
        width:          impCol.width  + (ScreenTools.defaultFontPixelWidth  * 4)
        height:         impCol.height + (ScreenTools.defaultFontPixelHeight * 2)
        modal:          true
        focus:          true
        parent:         Overlay.overlay
        closePolicy:    Popup.CloseOnEscape | Popup.CloseOnPressOutside
        x:              Math.round((mainWindow.width  - width)  * 0.5)
        y:              Math.round((mainWindow.height - height) * 0.5)
        onVisibleChanged: {
            if(visible) {
                exportTheme()
                _jsonButton.checked = true
            }
        }
        background: Rectangle {
            anchors.fill:   parent
            color:          palette.window
            radius:         ScreenTools.defaultFontPixelHeight * 0.5
            border.width:   1
            border.color:   palette.text
        }
        Column {
            id:             impCol
            spacing:        ScreenTools.defaultFontPixelHeight
            anchors.centerIn: parent
            Row {
                id:         exportFormats
                spacing:    ScreenTools.defaultFontPixelWidth  * 2
                anchors.horizontalCenter: parent.horizontalCenter
                QGCRadioButton {
                    id:     _jsonButton
                    text:   "Json"
                    onClicked: exportTheme()
                }
                QGCRadioButton {
                    text: "QGC"
                    onClicked: exportThemeCPP()
                }
                QGCRadioButton {
                    text: "Custom Plugin"
                    onClicked: exportThemePlugin()
                }
            }
            Rectangle {
                width:              flick.width  + (ScreenTools.defaultFontPixelWidth  * 2)
                height:             flick.height + (ScreenTools.defaultFontPixelHeight * 2)
                color:              "white"
                anchors.margins:    10
                Flickable {
                    id:             flick
                    clip:           true
                    width:          mainWindow.width  * 0.666
                    height:         mainWindow.height * 0.666
                    contentWidth:   themeImportExportEdit.paintedWidth
                    contentHeight:  themeImportExportEdit.paintedHeight
                    anchors.centerIn: parent
                    flickableDirection: Flickable.VerticalFlick

                    function ensureVisible(r)
                    {
                       if (contentX >= r.x)
                           contentX = r.x;
                       else if (contentX+width <= r.x+r.width)
                           contentX = r.x+r.width-width;
                       if (contentY >= r.y)
                           contentY = r.y;
                       else if (contentY+height <= r.y+r.height)
                           contentY = r.y+r.height-height;
                    }

                    TextEdit {
                       id:          themeImportExportEdit
                       width:       flick.width
                       focus:       true
                       font.family: ScreenTools.fixedFontFamily
                       font.pointSize: ScreenTools.defaultFontPointSize
                       onCursorRectangleChanged: flick.ensureVisible(cursorRectangle)
                    }
                }
            }
            Row {
                spacing:    ScreenTools.defaultFontPixelWidth  * 2
                anchors.horizontalCenter: parent.horizontalCenter
                QGCButton {
                    id:         importButton
                    text:       "Import (Json Only)"
                    enabled:    themeImportExportEdit.text[0] === "{" && _jsonButton.checked
                    onClicked: {
                        importTheme(themeImportExportEdit.text);
                    }
                }
                QGCButton {
                    text:       "Close"
                    onClicked: {
                        paletteImportExportPopup.close()
                    }
                }
            }
        }
    }

    //-------------------------------------------------------------------------
    //-- Header
    Rectangle {
        id:         _header
        width:      parent.width
        height:     themeChoice.height * 2
        color:      palette.window
        anchors.top: parent.top
        Row {
            id:         themeChoice
            spacing:    20
            anchors.centerIn: parent
            QGCLabel {
                text:   qsTr("Window Color")
                anchors.verticalCenter: parent.verticalCenter
            }
            QGCButton {
                text:   qsTr("Import/Export")
                anchors.verticalCenter: parent.verticalCenter
                onClicked: paletteImportExportPopup.open()
            }
            Row {
                spacing:         20
                anchors.verticalCenter: parent.verticalCenter
                QGCRadioButton {
                    text:       qsTr("Light")
                    checked:    _root.palette.globalTheme === QGCPalette.Light
                    onClicked: {
                        _root.palette.globalTheme = QGCPalette.Light
                    }
                }
                QGCRadioButton {
                    text:       qsTr("Dark")
                    checked:    _root.palette.globalTheme === QGCPalette.Dark
                    onClicked: {
                        _root.palette.globalTheme = QGCPalette.Dark
                    }
                }
            }
        }
    }
    //-------------------------------------------------------------------------
    //-- Main Contents
    QGCFlickable {
        anchors.top:            _header.bottom
        anchors.bottom:         parent.bottom
        width:                  parent.width
        contentWidth:           _rootCol.width
        contentHeight:          _rootCol.height
        clip:                   true
        Column {
            id:         _rootCol
            Row {
                spacing: 30
                // Edit theme GroupBox
                GroupBox {
                    title: "Preview and edit theme"
                Column {
                    id: editRoot
                    spacing: 5
                    property size cellSize: "90x25"

                    // Header row
                    Row {
                        Text {
                            width: editRoot.cellSize.width * 2
                            height: editRoot.cellSize.height
                            text: ""
                        }
                        Text {
                            width: editRoot.cellSize.width; height: editRoot.cellSize.height
                            color: "black"
                            horizontalAlignment: Text.AlignLeft
                            text: qsTr("Enabled")
                        }
                        Text {
                            width: editRoot.cellSize.width; height: editRoot.cellSize.height
                            color: "black"
                            horizontalAlignment: Text.AlignHCenter
                            text: qsTr("Value")
                        }
                        Text {
                            width: editRoot.cellSize.width; height: editRoot.cellSize.height
                            color: "black"
                            horizontalAlignment: Text.AlignHCenter
                            text: qsTr("Disabled")
                        }
                        Text {
                            width: editRoot.cellSize.width; height: editRoot.cellSize.height
                            color: "black"
                            horizontalAlignment: Text.AlignHCenter
                            text: qsTr("Value")
                        }
                    }

                    // Populate the model with all color names in the global palette
                    Component.onCompleted: {
                        for(var colorNameStr in palette) {
                            if(palette[colorNameStr].r !== undefined) {
                                paletteColorList.append({ colorName: colorNameStr });
                            }
                        }
                    }

                    ListModel {
                        id: paletteColorList
                    }

                    // Reproduce all the models
                    Repeater {
                        model: paletteColorList
                        delegate: Row {
                            spacing: 5
                            Text {
                                width: editRoot.cellSize.width * 2
                                height: editRoot.cellSize.height
                                horizontalAlignment: Text.AlignRight
                                verticalAlignment: Text.AlignVCenter
                                color: "black"
                                text: colorName
                            }
                            ClickableColor {
                                id: enabledColorPicker
                                color: enabledPalette[colorName]
                                onColorSelected: enabledPalette[colorName] = color
                            }
                            TextField {
                                id: enabledTextField
                                width: editRoot.cellSize.width; height: editRoot.cellSize.height
                                inputMask: "\\#>HHHHHHhh;"
                                horizontalAlignment: Text.AlignLeft
                                text: enabledPalette[colorName]
                                onEditingFinished: enabledPalette[colorName] = text
                            }
                            ClickableColor {
                                id: disabledColorPicker
                                color: disabledPalette[colorName]
                                onColorSelected: disabledPalette[colorName] = color
                            }
                            TextField {
                                width: editRoot.cellSize.width; height: editRoot.cellSize.height
                                inputMask: enabledTextField.inputMask
                                horizontalAlignment: Text.AlignLeft
                                text: disabledPalette[colorName]
                                onEditingFinished: disabledPalette[colorName] = text
                            }
                        }
                    }
                } // Column
                } // GroupBox { title: "Preview and edit theme"

                // QGC controls preview
                GroupBox { title: "Controls preview"
                Column {
                    id: ctlPrevColumn
                    property real _colWidth: ScreenTools.defaultFontPointSize * 18
                    property real _height: _colWidth*0.15
                    property color _bkColor: qgcPal.window
                    spacing: 10
                    width: previewGrid.width
                    Grid {
                        id: previewGrid
                        columns: 3
                        spacing: 10

                        // Header row
                        Text {
                            width: ctlPrevColumn._colWidth
                            height: ctlPrevColumn._height
                            color: "black"
                            horizontalAlignment: Text.AlignHCenter
                            text: qsTr("QGC name")
                        }
                        Text {
                            width: ctlPrevColumn._colWidth
                            height: ctlPrevColumn._height
                            color: "black"
                            horizontalAlignment: Text.AlignHCenter
                            text: qsTr("Enabled")
                        }
                        Text {
                            width: ctlPrevColumn._colWidth
                            height: ctlPrevColumn._height
                            color: "black"
                            horizontalAlignment: Text.AlignHCenter
                            text: qsTr("Disabled")
                        }

                        // QGCLabel
                        Loader {
                            sourceComponent: ctlRowHeader
                            property string text: "QGCLabel"
                        }
                        Rectangle {
                            width: ctlPrevColumn._colWidth
                            height: ctlPrevColumn._height
                            color: ctlPrevColumn._bkColor
                            QGCLabel {
                                anchors.fill: parent
                                anchors.margins: 5
                                text: qsTr("Label")
                            }
                        }
                        Rectangle {
                            width: ctlPrevColumn._colWidth
                            height: ctlPrevColumn._height
                            color: ctlPrevColumn._bkColor
                            QGCLabel {
                                anchors.fill: parent
                                anchors.margins: 5
                                text: qsTr("Label")
                                enabled: false
                            }
                        }

                        // QGCButton
                        Loader {
                            sourceComponent: ctlRowHeader
                            property string text: "QGCButton"
                        }
                        QGCButton {
                            width: ctlPrevColumn._colWidth
                            height: ctlPrevColumn._height
                            text: qsTr("Button")
                        }
                        QGCButton {
                            width: ctlPrevColumn._colWidth
                            height: ctlPrevColumn._height
                            text: qsTr("Button")
                            enabled: false
                        }

                        // QGCButton - primary
                        Loader {
                            sourceComponent: ctlRowHeader
                            property string text: "QGCButton(primary)"
                        }
                        QGCButton {
                            width: ctlPrevColumn._colWidth
                            height: ctlPrevColumn._height
                            primary: true
                            text: qsTr("Button")
                        }
                        QGCButton {
                            width:  ctlPrevColumn._colWidth
                            height: ctlPrevColumn._height
                            text:   qsTr("Button")
                            primary: true
                            enabled: false
                        }

                        // ToolStripHoverButton
                        Loader {
                            sourceComponent: ctlRowHeader
                            property string text: "ToolStripHoverButton"
                        }
                        ToolStripHoverButton {
                            width:  ctlPrevColumn._colWidth
                            height: ctlPrevColumn._height * 2
                            text:   qsTr("Hover Button")
                            radius: ScreenTools.defaultFontPointSize
                            imageSource: "/qmlimages/Gears.svg"
                        }
                        ToolStripHoverButton {
                            width:  ctlPrevColumn._colWidth
                            height: ctlPrevColumn._height * 2
                            text:   qsTr("Hover Button")
                            radius: ScreenTools.defaultFontPointSize
                            imageSource: "/qmlimages/Gears.svg"
                            enabled: false
                        }

                        // QGCButton - menu
                        Loader {
                            sourceComponent: ctlRowHeader
                            property string text: "QGCButton(menu)"
                        }
                        Menu {
                            id: buttonMenu
                            QGCMenuItem {
                                text: qsTr("Item 1")
                            }
                            QGCMenuItem {
                                text: qsTr("Item 2")
                            }
                            QGCMenuItem {
                                text: qsTr("Item 3")
                            }
                        }
                        QGCButton {
                            width: ctlPrevColumn._colWidth
                            height: ctlPrevColumn._height
                            text: qsTr("Button")
                            onClicked: buttonMenu.popup()
                        }
                        QGCButton {
                            width: ctlPrevColumn._colWidth
                            height: ctlPrevColumn._height
                            text: qsTr("Button")
                            enabled: false
                            onClicked: buttonMenu.popup()
                        }

                        // QGCRadioButton
                        Loader {
                            sourceComponent: ctlRowHeader
                            property string text: "QGCRadioButton"
                        }
                        Rectangle {
                            width: ctlPrevColumn._colWidth
                            height: ctlPrevColumn._height
                            color: ctlPrevColumn._bkColor
                            QGCRadioButton {
                                anchors.fill: parent
                                anchors.margins: 5
                                text: qsTr("Radio")
                            }
                        }
                        Rectangle {
                            width: ctlPrevColumn._colWidth
                            height: ctlPrevColumn._height
                            color: ctlPrevColumn._bkColor
                            QGCRadioButton {
                                anchors.fill: parent
                                anchors.margins: 5
                                text: qsTr("Radio")
                                enabled: false
                            }
                        }

                        // QGCCheckBox
                        Loader {
                            sourceComponent: ctlRowHeader
                            property string text: "QGCCheckBox"
                        }
                        Rectangle {
                            width: ctlPrevColumn._colWidth
                            height: ctlPrevColumn._height
                            color: ctlPrevColumn._bkColor
                            QGCCheckBox {
                                anchors.fill: parent
                                anchors.margins: 5
                                text: qsTr("Check Box")
                            }
                        }
                        Rectangle {
                            width: ctlPrevColumn._colWidth
                            height: ctlPrevColumn._height
                            color: ctlPrevColumn._bkColor
                            QGCCheckBox {
                                anchors.fill: parent
                                anchors.margins: 5
                                text: qsTr("Check Box")
                                enabled: false
                            }
                        }

                        // QGCTextField
                        Loader {
                            sourceComponent: ctlRowHeader
                            property string text: "QGCTextField"
                        }
                        QGCTextField {
                            width: ctlPrevColumn._colWidth
                            height: ctlPrevColumn._height
                            text: "QGCTextField"
                        }
                        QGCTextField {
                            width: ctlPrevColumn._colWidth
                            height: ctlPrevColumn._height
                            text: "QGCTextField"
                            enabled: false
                        }

                        // QGCComboBox
                        Loader {
                            sourceComponent: ctlRowHeader
                            property string text: "QGCComboBox"
                        }
                        QGCComboBox {
                            width: ctlPrevColumn._colWidth
                            height: ctlPrevColumn._height
                            model: [ qsTr("Item 1"), qsTr("Item 2"), qsTr("Item 3") ]
                        }
                        QGCComboBox {
                            width: ctlPrevColumn._colWidth
                            height: ctlPrevColumn._height
                            model: [ qsTr("Item 1"), qsTr("Item 2"), qsTr("Item 3") ]
                            enabled: false
                        }

                        // SubMenuButton
                        Loader {
                            sourceComponent: ctlRowHeader
                            property string text: "SubMenuButton"
                        }
                        SubMenuButton {
                            width: ctlPrevColumn._colWidth
                            height: ctlPrevColumn._colWidth/3
                            text: qsTr("SUB MENU")
                        }
                        SubMenuButton {
                            width: ctlPrevColumn._colWidth
                            height: ctlPrevColumn._colWidth/3
                            text: qsTr("SUB MENU")
                            enabled: false
                        }
                    }
                    Rectangle {
                        width:  previewGrid.width
                        height: 60
                        radius: 3
                        color:  palette.alertBackground
                        border.color: palette.alertBorder
                        border.width: 1
                        anchors.horizontalCenter: parent.horizontalCenter
                        Label {
                            text: "Alert Message"
                            color: palette.alertText
                            anchors.centerIn: parent
                        }
                    }
                } // Column
                } // GroupBox { title: "Controls preview"
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

    Component {
        id: ctlRowHeader
        Rectangle {
            width:  ctlPrevColumn._colWidth
            height: ctlPrevColumn._height
            color:  "white"
            Text {
                color:  "black"
                text:   parent.parent.text
                anchors.fill: parent
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

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
}
