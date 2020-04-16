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
                instrumentValue.text = instrumentValue.fact.shortDescription
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
                instrumentValue.text = instrumentValue.fact.shortDescription
            }
        }

        QGCRadioButton {
            id:                     iconCheckBox
            text:                   qsTr("Icon")
            Component.onCompleted:  checked = instrumentValue.icon != ""
            onClicked: {
                instrumentValue.text = ""
                instrumentValue.icon = instrumentValue.iconNames[0]
                var updateFunction = function(icon){ instrumentValue.icon = icon }
                mainWindow.showPopupDialog(iconPickerDialog, { iconNames: instrumentValue.iconNames, icon: instrumentValue.icon, updateIconFunction: updateFunction })
            }
        }

        QGCColoredImage {
            Layout.alignment:   Qt.AlignHCenter
            height:             labelPositionCombo.height
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
            id:             labelPositionCombo
            model:          instrumentValue.labelPositionNames
            currentIndex:   instrumentValue.labelPosition
            sizeToContents: true
            onActivated:    instrumentValue.lanelPosition = index
            enabled:        iconCheckBox.checked
        }

        QGCRadioButton {
            id:                     labelCheckBox
            text:                   qsTr("Text")
            Component.onCompleted:  checked = instrumentValue.text != ""
            onClicked: {
                instrumentValue.icon = ""
                instrumentValue.text = instrumentValue.fact ? instrumentValue.fact.shortDescription : qsTr("Label")
            }
        }

        QGCTextField {
            id:                 labelTextField
            Layout.fillWidth:   true
            Layout.columnSpan:  2
            text:               instrumentValue.text
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

            property var instrumentValue: root.instrumentValue

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
                currentColor:   instrumentValue.rangeColors.length ? instrumentValue.rangeColors[colorIndex] : "white"
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
                                        mainWindow.showPopupDialog(iconPickerDialog, { iconNames: instrumentValue.iconNames, icon: modelData, updateIconFunction: updateFunction })
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

