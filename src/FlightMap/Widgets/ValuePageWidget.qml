/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.12
import QtQuick.Dialogs  1.3
import QtQuick.Layouts  1.2
import QtQuick.Controls 2.5
import QtQml            2.12

import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controllers   1.0
import QGroundControl.Palette       1.0
import QGroundControl               1.0

/// Value page for InstrumentPanel PageView
Column {
    id:         _root
    width:      pageWidth
    spacing:    ScreenTools.defaultFontPixelHeight / 2

    property bool showSettingsIcon: true
    property bool showLockIcon:     true

    property var    _activeVehicle:                 QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle : QGroundControl.multiVehicleManager.offlineEditingVehicle
    property real   _margins:                       ScreenTools.defaultFontPixelWidth / 2
    property int    _colMax:                        4
    property bool   _settingsUnlocked:              false
    property var    instrumentValue:    null
    property var    _rgFontSizes:                   [ ScreenTools.defaultFontPointSize, ScreenTools.smallFontPointSize, ScreenTools.mediumFontPointSize, ScreenTools.largeFontPointSize ]
    property var    _rgFontSizeRatios:              [ 1, ScreenTools.smallFontPointRatio, ScreenTools.mediumFontPointRatio, ScreenTools.largeFontPointRatio ]
    property real   _doubleDescent:                 ScreenTools.defaultFontDescent * 2
    property real   _tightDefaultFontHeight:        ScreenTools.defaultFontPixelHeight - _doubleDescent
    property var    _rgFontSizeTightHeights:        [ _tightDefaultFontHeight * _rgFontSizeRatios[0] + 2, _tightDefaultFontHeight * _rgFontSizeRatios[1] + 2, _tightDefaultFontHeight * _rgFontSizeRatios[2] + 2, _tightDefaultFontHeight * _rgFontSizeRatios[3] + 2 ]
    property real   _blankEntryHeight:              ScreenTools.defaultFontPixelHeight * 2
    property real   _columnButtonWidth:             ScreenTools.minTouchPixels / 2
    property real   _columnButtonHeight:            ScreenTools.minTouchPixels
    property real   _columnButtonSpacing:           2
    property real   _columnButtonsTotalHeight:      (_columnButtonHeight * 2) + _columnButtonSpacing

    QGCPalette { id:qgcPal; colorGroupEnabled: true }
    QGCPalette { id:qgcPalDisabled; colorGroupEnabled: false }

    ValuesWidgetController { id: controller }

    function showSettings(settingsUnlocked) {
        _settingsUnlocked = settingsUnlocked
    }

    function listContains(list, value) {
        for (var i=0; i<list.length; i++) {
            if (list[i] === value) {
                return true
            }
        }
        return false
    }

    ButtonGroup { id: factRadioGroup }

    Component {
        id: valueItemMouseAreaComponent

        MouseArea {
            anchors.centerIn:   parent
            width:              parent.width
            height:             _columnButtonsTotalHeight
            visible:            _settingsUnlocked

            property var instrumentValue
            property int rowIndex

            onClicked: {
                instrumentValue = instrumentValue
                mainWindow.showPopupDialog(valueDialogComponent, { instrumentValue: instrumentValue })
            }
        }
    }

    Repeater {
        id:     rowRepeater
        model:  controller.valuesModel

        Column {
            id:             rowRepeaterLayout
            spacing:        1

            property int rowIndex: index

            Row {
                id:         columnRow
                spacing:    1

                Repeater {
                    id:     columnRepeater
                    model:  object

                    property real _interColumnSpacing:  (columnRepeater.count - (_settingsUnlocked ? 0 : 1)) * columnRow.spacing
                    property real columnWidth:          (pageWidth - (_settingsUnlocked ? _columnButtonWidth : 0) - _interColumnSpacing) / columnRepeater.count
                    property bool componentCompleted:   false

                    Component.onCompleted: componentCompleted = true
                    onItemAdded: valueItemMouseAreaComponent.createObject(item, { "instrumentValue": object.get(index), "rowIndex": index })

                    Item {
                        id:                     columnItem
                        anchors.verticalCenter: parent.verticalCenter
                        width:                  columnRepeater.columnWidth
                        height:                 value.y + value.height

                        property real columnWidth:                  columnRepeater.columnWidth
                        property bool repeaterComponentCompleted:   columnRepeater.componentCompleted

                        // After fighting with using layout and/or anchors I gave up and just do a manual recalc to position items which ends up being much simpler
                        function recalcPositions() {
                            if (!repeaterComponentCompleted) {
                                return
                            }
                            var smallSpacing = 2
                            if (object.icon) {
                                if (object.iconPosition === InstrumentValue.IconAbove) {
                                    valueIcon.x = (width - valueIcon.width) / 2
                                    valueIcon.y = 0
                                    value.x = (width - value.width) / 2
                                    value.y = valueIcon.height + smallSpacing
                                } else {
                                    var iconPlusValueWidth = valueIcon.width + value.width + ScreenTools.defaultFontPixelWidth
                                    valueIcon.x = (width - iconPlusValueWidth) / 2
                                    valueIcon.y = (value.height - valueIcon.height) / 2
                                    value.x = valueIcon.x + valueIcon.width + (ScreenTools.defaultFontPixelWidth / 2)
                                    value.y = 0
                                }
                                label.x = label.y = 0
                            } else {
                                // label above value
                                if (object.label) {
                                    label.x = (width - label.width) / 2
                                    label.y = 0
                                    value.y = label.height + smallSpacing
                                } else {
                                    value.y = 0
                                }
                                value.x = (width - value.width) / 2
                                valueIcon.x = valueIcon.y = 0
                            }
                        }

                        onRepeaterComponentCompletedChanged:    recalcPositions()
                        onColumnWidthChanged:                   recalcPositions()

                        Connections {
                            target:                 object
                            onIconChanged:          recalcPositions()
                            onIconPositionChanged:  recalcPositions()
                        }

                        QGCColoredImage {
                            id:                         valueIcon
                            height:                     _rgFontSizeTightHeights[object.fontSize]
                            width:                      height
                            source:                     icon
                            sourceSize.height:          height
                            fillMode:                   Image.PreserveAspectFit
                            mipmap:                     true
                            smooth:                     true
                            color:                      object.isValidColor(object.currentColor) ? object.currentColor : qgcPal.text
                            opacity:                    object.currentOpacity
                            visible:                    object.icon
                            onWidthChanged:             columnItem.recalcPositions()
                            onHeightChanged:            columnItem.recalcPositions()

                            property string icon
                            readonly property string iconPrefix: "/InstrumentValueIcons/"

                            function updateIcon() {
                                if (object.rangeType == InstrumentValue.IconSelectRange) {
                                    icon = iconPrefix + object.currentIcon
                                } else if (object.icon) {
                                    icon = iconPrefix + object.icon
                                } else {
                                    icon = ""
                                }
                            }

                            Connections {
                                target:                 object
                                onRangeTypeChanged:     valueIcon.updateIcon()
                                onCurrentIconChanged:   valueIcon.updateIcon()
                                onIconChanged:          valueIcon.updateIcon()
                            }
                            Component.onCompleted:      updateIcon();
                        }

                        QGCLabel {
                            id:                         blank
                            anchors.horizontalCenter:   parent.horizontalCenter
                            height:                     _columnButtonsTotalHeight
                            font.pointSize:             ScreenTools.smallFontPointSize
                            text:                       _settingsUnlocked ? qsTr("BLANK") : ""
                            horizontalAlignment:        Text.AlignHCenter
                            verticalAlignment:          Text.AlignVCenter
                            visible:                    !object.fact
                            onWidthChanged:             columnItem.recalcPositions()
                            onHeightChanged:            columnItem.recalcPositions()
                        }

                        QGCLabel {
                            id:                         label
                            height:                     _rgFontSizeTightHeights[InstrumentValue.SmallFontSize]
                            font.pointSize:             ScreenTools.smallFontPointSize
                            text:                       object.label.toUpperCase()
                            verticalAlignment:          Text.AlignVCenter
                            visible:                    object.fact && object.label && !object.icon
                            onWidthChanged:             columnItem.recalcPositions()
                            onHeightChanged:            columnItem.recalcPositions()
                        }

                        QGCLabel {
                            id:                         value
                            font.pointSize:             _rgFontSizes[object.fontSize]
                            text:                       visible ? (object.fact.enumOrValueString + (object.showUnits ? object.fact.units : "")) : ""
                            verticalAlignment:          Text.AlignVCenter
                            visible:                    object.fact
                            onWidthChanged:             columnItem.recalcPositions()
                            onHeightChanged:            columnItem.recalcPositions()
                        }
                    }
                } // Repeater - columns

                ColumnLayout {
                    id:                 columnsButtonsLayout
                    width:              _columnButtonWidth
                    spacing:            _columnButtonSpacing
                    visible:            _settingsUnlocked

                    QGCButton {
                        Layout.fillHeight:      true
                        Layout.preferredHeight: ScreenTools.minTouchPixels
                        Layout.preferredWidth:  parent.width
                        text:                   qsTr("+")
                        onClicked:              controller.appendColumn(rowRepeaterLayout.rowIndex)
                    }

                    QGCButton {
                        Layout.fillHeight:      true
                        Layout.preferredHeight: ScreenTools.minTouchPixels
                        Layout.preferredWidth:  parent.width
                        text:                   qsTr("-")
                        enabled:                index !== 0 || columnRepeater.count !== 1
                        onClicked:              controller.deleteLastColumn(rowRepeaterLayout.rowIndex)
                    }
                }
            } // RowLayout

            RowLayout {
                width:      parent.width
                height:     ScreenTools.defaultFontPixelWidth * 2
                spacing:    1
                visible:    _settingsUnlocked

                QGCButton {
                    Layout.fillWidth:   true
                    Layout.preferredHeight: ScreenTools.defaultFontPixelWidth * 2
                    text:               qsTr("+")
                    onClicked:          controller.insertRow(index + 1)
                }

                QGCButton {
                    Layout.fillWidth:   true
                    Layout.preferredHeight: ScreenTools.defaultFontPixelWidth * 2
                    text:               qsTr("-")
                    enabled:            index !== 0
                    onClicked:          controller.deleteRow(index)
                }
            }
        }
    } // Repeater - rows

    QGCButton {
        anchors.left:   parent.left
        anchors.right:  parent.right
        text:           qsTr("Reset To Defaults")
        visible:        _settingsUnlocked
        onClicked:      controller.resetToDefaults()
    }

    Component {
        id: valueDialogComponent

        QGCPopupDialog {
            id:         valueDisplayDialog
            title:      qsTr("Value Display")
            buttons:    StandardButton.Close

            property var instrumentValue: dialogProperties.instrumentValue

            GridLayout {
                rowSpacing:     _margins
                columnSpacing:  _margins
                columns:        3

                QGCCheckBox {
                    id:         valueCheckBox
                    text:       qsTr("Value")
                    checked:    instrumentValue.fact
                    onClicked: {
                        if (checked) {
                            instrumentValue.setFact(instrumentValue.factGroupNames[0], instrumentValue.factValueNames[0])
                        } else {
                            instrumentValue.clearFact()
                        }
                    }
                }

                QGCComboBox {
                    model:                  instrumentValue.factGroupNames
                    sizeToContents:         true
                    enabled:                valueCheckBox.enabled
                    onModelChanged:         currentIndex = find(instrumentValue.factGroupName)
                    Component.onCompleted:  currentIndex = find(instrumentValue.factGroupName)
                    onActivated: {
                        instrumentValue.setFact(currentText, "")
                        instrumentValue.icon = ""
                        instrumentValue.label = instrumentValue.fact.shortDescription
                    }
                }

                QGCComboBox {
                    model:                  instrumentValue.factValueNames
                    sizeToContents:         true
                    enabled:                valueCheckBox.enabled
                    onModelChanged:         currentIndex = instrumentValue.fact ? find(instrumentValue.factName) : -1
                    Component.onCompleted:  currentIndex = instrumentValue.fact ? find(instrumentValue.factName) : -1
                    onActivated: {
                        instrumentValue.setFact(instrumentValue.factGroupName, currentText)
                        instrumentValue.icon = ""
                        instrumentValue.label = instrumentValue.fact.shortDescription
                    }
                }

                QGCRadioButton {
                    id:                     iconCheckBox
                    text:                   qsTr("Icon")
                    Component.onCompleted:  checked = instrumentValue.icon != ""
                    onClicked: {
                        instrumentValue.label = ""
                        instrumentValue.icon = instrumentValue.iconNames[0]
                        var updateFunction = function(icon){ instrumentValue.icon = icon }
                        mainWindow.showPopupDialog(iconPickerDialog, { iconNames: instrumentValue.iconNames, icon: instrumentValue.icon, updateIconFunction: updateFunction })
                    }
                }

                QGCColoredImage {
                    Layout.alignment:   Qt.AlignHCenter
                    height:             iconPositionCombo.height
                    width:              height
                    source:             "/InstrumentValueIcons/" + (instrumentValue.icon ? instrumentValue.icon : instrumentValue.iconNames[0])
                    sourceSize.height:  height
                    fillMode:           Image.PreserveAspectFit
                    mipmap:             true
                    smooth:             true
                    color:              enabled ? qgcPal.text : qgcPalDisabled.text
                    enabled:            iconCheckBox.checked

                    MouseArea {
                        anchors.fill:   parent
                        onClicked: {
                            var updateFunction = function(icon){ instrumentValue.icon = icon }
                            mainWindow.showPopupDialog(iconPickerDialog, { iconNames: instrumentValue.iconNames, icon: instrumentValue.icon, updateIconFunction: updateFunction })
                        }
                    }
                }

                QGCComboBox {
                    id:             iconPositionCombo
                    model:          instrumentValue.iconPositionNames
                    currentIndex:   instrumentValue.iconPosition
                    sizeToContents: true
                    onActivated:    instrumentValue.iconPosition = index
                    enabled:        iconCheckBox.checked
                }

                QGCRadioButton {
                    id:                     labelCheckBox
                    text:                   qsTr("Label")
                    Component.onCompleted:  checked = instrumentValue.label != ""
                    onClicked: {
                        instrumentValue.icon = ""
                        instrumentValue.label = instrumentValue.fact ? instrumentValue.fact.shortDescription : qsTr("Label")
                    }
                }

                QGCTextField {
                    id:                 labelTextField
                    Layout.fillWidth:   true
                    Layout.columnSpan:  2
                    text:               instrumentValue.label
                    enabled:            labelCheckBox.checked
                }

                QGCLabel { text: qsTr("Size") }

                QGCComboBox {
                    id:                 fontSizeCombo
                    model:              instrumentValue.fontSizeNames
                    currentIndex:       instrumentValue.fontSize
                    sizeToContents:     true
                    onActivated:        instrumentValue.fontSize = index
                }

                QGCCheckBox {
                    text:               qsTr("Show Units")
                    checked:            instrumentValue.showUnits
                    onClicked:          instrumentValue.showUnits = checked
                }

                QGCLabel { text: qsTr("Range") }

                QGCComboBox {
                    id:                 rangeTypeCombo
                    Layout.columnSpan:  2
                    model:              instrumentValue.rangeTypeNames
                    currentIndex:       instrumentValue.rangeType
                    sizeToContents:     true
                    onActivated:        instrumentValue.rangeType = index
                }

                Loader {
                    id:                     rangeLoader
                    Layout.columnSpan:      3
                    Layout.fillWidth:       true
                    Layout.preferredWidth:  item ? item.width : 0
                    Layout.preferredHeight: item ? item.height : 0

                    property var instrumentValue: valueDisplayDialog.instrumentValue

                    function updateSourceComponent() {
                        switch (instrumentValue.rangeType) {
                        case InstrumentValue.NoRangeInfo:
                            sourceComponent = undefined
                            break
                        case InstrumentValue.ColorRange:
                            sourceComponent = colorRangeDialog
                            break
                        case InstrumentValue.OpacityRange:
                            sourceComponent = opacityRangeDialog
                            break
                        case InstrumentValue.IconSelectRange:
                            sourceComponent = iconRangeDialog
                            break
                        }
                    }

                    Component.onCompleted: updateSourceComponent()

                    Connections {
                        target:             instrumentValue
                        onRangeTypeChanged: rangeLoader.updateSourceComponent()
                    }

                }
            }
        }
    }

    Component {
        id: iconPickerDialog

        QGCPopupDialog {
            property var     iconNames:             dialogProperties.iconNames
            property string  icon:                  dialogProperties.icon
            property var     updateIconFunction:    dialogProperties.updateIconFunction

            title:      qsTr("Select Icon")
            buttons:    StandardButton.Close

            GridLayout {
                columns:        10
                columnSpacing:  0
                rowSpacing:     0

                Repeater {
                    model: iconNames

                    Rectangle {
                        height: ScreenTools.minTouchPixels
                        width:  height
                        color:  currentSelection ? qgcPal.text  : qgcPal.window

                        property bool currentSelection: icon == modelData

                        QGCColoredImage {
                            anchors.centerIn:   parent
                            height:             parent.height * 0.75
                            width:              height
                            source:             "/InstrumentValueIcons/" + modelData
                            sourceSize.height:  height
                            fillMode:           Image.PreserveAspectFit
                            mipmap:             true
                            smooth:             true
                            color:              currentSelection ? qgcPal.window : qgcPal.text

                            MouseArea {
                                anchors.fill:   parent
                                onClicked:  {
                                    icon = modelData
                                    updateIconFunction(modelData)
                                    hideDialog()
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: colorRangeDialog

        Item {
            width:  childrenRect.width
            height: childrenRect.height

            function updateRangeValue(index, text) {
                var newValues = instrumentValue.rangeValues
                newValues[index] = parseFloat(text)
                instrumentValue.rangeValues = newValues
            }

            function updateColorValue(index, color) {
                var newColors = instrumentValue.rangeColors
                newColors[index] = color
                instrumentValue.rangeColors = newColors
            }

            ColorDialog {
                id:             colorPickerDialog
                modality:       Qt.ApplicationModal
                currentColor:   instrumentValue.rangeColors[colorIndex]
                onAccepted:     updateColorValue(colorIndex, color)

                property int colorIndex: 0
            }

            Column {
                id:         mainColumn
                spacing:    ScreenTools.defaultFontPixelHeight / 2

                QGCLabel {
                    width:      rowLayout.width
                    text:       qsTr("Specify the color you want to apply based on value ranges. The color will be applied to the icon if available, otherwise to the value itself.")
                    wrapMode:   Text.WordWrap
                }

                Row {
                    id:         rowLayout
                    spacing:    _margins

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing:                _margins

                        Repeater {
                            model: instrumentValue.rangeValues.length

                            QGCButton {
                                width:      ScreenTools.implicitTextFieldHeight
                                height:     width
                                text:       qsTr("-")
                                onClicked:  instrumentValue.removeRangeValue(index)
                            }
                        }
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing:                _margins

                        Repeater {
                            model: instrumentValue.rangeValues.length

                            QGCTextField {
                                text:               instrumentValue.rangeValues[index]
                                onEditingFinished:  updateRangeValue(index, text)
                            }
                        }
                    }

                    Column {
                        spacing: _margins
                        Repeater {
                            model: instrumentValue.rangeColors

                            QGCCheckBox {
                                height:     ScreenTools.implicitTextFieldHeight
                                checked:    instrumentValue.isValidColor(instrumentValue.rangeColors[index])
                                onClicked:  updateColorValue(index, checked ? "green" : instrumentValue.invalidColor())
                            }
                        }
                    }

                    Column {
                        spacing: _margins
                        Repeater {
                            model: instrumentValue.rangeColors

                            Rectangle {
                                width:          ScreenTools.implicitTextFieldHeight
                                height:         width
                                border.color:   qgcPal.text
                                color:          instrumentValue.isValidColor(modelData) ? modelData : qgcPal.text

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        colorPickerDialog.colorIndex = index
                                        colorPickerDialog.open()
                                    }
                                }
                            }
                        }
                    }
                }

                QGCButton {
                    text:       qsTr("Add Row")
                    onClicked:  instrumentValue.addRangeValue()
                }
            }
        }
    }

    Component {
        id: iconRangeDialog

        Item {
            width:  childrenRect.width
            height: childrenRect.height

            function updateRangeValue(index, text) {
                var newValues = instrumentValue.rangeValues
                newValues[index] = parseFloat(text)
                instrumentValue.rangeValues = newValues
            }

            function updateIconValue(index, icon) {
                var newIcons = instrumentValue.rangeIcons
                newIcons[index] = icon
                instrumentValue.rangeIcons = newIcons
            }

            Column {
                id:         mainColumn
                spacing:    ScreenTools.defaultFontPixelHeight / 2

                QGCLabel {
                    width:      rowLayout.width
                    text:       qsTr("Specify the icon you want to display based on value ranges.")
                    wrapMode:   Text.WordWrap
                }

                Row {
                    id:         rowLayout
                    spacing:    _margins

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing:                _margins

                        Repeater {
                            model: instrumentValue.rangeValues.length

                            QGCButton {
                                width:      ScreenTools.implicitTextFieldHeight
                                height:     width
                                text:       qsTr("-")
                                onClicked:  instrumentValue.removeRangeValue(index)
                            }
                        }
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing:                _margins

                        Repeater {
                            model: instrumentValue.rangeValues.length

                            QGCTextField {
                                text:               instrumentValue.rangeValues[index]
                                onEditingFinished:  updateRangeValue(index, text)
                            }
                        }
                    }

                    Column {
                        spacing: _margins

                        Repeater {
                            model: instrumentValue.rangeIcons

                            QGCColoredImage {
                                height:             ScreenTools.implicitTextFieldHeight
                                width:              height
                                source:             "/InstrumentValueIcons/" + modelData
                                sourceSize.height:  height
                                fillMode:           Image.PreserveAspectFit
                                mipmap:             true
                                smooth:             true
                                color:              qgcPal.text

                                MouseArea {
                                    anchors.fill:   parent
                                    onClicked: {
                                        var updateFunction = function(icon){ updateIconValue(index, icon) }
                                        mainWindow.showPopupDialog(iconPickerDialog, { iconNames: instrumentValue.iconNames, icon: modelData, updateIconFunction = updateFunction })
                                    }
                                }
                            }
                        }
                    }
                }

                QGCButton {
                    text:       qsTr("Add Row")
                    onClicked:  instrumentValue.addRangeValue()
                }
            }
        }
    }

    Component {
        id: opacityRangeDialog

        Item {
            width:  childrenRect.width
            height: childrenRect.height

            function updateRangeValue(index, text) {
                var newValues = instrumentValue.rangeValues
                newValues[index] = parseFloat(text)
                instrumentValue.rangeValues = newValues
            }

            function updateOpacityValue(index, opacity) {
                var newOpacities = instrumentValue.rangeOpacities
                newOpacities[index] = opacity
                instrumentValue.rangeOpacities = newOpacities
            }

            Column {
                id:         mainColumn
                spacing:    ScreenTools.defaultFontPixelHeight / 2

                QGCLabel {
                    width:      rowLayout.width
                    text:       qsTr("Specify the icon opacity you want based on value ranges.")
                    wrapMode:   Text.WordWrap
                }

                Row {
                    id:         rowLayout
                    spacing:    _margins

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing:                _margins

                        Repeater {
                            model: instrumentValue.rangeValues.length

                            QGCButton {
                                width:      ScreenTools.implicitTextFieldHeight
                                height:     width
                                text:       qsTr("-")
                                onClicked:  instrumentValue.removeRangeValue(index)
                            }
                        }
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing:                _margins

                        Repeater {
                            model: instrumentValue.rangeValues

                            QGCTextField {
                                text:               modelData
                                onEditingFinished:  updateRangeValue(index, text)
                            }
                        }
                    }

                    Column {
                        spacing: _margins

                        Repeater {
                            model: instrumentValue.rangeOpacities

                            QGCTextField {
                                text:               modelData
                                onEditingFinished:  updateOpacityValue(index, text)
                            }
                        }
                    }
                }

                QGCButton {
                    text:       qsTr("Add Row")
                    onClicked:  instrumentValue.addRangeValue()
                }
            }
        }
    }
}
