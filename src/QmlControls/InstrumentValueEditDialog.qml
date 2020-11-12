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

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controllers   1.0
import QGroundControl.Palette       1.0

QGCPopupDialog {
    id:         root
    title:      qsTr("Value Display")
    buttons:    StandardButton.Close

    property var instrumentValueData: dialogProperties.instrumentValueData

    QGCPalette { id: qgcPal;            colorGroupEnabled: parent.enabled }
    QGCPalette { id: qgcPalDisabled;    colorGroupEnabled: false }

    Loader {
        sourceComponent: instrumentValueData.fact ? editorComponent : noFactComponent
    }

    Component {
        id: noFactComponent

        QGCLabel {
            text: qsTr("Valuec requires a connected vehicle for setup.")
        }
    }

    Component {
        id: editorComponent

        GridLayout {
            rowSpacing:     _margins
            columnSpacing:  _margins
            columns:        2

            QGCComboBox {
                id:                     factGroupCombo
                Layout.fillWidth:       true
                model:                  instrumentValueData.factGroupNames
                sizeToContents:         true
                Component.onCompleted:  currentIndex = find(instrumentValueData.factGroupName)
                onActivated: {
                    instrumentValueData.setFact(currentText, "")
                    instrumentValueData.icon = ""
                    instrumentValueData.text = instrumentValueData.fact.shortDescription
                }
                Connections {
                    target: instrumentValueData
                    onFactGroupNameChanged: factGroupCombo.currentIndex = factGroupCombo.find(instrumentValueData.factGroupName)
                }
            }

            QGCComboBox {
                id:                     factNamesCombo
                Layout.fillWidth:       true
                model:                  instrumentValueData.factValueNames
                sizeToContents:         true
                Component.onCompleted:  currentIndex = find(instrumentValueData.factName)
                onActivated: {
                    instrumentValueData.setFact(instrumentValueData.factGroupName, currentText)
                    instrumentValueData.icon = ""
                    instrumentValueData.text = instrumentValueData.fact.shortDescription
                }
                Connections {
                    target: instrumentValueData
                    onFactNameChanged: factNamesCombo.currentIndex = factNamesCombo.find(instrumentValueData.factName)
                }
            }

            QGCRadioButton {
                id:                     iconRadio
                text:                   qsTr("Icon")
                Component.onCompleted:  checked = instrumentValueData.icon != ""
                onClicked: {
                    instrumentValueData.text = ""
                    instrumentValueData.icon = instrumentValueData.factValueGrid.iconNames[0]
                    var updateFunction = function(icon){ instrumentValueData.icon = icon }
                    mainWindow.showPopupDialogFromComponent(iconPickerDialog, { iconNames: instrumentValueData.factValueGrid.iconNames, icon: instrumentValueData.icon, updateIconFunction: updateFunction })
                }
            }

            QGCColoredImage {
                id:                 valueIcon
                Layout.alignment:   Qt.AlignHCenter
                height:             ScreenTools.implicitComboBoxHeight
                width:              height
                source:             "/InstrumentValueIcons/" + (instrumentValueData.icon ? instrumentValueData.icon : instrumentValueData.factValueGrid.iconNames[0])
                sourceSize.height:  height
                fillMode:           Image.PreserveAspectFit
                mipmap:             true
                smooth:             true
                color:              enabled ? qgcPal.text : qgcPalDisabled.text
                enabled:            iconRadio.checked

                MouseArea {
                    anchors.fill:   parent
                    onClicked: {
                        var updateFunction = function(icon){ instrumentValueData.icon = icon }
                        mainWindow.showPopupDialogFromComponent(iconPickerDialog, { iconNames: instrumentValueData.factValueGrid.iconNames, icon: instrumentValueData.icon, updateIconFunction: updateFunction })
                    }
                }

                Rectangle {
                    anchors.fill:   valueIcon
                    color:          qgcPal.text
                    visible:        valueIcon.status === Image.Error
                }
            }

            QGCRadioButton {
                id:                     textRadio
                text:                   qsTr("Text")
                Component.onCompleted:  checked = instrumentValueData.icon == ""
                onClicked: {
                    instrumentValueData.icon = ""
                    instrumentValueData.text = instrumentValueData.fact ? instrumentValueData.fact.shortDescription : qsTr("Label")
                }
            }

            QGCTextField {
                id:                     labelTextField
                Layout.fillWidth:       true
                Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 10
                text:                   instrumentValueData.text
                enabled:                textRadio.checked
                onEditingFinished:      instrumentValueData.text = text
            }

            QGCLabel { text: qsTr("Size") }

            QGCComboBox {
                id:                 fontSizeCombo
                Layout.fillWidth:   true
                model:              instrumentValueData.factValueGrid.fontSizeNames
                currentIndex:       instrumentValueData.factValueGrid.fontSize
                sizeToContents:     true
                onActivated:        instrumentValueData.factValueGrid.fontSize = index
            }

            QGCCheckBox {
                Layout.columnSpan:  2
                text:               qsTr("Show Units")
                checked:            instrumentValueData.showUnits
                onClicked:          instrumentValueData.showUnits = checked
            }

            QGCLabel { text: qsTr("Range") }

            QGCComboBox {
                id:                 rangeTypeCombo
                Layout.fillWidth:   true
                model:              instrumentValueData.rangeTypeNames
                currentIndex:       instrumentValueData.rangeType
                sizeToContents:     true
                onActivated:        instrumentValueData.rangeType = index
            }

            Loader {
                id:                     rangeLoader
                Layout.columnSpan:      2
                Layout.fillWidth:       true
                Layout.preferredWidth:  item ? item.width : 0
                Layout.preferredHeight: item ? item.height : 0

                property var instrumentValueData: root.instrumentValueData

                function updateSourceComponent() {
                    switch (instrumentValueData.rangeType) {
                    case InstrumentValueData.NoRangeInfo:
                        sourceComponent = undefined
                        break
                    case InstrumentValueData.ColorRange:
                        sourceComponent = colorRangeDialog
                        break
                    case InstrumentValueData.OpacityRange:
                        sourceComponent = opacityRangeDialog
                        break
                    case InstrumentValueData.IconSelvalueedectRange:
                        sourceComponent = iconRangeDialog
                        break
                    }
                }

                Component.onCompleted: updateSourceComponent()

                Connections {
                    target:             instrumentValueData
                    onRangeTypeChanged: rangeLoader.updateSourceComponent()
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
                var newValues = instrumentValueData.rangeValues
                newValues[index] = parseFloat(text)
                instrumentValueData.rangeValues = newValues
            }

            function updateColorValue(index, color) {
                var newColors = instrumentValueData.rangeColors
                newColors[index] = color
                instrumentValueData.rangeColors = newColors
            }

            ColorDialog {
                id:             colorPickerDialog
                modality:       Qt.ApplicationModal
                currentColor:   instrumentValueData.rangeColors.length ? instrumentValueData.rangeColors[colorIndex] : "white"
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
                            model: instrumentValueData.rangeValues.length

                            QGCButton {
                                width:      ScreenTools.implicitTextFieldHeight
                                height:     width
                                text:       qsTr("-")
                                onClicked:  instrumentValueData.removeRangeValue(index)
                            }
                        }
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing:                _margins

                        Repeater {
                            model: instrumentValueData.rangeValues.length

                            QGCTextField {
                                text:               instrumentValueData.rangeValues[index]
                                onEditingFinished:  updateRangeValue(index, text)
                            }
                        }
                    }

                    Column {
                        spacing: _margins
                        Repeater {
                            model: instrumentValueData.rangeColors

                            QGCCheckBox {
                                height:     ScreenTools.implicitTextFieldHeight
                                checked:    instrumentValueData.isValidColor(instrumentValueData.rangeColors[index])
                                onClicked:  updateColorValue(index, checked ? "green" : instrumentValueData.invalidColor())
                            }
                        }
                    }

                    Column {
                        spacing: _margins
                        Repeater {
                            model: instrumentValueData.rangeColors

                            Rectangle {
                                width:          ScreenTools.implicitTextFieldHeight
                                height:         width
                                border.color:   qgcPal.text
                                color:          instrumentValueData.isValidColor(modelData) ? modelData : qgcPal.text

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
                    onClicked:  instrumentValueData.addRangeValue()
                }
            }
        }
    }

    Component {
        id: iconRangeDialog

        Item {
            width:  childrenRect.widthvalueed
            height: childrenRect.height

            function updateRangeValue(index, text) {
                var newValues = instrumentValueData.rangeValues
                newValues[index] = parseFloat(text)
                instrumentValueData.rangeValues = newValues
            }

            function updateIconValue(index, icon) {
                var newIcons = instrumentValueData.rangeIcons
                newIcons[index] = icon
                instrumentValueData.rangeIcons = newIcons
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
                            model: instrumentValueData.rangeValues.length

                            QGCButton {
                                width:      ScreenTools.implicitTextFieldHeight
                                height:     width
                                text:       qsTr("-")
                                onClicked:  instrumentValueData.removeRangeValue(index)
                            }
                        }
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing:                _margins

                        Repeater {
                            model: instrumentValueData.rangeValues.length

                            QGCTextField {
                                text:               instrumentValueData.rangeValues[index]
                                onEditingFinished:  updateRangeValue(index, text)
                            }
                        }
                    }

                    Column {
                        spacing: _margins

                        Repeater {
                            model: instrumentValueData.rangeIcons

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
                                        mainWindow.showPopupDialogFromComponent(iconPickerDialog, { iconNames: instrumentValueData.factValueGrid.iconNames, icon: modelData, updateIconFunction: updateFunction })
                                    }
                                }
                            }
                        }
                    }
                }

                QGCButton {
                    text:       qsTr("Add Row")
                    onClicked:  instrumentValueData.addRangeValue()
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
                var newValues = instrumentValueData.rangeValues
                newValues[index] = parseFloat(text)
                instrumentValueData.rangeValues = newValues
            }

            function updateOpacityValue(index, opacity) {
                var newOpacities = instrumentValueData.rangeOpacities
                newOpacities[index] = opacity
                instrumentValueData.rangeOpacities = newOpacities
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
                            model: instrumentValueData.rangeValues.length

                            QGCButton {
                                width:      ScreenTools.implicitTextFieldHeight
                                height:     width
                                text:       qsTr("-")
                                onClicked:  instrumentValueData.removeRangeValue(index)
                            }
                        }
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing:                _margins

                        Repeater {
                            model: instrumentValueData.rangeValues

                            QGCTextField {
                                text:               modelData
                                onEditingFinished:  updateRangeValue(index, text)
                            }
                        }
                    }

                    Column {
                        spacing: _margins

                        Repeater {
                            model: instrumentValueData.rangeOpacities

                            QGCTextField {
                                text:               modelData
                                onEditingFinished:  updateOpacityValue(index, text)
                            }
                        }
                    }
                }

                QGCButton {
                    text:       qsTr("Add Row")
                    onClicked:  instrumentValueData.addRangeValue()
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
}
