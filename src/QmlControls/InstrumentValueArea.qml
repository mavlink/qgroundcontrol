/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.12
import QtQuick.Layouts  1.2
import QtQuick.Controls 2.5
import QtQml            2.12

import QGroundControl.Templates     1.0 as T
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0
import QGroundControl.Palette       1.0
import QGroundControl.FlightMap     1.0
import QGroundControl               1.0

T.InstrumentValueArea {
    id:     _root
    height: innerColumn.height

    property bool   settingsUnlocked:               false

    property real   _margins:                       ScreenTools.defaultFontPixelWidth / 2
    property int    _colMax:                        4
    property real   _columnButtonWidth:             ScreenTools.minTouchPixels / 2
    property real   _columnButtonHeight:            ScreenTools.minTouchPixels
    property real   _columnButtonSpacing:           2
    property real   _columnButtonsTotalHeight:      (_columnButtonHeight * 2) + _columnButtonSpacing

    Column {
        id:         innerColumn
        width:      parent.width
        spacing:    Math.round(ScreenTools.defaultFontPixelHeight / 2)

        Repeater {
            id:     rowRepeater
            model:  rowValues

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

                        property real _interColumnSpacing:  (columnRepeater.count - (settingsUnlocked ? 0 : 1)) * columnRow.spacing
                        property real columnWidth:          (pageWidth - (settingsUnlocked ? _columnButtonWidth : 0) - _interColumnSpacing) / columnRepeater.count
                        property bool componentCompleted:   false

                        Component.onCompleted: componentCompleted = true
                        onItemAdded: valueItemMouseAreaComponent.createObject(item, { "instrumentValueData": object.get(index), "rowIndex": index })

                        InstrumentValue {
                            id:                     columnItem
                            anchors.verticalCenter: parent.verticalCenter
                            width:                  columnRepeater.columnWidth
                            recalcOk:               columnRepeater.componentCompleted
                            instrumentValueData:    object
                        }
                    } // Repeater - columns

                    ColumnLayout {
                        id:                 columnsButtonsLayout
                        width:              _columnButtonWidth
                        spacing:            _columnButtonSpacing
                        visible:            settingsUnlocked

                        QGCButton {
                            Layout.fillHeight:      true
                            Layout.preferredHeight: ScreenTools.minTouchPixels
                            Layout.preferredWidth:  parent.width
                            text:                   qsTr("+")
                            onClicked:              appendColumn(rowRepeaterLayout.rowIndex)
                        }

                        QGCButton {
                            Layout.fillHeight:      true
                            Layout.preferredHeight: ScreenTools.minTouchPixels
                            Layout.preferredWidth:  parent.width
                            text:                   qsTr("-")
                            enabled:                index !== 0 || columnRepeater.count !== 1
                            onClicked:              deleteLastColumn(rowRepeaterLayout.rowIndex)
                        }
                    }
                } // RowLayout

                RowLayout {
                    width:      parent.width
                    height:     ScreenTools.defaultFontPixelWidth * 2
                    spacing:    1
                    visible:    settingsUnlocked

                    QGCButton {
                        Layout.fillWidth:   true
                        Layout.preferredHeight: ScreenTools.defaultFontPixelWidth * 2
                        text:               qsTr("+")
                        onClicked:          insertRow(index + 1)
                    }

                    QGCButton {
                        Layout.fillWidth:   true
                        Layout.preferredHeight: ScreenTools.defaultFontPixelWidth * 2
                        text:               qsTr("-")
                        enabled:            index !== 0
                        onClicked:          deleteRow(index)
                    }
                }
            }
        } // Repeater - rows

        QGCButton {
            anchors.left:   parent.left
            anchors.right:  parent.right
            text:           qsTr("Reset To Defaults")
            visible:        settingsUnlocked
            onClicked:      resetToDefaults()
        }

        Component {
            id: valueItemMouseAreaComponent

            MouseArea {
                anchors.centerIn:   parent
                width:              parent.width
                height:             _columnButtonsTotalHeight
                visible:            settingsUnlocked

                property var instrumentValueData
                property int rowIndex

                onClicked: mainWindow.showPopupDialog(valueEditDialog, { instrumentValueData: instrumentValueData })
            }
        }

        Component {
            id: valueEditDialog

            InstrumentValueEditDialog { }
        }
    }
}
