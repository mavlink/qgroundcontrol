/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                      2.11
import QtQuick.Controls             2.4
import QtQuick.Dialogs              1.3
import QtQuick.Layouts              1.11

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0

Item {
    id:                         outputRoot
    width:                      availableWidth
    height:                     mainGrid.height + (ScreenTools.defaultFontPixelHeight * 2)

    readonly property real      blockWidth:     outputRoot.width * 0.45
    readonly property real      blockHeight:    ScreenTools.defaultFontPixelHeight * 12

    GridLayout {
        id:                     mainGrid
        columns:                3
        columnSpacing:          ScreenTools.defaultFontPixelWidth * 2
        rowSpacing:             ScreenTools.defaultFontPixelHeight
        y:                      ScreenTools.defaultFontPixelHeight * 0.5
        width:                  parent.width
        //-- Main
        QGCLabel {
            text:               qsTr("MAIN")
            rotation:           270
            font.pointSize:     ScreenTools.largeFontPointSize
            Layout.alignment:   Qt.AlignVCenter | Qt.AlignHCenter
        }
        ColumnLayout {
            spacing:            ScreenTools.defaultFontPixelHeight * 0.5
            Layout.fillWidth:   true
            QGCTextField {
                id:             mainText
                Layout.minimumHeight:blockHeight
                Layout.minimumWidth: blockWidth
                Layout.fillWidth: true
            }
            QGCButton {
                text:           qsTr("Save Onboard")
                Layout.alignment: Qt.AlignRight
            }
        }
        //-- Table
        GridLayout {
            columns:            6
            columnSpacing:      ScreenTools.defaultFontPixelWidth
            rowSpacing:         ScreenTools.defaultFontPixelHeight
            Layout.fillWidth:   true
            Layout.alignment:   Qt.AlignTop
            QGCLabel {
                text:           qsTr("DIS")
            }
            QGCLabel {
                text:           qsTr("MIN")
            }
            QGCLabel {
                text:           qsTr("MAX")
            }
            QGCLabel {
                text:           qsTr("")
            }
            QGCLabel {
                text:           qsTr("CUR")
            }
            Repeater {
                model:                  5
                delegate: QGCLabel {
                    Layout.row:         index + 2
                    Layout.column:      0
                    Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 6
                    text:               "900"
                    Layout.alignment:   Qt.AlignVCenter
                }
            }
            Repeater {
                model:                  5
                delegate: QGCLabel {
                    Layout.row:         index + 2
                    Layout.column:      1
                    Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 6
                    text:               "1100"
                    Layout.alignment:   Qt.AlignVCenter
                }
            }
            Repeater {
                model:                  5
                delegate: QGCLabel {
                    Layout.row:         index + 2
                    Layout.column:      2
                    Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 6
                    text:               "1900"
                    Layout.alignment:   Qt.AlignVCenter
                }
            }
            Repeater {
                model:                  5
                delegate: QGCButton {
                    Layout.row:         index + 2
                    Layout.column:      3
                    Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 6
                    text:               "MAIN" + (index + 1)
                    Layout.alignment:   Qt.AlignVCenter
                }
            }
            Repeater {
                model:                  5
                delegate: QGCLabel {
                    Layout.row:         index + 2
                    Layout.column:      4
                    Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 6
                    text:               "1500"
                    Layout.alignment:   Qt.AlignVCenter
                }
            }
        }
        //-- Separator
        Rectangle {
            Layout.fillWidth:   true
            height:             1
            color:              qgcPal.text
            Layout.columnSpan:  3
        }
        //-- AUX
        QGCLabel {
            text:               qsTr("AUX")
            rotation:           270
            font.pointSize:     ScreenTools.largeFontPointSize
            Layout.alignment:   Qt.AlignVCenter | Qt.AlignHCenter
        }
        ColumnLayout {
            spacing:            ScreenTools.defaultFontPixelHeight * 0.5
            Layout.fillWidth:   true
            QGCTextField {
                id:             auxText
                Layout.minimumHeight:blockHeight
                Layout.minimumWidth: blockWidth
                Layout.fillWidth: true
            }
            QGCButton {
                text:           qsTr("Save Onboard")
                Layout.alignment: Qt.AlignRight
            }
        }
        //-- Table
        GridLayout {
            columns:            6
            columnSpacing:      ScreenTools.defaultFontPixelWidth
            rowSpacing:         ScreenTools.defaultFontPixelHeight
            Layout.fillWidth:   true
            Layout.alignment:   Qt.AlignTop
            QGCLabel {
                text:           qsTr("DIS")
            }
            QGCLabel {
                text:           qsTr("MIN")
            }
            QGCLabel {
                text:           qsTr("MAX")
            }
            QGCLabel {
                text:           qsTr("")
            }
            QGCLabel {
                text:           qsTr("CUR")
            }
            Repeater {
                model:                  5
                delegate: QGCLabel {
                    Layout.row:         index + 2
                    Layout.column:      0
                    Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 6
                    text:               "900"
                    Layout.alignment:   Qt.AlignVCenter
                }
            }
            Repeater {
                model:                  5
                delegate: QGCLabel {
                    Layout.row:         index + 2
                    Layout.column:      1
                    Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 6
                    text:               "1100"
                    Layout.alignment:   Qt.AlignVCenter
                }
            }
            Repeater {
                model:                  5
                delegate: QGCLabel {
                    Layout.row:         index + 2
                    Layout.column:      2
                    Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 6
                    text:               "1900"
                    Layout.alignment:   Qt.AlignVCenter
                }
            }
            Repeater {
                model:                  5
                delegate: QGCButton {
                    Layout.row:         index + 2
                    Layout.column:      3
                    Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 6
                    text:               "MAIN" + (index + 1)
                    Layout.alignment:   Qt.AlignVCenter
                }
            }
            Repeater {
                model:                  5
                delegate: QGCLabel {
                    Layout.row:         index + 2
                    Layout.column:      4
                    Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 6
                    text:               "1500"
                    Layout.alignment:   Qt.AlignVCenter
                }
            }
        }
    }
}


