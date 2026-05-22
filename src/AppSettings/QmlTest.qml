import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Rectangle {
    id: _root
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth
    color:              "white"

    QGCPalette { id: qgcPal }

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
        var oldTheme = qgcPal.globalTheme;

        qgcPal.globalTheme = QGCPalette.Light
        qgcPal.colorGroupEnabled = true
        themeObj.light["enabled"] = exportPaletteColors(qgcPal);
        qgcPal.colorGroupEnabled = false
        themeObj.light["disabled"] = exportPaletteColors(qgcPal);
        qgcPal.globalTheme = QGCPalette.Dark
        qgcPal.colorGroupEnabled = true
        themeObj.dark["enabled"] = exportPaletteColors(qgcPal);
        qgcPal.colorGroupEnabled = false
        themeObj.dark["disabled"] = exportPaletteColors(qgcPal);

        qgcPal.globalTheme = oldTheme;
        qgcPal.colorGroupEnabled = true;

        var jsonString = JSON.stringify(themeObj, null, 4);

        themeImportExportEdit.text = jsonString
    }

    function exportThemeCPP() {
        var palToExport = ""
        for(var i = 0; i < qgcPal.colors.length; i++) {
            var cs = qgcPal.colors[i]
            var csc = cs + 'Colors'
            palToExport += 'DECLARE_QGC_COLOR(' + cs + ', \"' + qgcPal[csc][1] + '\", \"' + qgcPal[csc][0] + '\", \"' + qgcPal[csc][3] + '\", \"' + qgcPal[csc][2] + '\")\n'
        }
        themeImportExportEdit.text = palToExport
    }

    function exportThemePlugin() {
        var palToExport = ""
        for(var i = 0; i < qgcPal.colors.length; i++) {
            var cs = qgcPal.colors[i]
            var csc = cs + 'Colors'
            if(i > 0) {
                palToExport += '\nelse '
            }
            palToExport +=
            'if (colorName == QStringLiteral(\"' + cs + '\")) {\n' +
            '    colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor(\"' + qgcPal[csc][2] + '\");\n' +
            '    colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor(\"' + qgcPal[csc][3] + '\");\n' +
            '    colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor(\"' + qgcPal[csc][0] + '\");\n' +
            '    colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor(\"' + qgcPal[csc][1] + '\");\n' +
            '}'
        }
        themeImportExportEdit.text = palToExport
    }

    function importTheme(jsonStr) {
        var jsonObj = JSON.parse(jsonStr)
        var themeObj = {"light": {}, "dark":{}}
        var oldTheme = qgcPal.globalTheme;

        qgcPal.globalTheme = QGCPalette.Light
        qgcPal.colorGroupEnabled = true
        fillPalette(qgcPal, jsonObj.light.enabled)
        qgcPal.colorGroupEnabled = false
        fillPalette(qgcPal, jsonObj.light.disabled);
        qgcPal.globalTheme = QGCPalette.Dark
        qgcPal.colorGroupEnabled = true
        fillPalette(qgcPal, jsonObj.dark.enabled);
        qgcPal.colorGroupEnabled = false
        fillPalette(qgcPal, jsonObj.dark.disabled);

        qgcPal.globalTheme = oldTheme;
        qgcPal.colorGroupEnabled = true;

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
            color:          qgcPal.window
            radius:         ScreenTools.defaultFontPixelHeight * 0.5
            border.width:   1
            border.color:   qgcPal.text
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
        color:      qgcPal.window
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
                    checked:    qgcPal.globalTheme === QGCPalette.Light
                    onClicked: {
                        qgcPal.globalTheme = QGCPalette.Light
                    }
                }
                QGCRadioButton {
                    text:       qsTr("Dark")
                    checked:    qgcPal.globalTheme === QGCPalette.Dark
                    onClicked: {
                        qgcPal.globalTheme = QGCPalette.Dark
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
                        for(var colorNameStr in enabledPalette) {
                            if(enabledPalette[colorNameStr].r !== undefined) {
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

                        // QGCCheckBoxSlider
                        Loader {
                            sourceComponent: ctlRowHeader
                            property string text: "QGCCheckBoxSlider"
                        }
                        Rectangle {
                            width: ctlPrevColumn._colWidth
                            height: ctlPrevColumn._height
                            color: ctlPrevColumn._bkColor
                            QGCCheckBoxSlider {
                                anchors.fill: parent
                                anchors.margins: 5
                                text: qsTr("Check Box Slider")
                            }
                        }
                        Rectangle {
                            width: ctlPrevColumn._colWidth
                            height: ctlPrevColumn._height
                            color: ctlPrevColumn._bkColor
                            QGCCheckBoxSlider {
                                anchors.fill: parent
                                anchors.margins: 5
                                text: qsTr("Check Box Slider")
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
                        color:  qgcPal.alertBackground
                        border.color: qgcPal.alertBorder
                        border.width: 1
                        anchors.horizontalCenter: parent.horizontalCenter
                        Label {
                            text: "Alert Message"
                            color: qgcPal.alertText
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

            Item{
                height: 20;
                width:  1;
            }

            // SettingsGroupLayout Test
            GroupBox {
                title: "SettingsGroupLayout Test"
                anchors.horizontalCenter: parent.horizontalCenter

                background: Rectangle {
                    color: qgcPal.window
                    border.color: qgcPal.text
                    border.width: 1
                }

                Column {
                    spacing: ScreenTools.defaultFontPixelHeight

                    // Controls for testing properties
                    Row {
                        spacing: ScreenTools.defaultFontPixelWidth * 2

                        QGCCheckBox {
                            id: showBorderCheck
                            text: "showBorder"
                            checked: true
                        }

                        QGCCheckBox {
                            id: showDividersCheck
                            text: "showDividers"
                            checked: true
                        }

                        QGCCheckBox {
                            id: showHeadingCheck
                            text: "Show Heading"
                            checked: true
                        }

                        QGCCheckBox {
                            id: showDescriptionCheck
                            text: "Show Description"
                            checked: true
                        }
                    }

                    Row {
                        spacing: ScreenTools.defaultFontPixelWidth * 2

                        QGCLabel { text: "Visibility toggles:"; font.bold: true }

                        QGCCheckBox {
                            id: item1Visible
                            text: "Item 1"
                            checked: true
                        }

                        QGCCheckBox {
                            id: item2Visible
                            text: "Item 2"
                            checked: true
                        }

                        QGCCheckBox {
                            id: item3Visible
                            text: "Item 3"
                            checked: true
                        }

                        QGCCheckBox {
                            id: item4Visible
                            text: "Item 4"
                            checked: true
                        }
                    }

                    Row {
                        spacing: ScreenTools.defaultFontPixelWidth * 2

                        QGCLabel { text: "Repeater toggles:"; font.bold: true }

                        QGCCheckBox {
                            id: repeaterShowDividers
                            text: "Dividers"
                            checked: true
                        }

                        QGCCheckBox {
                            id: repeater1Visible
                            text: "Rep 1"
                            checked: true
                        }

                        QGCCheckBox {
                            id: repeater2Visible
                            text: "Rep 2"
                            checked: true
                        }

                        QGCCheckBox {
                            id: repeater3Visible
                            text: "Rep 3"
                            checked: true
                        }

                        QGCCheckBox {
                            id: repeater4Visible
                            text: "Rep 4"
                            checked: true
                        }

                        QGCCheckBox {
                            id: repeater5Visible
                            text: "Rep 5"
                            checked: true
                        }
                    }

                    // Test SettingsGroupLayout with various content
                    SettingsGroupLayout {
                        width: ScreenTools.defaultFontPixelWidth * 60
                        heading: showHeadingCheck.checked ? "Test Settings Group" : ""
                        headingDescription: showDescriptionCheck.checked ? "This is a description of the settings group that explains what these settings are for." : ""
                        showBorder: showBorderCheck.checked
                        showDividers: showDividersCheck.checked

                        RowLayout {
                            Layout.fillWidth: true
                            visible: item1Visible.checked

                            QGCLabel {
                                text: "Setting 1:"
                                Layout.fillWidth: true
                            }
                            QGCTextField {
                                text: "Value 1"
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 15
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            visible: item2Visible.checked

                            QGCLabel {
                                text: "Setting 2:"
                                Layout.fillWidth: true
                            }
                            QGCComboBox {
                                model: ["Option 1", "Option 2", "Option 3"]
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 15
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            visible: item3Visible.checked

                            QGCCheckBox {
                                text: "Enable feature"
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            visible: item4Visible.checked

                            QGCLabel {
                                text: "Setting 4:"
                                Layout.fillWidth: true
                            }
                            QGCButton {
                                text: "Configure"
                            }
                        }
                    }

                    // Nested SettingsGroupLayout test
                    QGCLabel {
                        text: "Nested SettingsGroupLayout:"
                        font.bold: true
                    }

                    SettingsGroupLayout {
                        width: ScreenTools.defaultFontPixelWidth * 60
                        heading: "Outer Group"
                        showBorder: true
                        showDividers: true

                        RowLayout {
                            Layout.fillWidth: true
                            QGCLabel { text: "Outer setting"; Layout.fillWidth: true }
                            QGCTextField { text: "Value" }
                        }

                        SettingsGroupLayout {
                            Layout.fillWidth: true
                            heading: "Inner Group"
                            headingDescription: "Nested group inside outer group"
                            showBorder: true
                            showDividers: false

                            QGCCheckBox {
                                text: "Inner checkbox 1"
                                Layout.fillWidth: true
                            }

                            QGCCheckBox {
                                text: "Inner checkbox 2"
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            QGCLabel { text: "Another outer setting"; Layout.fillWidth: true }
                            QGCComboBox { model: ["A", "B", "C"] }
                        }
                    }

                    // Test with Repeater
                    QGCLabel {
                        text: "SettingsGroupLayout with Repeater:"
                        font.bold: true
                    }

                    SettingsGroupLayout {
                        width: ScreenTools.defaultFontPixelWidth * 60
                        heading: "Repeater Test"
                        showBorder: true
                        showDividers: repeaterShowDividers.checked

                        Repeater {
                            model: 5
                            delegate: RowLayout {
                                Layout.fillWidth: true
                                visible: {
                                    switch(index) {
                                        case 0: return repeater1Visible.checked
                                        case 1: return repeater2Visible.checked
                                        case 2: return repeater3Visible.checked
                                        case 3: return repeater4Visible.checked
                                        case 4: return repeater5Visible.checked
                                        default: return true
                                    }
                                }
                                QGCLabel {
                                    text: "Repeated Item " + (index + 1) + ":"
                                    Layout.fillWidth: true
                                }
                                QGCTextField {
                                    text: "Value " + (index + 1)
                                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 15
                                }
                            }
                        }
                    }
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
