import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// One row of a row-based setup page: a separator, an identity column (status dot, title, parameter names, then any
/// children) and the row's cells, which wrap instead of widening the page.
ColumnLayout {
    id: control

    property alias  title:          titleLabel.text
    property var    params:         []
    property bool   active:         false
    property color  activeColor:    qgcPal.colorGreen
    property bool   showSeparator:  false

    default property alias  identityData:   identityColumn.data
    property alias          cellsData:      cellFlow.data

    readonly property real _cellPadding: ScreenTools.defaultFontPixelWidth * 1.5

    Layout.fillWidth:   true
    spacing:            ScreenTools.defaultFontPixelHeight * 0.5

    QGCPalette { id: qgcPal }

    Rectangle {
        Layout.fillWidth:   true
        implicitHeight:     1
        color:              qgcPal.borderSubtle
        visible:            control.showSeparator
    }

    RowLayout {
        Layout.fillWidth:   true
        spacing:            ScreenTools.defaultFontPixelHeight * 0.5

        ColumnLayout {
            id:                     identityColumn
            Layout.alignment:       Qt.AlignTop
            Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 30
            Layout.maximumWidth:    Layout.preferredWidth
            Layout.topMargin:       control._cellPadding
            spacing:                ScreenTools.defaultFontPixelHeight * 0.25

            RowLayout {
                spacing: ScreenTools.defaultFontPixelWidth

                Rectangle {
                    implicitWidth:  ScreenTools.defaultFontPixelHeight * 0.5
                    implicitHeight: implicitWidth
                    radius:         implicitWidth / 2
                    color:          control.active ? control.activeColor : "transparent"
                    border.color:   control.active ? color : qgcPal.textSecondary
                }

                QGCLabel {
                    id:                     titleLabel
                    Layout.fillWidth:       true
                    wrapMode:               Text.WordWrap
                    font.bold:              true
                    font.capitalization:    Font.AllUppercase
                    font.letterSpacing:     ScreenTools.defaultFontPixelWidth * 0.1
                }
            }

            Flow {
                Layout.fillWidth:   true
                spacing:            ScreenTools.defaultFontPixelWidth

                Repeater {
                    model: control.params

                    QGCLabel {
                        text:           modelData
                        font.family:    ScreenTools.fixedFontFamily
                        font.pointSize: ScreenTools.smallFontPointSize
                        color:          qgcPal.textSecondary

                        required property string modelData
                    }
                }
            }
        }

        // Cells wrap instead of widening the page, so the column only needs its widest single cell
        Flow {
            id:                     cellFlow
            Layout.alignment:       Qt.AlignTop
            Layout.fillWidth:       true
            Layout.minimumWidth:    _widestChildWidth
            Layout.preferredWidth:  _widestChildWidth
            spacing:                ScreenTools.defaultFontPixelHeight * 0.5

            readonly property real _widestChildWidth: {
                let widest = 0
                for (let i = 0; i < children.length; i++) {
                    if (children[i].visible) {
                        widest = Math.max(widest, children[i].implicitWidth)
                    }
                }
                return widest
            }
        }
    }
}
