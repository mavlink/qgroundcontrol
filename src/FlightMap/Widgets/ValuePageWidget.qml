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
    property var    _valueDialogInstrumentValue:    null
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
                _valueDialogInstrumentValue = instrumentValue
                mainWindow.showPopupDialog(valueDialog, qsTr("Value Display"), StandardButton.Close)
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
        id: valueDialog

        QGCPopupDialog {
            GridLayout {
                rowSpacing:     _margins
                columnSpacing:  _margins
                columns:        3

                QGCCheckBox {
                    id:         valueCheckBox
                    text:       qsTr("Value")
                    checked:    _valueDialogInstrumentValue.fact
                    onClicked: {
                        if (checked) {
                            _valueDialogInstrumentValue.setFact(_valueDialogInstrumentValue.factGroupNames[0], _valueDialogInstrumentValue.factValueNames[0])
                        } else {
                            _valueDialogInstrumentValue.clearFact()
                        }
                    }
                }

                QGCComboBox {
                    model:                  _valueDialogInstrumentValue.factGroupNames
                    sizeToContents:         true
                    enabled:                valueCheckBox.enabled
                    onModelChanged:         currentIndex = find(_valueDialogInstrumentValue.factGroupName)
                    Component.onCompleted:  currentIndex = find(_valueDialogInstrumentValue.factGroupName)
                    onActivated: {
                        _valueDialogInstrumentValue.setFact(currentText, "")
                        _valueDialogInstrumentValue.icon = ""
                        _valueDialogInstrumentValue.label = _valueDialogInstrumentValue.fact.shortDescription
                    }
                }

                QGCComboBox {
                    model:                  _valueDialogInstrumentValue.factValueNames
                    sizeToContents:         true
                    enabled:                valueCheckBox.enabled
                    onModelChanged:         currentIndex = _valueDialogInstrumentValue.fact ? find(_valueDialogInstrumentValue.factName) : -1
                    Component.onCompleted:  currentIndex = _valueDialogInstrumentValue.fact ? find(_valueDialogInstrumentValue.factName) : -1
                    onActivated: {
                        _valueDialogInstrumentValue.setFact(_valueDialogInstrumentValue.factGroupName, currentText)
                        _valueDialogInstrumentValue.icon = ""
                        _valueDialogInstrumentValue.label = _valueDialogInstrumentValue.fact.shortDescription
                    }
                }

                QGCRadioButton {
                    id:                     iconCheckBox
                    text:                   qsTr("Icon")
                    Component.onCompleted:  checked = _valueDialogInstrumentValue.icon != ""
                    onClicked: {
                        _valueDialogInstrumentValue.label = ""
                        _valueDialogInstrumentValue.icon = _valueDialogInstrumentValue.iconNames[0]
                        mainWindow.showPopupDialog(iconDialog, qsTr("Select Icon"), StandardButton.Close)
                    }
                }

                QGCColoredImage {
                    Layout.alignment:   Qt.AlignHCenter
                    height:             iconPositionCombo.height
                    width:              height
                    source:             "/InstrumentValueIcons/" + (_valueDialogInstrumentValue.icon ? _valueDialogInstrumentValue.icon : _valueDialogInstrumentValue.iconNames[0])
                    sourceSize.height:  height
                    fillMode:           Image.PreserveAspectFit
                    mipmap:             true
                    smooth:             true
                    color:              enabled ? qgcPal.text : qgcPalDisabled.text
                    enabled:            iconCheckBox.checked

                    MouseArea {
                        anchors.fill:   parent
                        onClicked:      mainWindow.showPopupDialog(iconDialog, qsTr("Select Icon"), StandardButton.Close)
                    }
                }

                QGCComboBox {
                    id:             iconPositionCombo
                    model:          _valueDialogInstrumentValue.iconPositionNames
                    currentIndex:   _valueDialogInstrumentValue.iconPosition
                    sizeToContents: true
                    onActivated:    _valueDialogInstrumentValue.iconPosition = index
                    enabled:        iconCheckBox.checked
                }

                QGCRadioButton {
                    id:                     labelCheckBox
                    text:                   qsTr("Label")
                    Component.onCompleted:  checked = _valueDialogInstrumentValue.label != ""
                    onClicked: {
                        _valueDialogInstrumentValue.icon = ""
                        _valueDialogInstrumentValue.label = _valueDialogInstrumentValue.fact ? _valueDialogInstrumentValue.fact.shortDescription : qsTr("Label")
                    }
                }

                QGCTextField {
                    id:                 labelTextField
                    Layout.fillWidth:   true
                    Layout.columnSpan:  2
                    text:               _valueDialogInstrumentValue.label
                    enabled:            labelCheckBox.checked
                }

                QGCLabel { text: qsTr("Size") }

                QGCComboBox {
                    id:                 fontSizeCombo
                    Layout.columnSpan:  2
                    model:              _valueDialogInstrumentValue.fontSizeNames
                    currentIndex:       _valueDialogInstrumentValue.fontSize
                    sizeToContents:     true
                    onActivated:        _valueDialogInstrumentValue.fontSize = index
                }

                QGCCheckBox {
                    Layout.columnSpan:  3
                    text:               qsTr("Show Units")
                    checked:            _valueDialogInstrumentValue.showUnits
                    onClicked:          _valueDialogInstrumentValue.showUnits = checked
                }
            }
        }
    }

    Component {
        id: iconDialog

        QGCPopupDialog {
            GridLayout {
                columns:        10
                columnSpacing:  0
                rowSpacing:     0

                Repeater {
                    model: _valueDialogInstrumentValue.iconNames

                    Rectangle {
                        height: ScreenTools.minTouchPixels
                        width:  height
                        color:  currentSelection ? qgcPal.text  : qgcPal.window

                        property bool currentSelection: _valueDialogInstrumentValue.icon == modelData

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
                                    _valueDialogInstrumentValue.icon = modelData
                                    hideDialog()
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
