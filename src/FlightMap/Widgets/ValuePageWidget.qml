/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.12
import QtQuick.Dialogs  1.2
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
    property var    _valuePickerInstrumentValue:    null
    property int    _valuePickerRowIndex:           0
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
                _valuePickerInstrumentValue = instrumentValue
                _valuePickerRowIndex = rowIndex
                mainWindow.showComponentDialog(valuePickerDialog, qsTr("Select Value"), mainWindow.showDialogDefaultWidth, StandardButton.Ok)
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
                                if (label) {
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
                            source:                     object.icon ? "/InstrumentValueIcons/" + object.icon : ""
                            sourceSize.height:          height
                            fillMode:                   Image.PreserveAspectFit
                            mipmap:                     true
                            smooth:                     true
                            color:                      qgcPal.text
                            visible:                    object.icon
                            onWidthChanged:             columnItem.recalcPositions()
                            onHeightChanged:            columnItem.recalcPositions()
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
                        Layout.minimumHeight:   ScreenTools.minTouchPixels
                        Layout.preferredWidth:  parent.width
                        text:                   qsTr("+")
                        onClicked:              controller.appendColumn(rowRepeaterLayout.rowIndex)
                    }

                    QGCButton {
                        Layout.fillHeight:      true
                        Layout.minimumHeight:   ScreenTools.minTouchPixels
                        Layout.preferredWidth:  parent.width
                        text:                   qsTr("-")
                        enabled:                index !== 0 || columnRepeater.count !== 1
                        onClicked:              controller.deleteLastColumn(rowRepeaterLayout.rowIndex)
                    }
                }
            } // RowLayout

            RowLayout {
                width:      parent.width
                height:             ScreenTools.defaultFontPixelWidth * 2
                spacing:            1
                visible:            _settingsUnlocked

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
        id: valuePickerDialog

        QGCViewDialog {
            function accept() {
                if (factRadioGroup.checkedButton) {
                    _valuePickerInstrumentValue.setFact(factRadioGroup.checkedButton.radioFactGroupName, factRadioGroup.checkedButton.radioFact.name, labelTextField.text, fontSizeCombo.currentIndex)
                } else {
                    _valuePickerInstrumentValue.clearFact()
                }

                hideDialog()
            }

            Connections {
                target: factRadioGroup
                onCheckedButtonChanged: labelTextField.text = factRadioGroup.checkedButton.radioFact.shortDescription
            }

            ButtonGroup { id: fontRadioGroup }

            QGCFlickable {
                anchors.fill:       parent
                contentHeight:      column.height
                flickableDirection: Flickable.VerticalFlick
                clip:               true

                ColumnLayout {
                    id:             column
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    spacing:        _margins

                    QGCButton {
                        Layout.fillWidth:   true
                        text:               qsTr("Blank Entry")
                        onClicked:          { _valuePickerInstrumentValue.clearFact(); hideDialog() }
                    }

                    RowLayout {
                        Layout.fillWidth:   true
                        spacing:            ScreenTools.defaultFontPixelWidth

                        QGCLabel { text: qsTr("Label") }
                        QGCTextField {
                            id:                 labelTextField
                            Layout.fillWidth:   true
                            text:               _valuePickerInstrumentValue.label
                        }
                    }

                    RowLayout {
                        spacing: ScreenTools.defaultFontPixelWidth

                        QGCLabel { text: qsTr("Font Size") }
                        QGCComboBox {
                            id:             fontSizeCombo
                            model:          _valuePickerInstrumentValue.fontSizeNames
                            currentIndex:   _valuePickerInstrumentValue.fontSize
                            sizeToContents: true
                            onActivated:    _valuePickerInstrumentValue.fontSize = index
                        }
                        QGCCheckBox {
                            text:       qsTr("Show Units")
                            checked:    _valuePickerInstrumentValue.showUnits
                            onClicked:  _valuePickerInstrumentValue.showUnits = checked
                        }
                    }

                    RowLayout {
                        spacing: ScreenTools.defaultFontPixelWidth

                        QGCLabel { text: qsTr("Icon") }

                        Rectangle {
                            height:         iconPositionCombo.height
                            width:          noIconLabel.width + ScreenTools.defaultFontPixelWidth * 2
                            color:          qgcPal.window
                            border.color:   qgcPal.text
                            visible:        !_valuePickerInstrumentValue.icon

                            QGCLabel {
                                id:                 noIconLabel
                                anchors.centerIn:   parent
                                text:               qsTr("No Icon")
                            }
                        }

                        QGCColoredImage {
                            height:             iconPositionCombo.height
                            width:              height
                            source:             _valuePickerInstrumentValue.icon ? "/InstrumentValueIcons/" + _valuePickerInstrumentValue.icon : ""
                            sourceSize.height:  height
                            fillMode:           Image.PreserveAspectFit
                            mipmap:             true
                            smooth:             true
                            color:              qgcPal.text
                            visible:            _valuePickerInstrumentValue.icon
                        }

                        QGCComboBox {
                            id:             iconPositionCombo
                            model:          _valuePickerInstrumentValue.iconPositionNames
                            currentIndex:   _valuePickerInstrumentValue.iconPosition
                            sizeToContents: true
                            onActivated:    _valuePickerInstrumentValue.iconPosition = index
                        }
                    }

                    SectionHeader {
                        id:                 iconListHeader
                        Layout.fillWidth:   true
                        text:               qsTr("Icons")
                        checked:            false
                    }

                    Item { width: 1; height: 1 }

                    Loader {
                        Layout.fillWidth:   true
                        sourceComponent:    iconListHeader.checked ? iconList : undefined
                        visible:            iconListHeader.checked
                    }

                    Loader {
                        Layout.fillWidth:   true
                        sourceComponent:    factGroupList

                        property var    factGroup:     _activeVehicle
                        property string factGroupName: "Vehicle"
                    }

                    Repeater {
                        model: _activeVehicle.factGroupNames

                        Loader {
                            Layout.fillWidth:   true
                            sourceComponent:    factGroupList

                            property var    factGroup:     _activeVehicle.getFactGroup(modelData)
                            property string factGroupName: modelData
                        }
                    }
                }
            }
        }
    }

    Component {
        id: iconList

        Flow {
            Rectangle {
                height:         ScreenTools.minTouchPixels
                width:          noIconLabel.width + ScreenTools.defaultFontPixelWidth * 2
                color:          isNoIcon ? qgcPal.text : qgcPal.window
                border.color:   isNoIcon ? qgcPal.window : qgcPal.text

                property bool isNoIcon: _valuePickerInstrumentValue.icon === ""

                QGCLabel {
                    id:                 noIconLabel
                    anchors.centerIn:   parent
                    color:              parent.isNoIcon ? qgcPal.window : qgcPal.text
                    text:               qsTr("No Icon")
                }

                MouseArea {
                    anchors.fill:   parent
                    onClicked:      _valuePickerInstrumentValue.icon = ""
                }
            }

            Repeater {
                model: _valuePickerInstrumentValue.iconNames

                Rectangle {
                    height: ScreenTools.minTouchPixels
                    width:  height
                    color:  currentSelection ? qgcPal.text  : qgcPal.window

                    property bool currentSelection: _valuePickerInstrumentValue.icon == modelData

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
                            onClicked:      _valuePickerInstrumentValue.icon = modelData
                        }
                    }
                }
            }
        }
    }

    Component {
        id: factGroupList

        // You must push in the following properties from the Loader
        // property var factGroup
        // property string factGroupName

        Column {
            SectionHeader {
                id:             header
                anchors.left:   parent.left
                anchors.right:  parent.right
                text:           factGroupName.charAt(0).toUpperCase() + factGroupName.slice(1)
                checked:        false
            }

            Column {
                visible: header.checked

                Repeater {
                    model: factGroup ? factGroup.factNames : 0

                    QGCRadioButton {
                        text:               radioFact.shortDescription
                        ButtonGroup.group:  factRadioGroup
                        checked:            radioFactGroupName == _valuePickerInstrumentValue.factGroupName && radioFact == _valuePickerInstrumentValue.fact

                        property string radioFactGroupName: factGroupName
                        property var    radioFact:          factGroup.getFact(modelData)

                        Component.onCompleted: {
                            if (checked) {
                                header.checked = true
                            }
                        }
                    }
                }
            }
        }
    }
}
