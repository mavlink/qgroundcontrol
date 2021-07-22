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

// Note: This control will spit out qWarnings like this: "QGridLayoutEngine::addItem: Cell (0, 1) already taken"
// This is due to Qt bug https://bugreports.qt.io/browse/QTBUG-65121
// If this becomes a problem I'll implement our own grid layout control

T.HorizontalFactValueGrid {
    id:                     _root
    Layout.preferredWidth:  topLayout.width
    Layout.preferredHeight: topLayout.height

    property bool   settingsUnlocked:       false

    property real   _margins:               ScreenTools.defaultFontPixelWidth / 2
    property int    _rowMax:                2
    property real   _rowButtonWidth:        ScreenTools.minTouchPixels
    property real   _rowButtonHeight:       ScreenTools.minTouchPixels / 2
    property real   _editButtonSpacing:     2

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    ColumnLayout {
        id:         topLayout
        spacing:    0

        RowLayout {
            RowLayout {
                id:         labelValueColumnLayout
                spacing:    ScreenTools.defaultFontPixelWidth * 1.25

                Repeater {
                    model: _root.columns

                    GridLayout {
                        rows:           object.count
                        columns:        2
                        rowSpacing:     0
                        columnSpacing:  ScreenTools.defaultFontPixelWidth / 4
                        flow:           GridLayout.TopToBottom

                        Repeater {
                            id:     labelRepeater
                            model:  object

                            InstrumentValueLabel {
                                Layout.fillHeight:      true
                                Layout.alignment:       Qt.AlignRight
                                instrumentValueData:    object
                            }
                        }

                        Repeater {
                            id:     valueRepeater
                            model:  object

                            property real   _index:     index
                            property real   maxWidth:   0
                            property var    lastCheck:  new Date().getTime()

                            function recalcWidth() {
                                var newMaxWidth = 0
                                for (var i=0; i<valueRepeater.count; i++) {
                                    newMaxWidth = Math.max(newMaxWidth, valueRepeater.itemAt(0).contentWidth)
                                }
                                maxWidth = Math.min(maxWidth, newMaxWidth)
                            }

                            InstrumentValueValue {
                                Layout.fillHeight:      true
                                Layout.alignment:       Qt.AlignLeft
                                Layout.preferredWidth:  valueRepeater.maxWidth
                                instrumentValueData:    object

                                property real lastContentWidth

                                Component.onCompleted:  {
                                    valueRepeater.maxWidth = Math.max(valueRepeater.maxWidth, contentWidth)
                                    lastContentWidth = contentWidth
                                }

                                onContentWidthChanged: {
                                    valueRepeater.maxWidth = Math.max(valueRepeater.maxWidth, contentWidth)
                                    lastContentWidth = contentWidth
                                    var currentTime = new Date().getTime()
                                    if (currentTime - valueRepeater.lastCheck > 30 * 1000) {
                                        valueRepeater.lastCheck = currentTime
                                        valueRepeater.recalcWidth()
                                    }
                                }
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.bottomMargin:    1
                Layout.fillHeight:      true
                Layout.preferredWidth:  ScreenTools.minTouchPixels / 2
                spacing:                1
                visible:                settingsUnlocked
                enabled:                settingsUnlocked

                QGCButton {
                    Layout.fillHeight:      true
                    Layout.preferredHeight: ScreenTools.minTouchPixels
                    Layout.preferredWidth:  parent.width
                    text:                   qsTr("+")
                    onClicked:              appendColumn()
                }

                QGCButton {
                    Layout.fillHeight:      true
                    Layout.preferredHeight: ScreenTools.minTouchPixels
                    Layout.preferredWidth:  parent.width
                    text:                   qsTr("-")
                    enabled:                _root.columns.count > 1
                    onClicked:              deleteLastColumn()
                }
            }
        }

        RowLayout {
            Layout.preferredHeight: ScreenTools.minTouchPixels / 2
            Layout.fillWidth:       true
            spacing:                1
            visible:                settingsUnlocked
            enabled:                settingsUnlocked

            QGCButton {
                Layout.fillWidth:       true
                Layout.preferredHeight: parent.height
                text:                   qsTr("+")
                onClicked:              appendRow()
            }

            QGCButton {
                Layout.fillWidth:       true
                Layout.preferredHeight: parent.height
                text:                   qsTr("-")
                enabled:                _root.rowCount > 1
                onClicked:              deleteLastRow()
            }
        }
    }

    QGCMouseArea {
        x:          labelValueColumnLayout.x
        y:          labelValueColumnLayout.y
        width:      labelValueColumnLayout.width
        height:     labelValueColumnLayout.height
        visible:    settingsUnlocked
        cursorShape:Qt.PointingHandCursor

        property var mappedLabelValueColumnLayoutPosition: _root.mapFromItem(labelValueColumnLayout, labelValueColumnLayout.x, labelValueColumnLayout.y)

        onClicked: {
            var columnGridLayoutItem = labelValueColumnLayout.childAt(mouse.x, mouse.y)
            //console.log(mouse.x, mouse.y, columnGridLayoutItem)
            var mappedMouse = labelValueColumnLayout.mapToItem(columnGridLayoutItem, mouse.x, mouse.y)
            var labelOrDataItem = columnGridLayoutItem.childAt(mappedMouse.x, mappedMouse.y)
            //console.log(mappedMouse.x, mappedMouse.y, labelOrDataItem, labelOrDataItem ? labelOrDataItem.instrumentValueData : "null", labelOrDataItem && labelOrDataItem.parent ? labelOrDataItem.parent.instrumentValueData : "null")
            if (labelOrDataItem && labelOrDataItem.instrumentValueData !== undefined) {
                mainWindow.showPopupDialogFromComponent(valueEditDialog, { instrumentValueData: labelOrDataItem.instrumentValueData })
            }
        }
    }

    Component {
        id: valueEditDialog

        InstrumentValueEditDialog { }
    }
}
