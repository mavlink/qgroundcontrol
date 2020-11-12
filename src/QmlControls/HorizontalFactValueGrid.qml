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
    Layout.preferredWidth:  topLevelRowLayout.width
    Layout.preferredHeight: topLevelRowLayout.height

    property bool   settingsUnlocked:       false

    property real   _margins:               ScreenTools.defaultFontPixelWidth / 2
    property int    _rowMax:                2
    property real   _rowButtonWidth:        ScreenTools.minTouchPixels
    property real   _rowButtonHeight:       ScreenTools.minTouchPixels / 2
    property real   _editButtonSpacing:     2

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    RowLayout {
        id: topLevelRowLayout

        Item {
            id:                     lockItem
            Layout.fillHeight:      true
            Layout.preferredWidth:  ScreenTools.minTouchPixels
            visible:                settingsUnlocked
            enabled:                settingsUnlocked

            QGCColoredImage {
                anchors.centerIn:   parent
                source:             "/res/LockOpen.svg"
                mipmap:             true
                width:              parent.width * 0.75
                height:             width
                sourceSize.width:   width
                color:              qgcPal.text
                fillMode:           Image.PreserveAspectFit
            }

            QGCMouseArea {
                fillItem:   parent
                onClicked:  settingsUnlocked = false
            }
        }

        ColumnLayout {
            Layout.fillHeight:  true

            GridLayout {
                id:         valueGrid
                rows:       _root.columns.count
                rowSpacing: 0

                Repeater {
                    model: _root.columns

                    Repeater {
                        id:     labelRepeater
                        model:  object

                        property real _index: index

                        InstrumentValueLabel {
                            Layout.row:             index
                            Layout.column:          labelRepeater._index * 3
                            Layout.fillHeight:      true
                            Layout.alignment:       Qt.AlignRight
                            instrumentValueData:    object
                        }
                    }
                }

                Repeater {
                    model: _root.columns

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
                            Layout.row:             index
                            Layout.column:          (valueRepeater._index * 3) + 1
                            Layout.fillHeight:      true
                            Layout.alignment:       Qt.AlignLeft
                            Layout.preferredWidth:  maxWidth
                            instrumentValueData:    object

                            property real lastContentWidth

                            Component.onCompleted:  {
                                maxWidth = Math.max(maxWidth, contentWidth)
                                lastContentWidth = contentWidth
                            }

                            onContentWidthChanged: {
                                maxWidth = Math.max(maxWidth, contentWidth)
                                lastContentWidth = contentWidth
                                var currentTime = new Date().getTime()
                                if (currentTime - lastCheck > 30 * 1000) {
                                    lastCheck = currentTime
                                    valueRepeater.recalcWidth()
                                }
                            }
                        }
                    }
                }

                Repeater {
                    model: _root.columns

                    Repeater {
                        id:     spacerRepeater
                        model:  object.count > 1 ? object : 0

                        property real _index: index

                        Item {
                            Layout.row:             index
                            Layout.column:          (spacerRepeater._index * 3) + 2
                            Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth
                            Layout.preferredHeight: 1
                        }
                    }
                }
            }

            RowLayout {
                id:                     rowButtons
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

        ColumnLayout {
            Layout.fillHeight:      true
            Layout.bottomMargin:    rowButtons.height
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

    QGCMouseArea {
        x:          valueGrid.x + lockItem.width + topLevelRowLayout.spacing
        y:          valueGrid.y
        width:      valueGrid.width
        height:     valueGrid.height
        visible:    settingsUnlocked
        onClicked: {
            var item = valueGrid.childAt(mouse.x, mouse.y)
            //console.log(item, item ? item.instrumentValueData : "null", item && item.parent ? item.parent.instrumentValueData : "null")
            if (item && item.instrumentValueData !== undefined) {
                mainWindow.showPopupDialogFromComponent(valueEditDialog, { instrumentValueData: item.instrumentValueData })
            }
        }

        /*Rectangle {
            anchors.fill: parent
            border.color: "green"
            border.width: 1
            color: "transparent"
        }*/
    }

    Component {
        id: valueEditDialog

        InstrumentValueEditDialog { }
    }
}

