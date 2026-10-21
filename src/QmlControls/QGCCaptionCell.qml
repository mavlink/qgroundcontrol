import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Card with an optional caption above its contents. Children are laid out in a column.
Rectangle {
    id: control

    default property alias cellData:    cellLayout.data
    property alias caption:             captionLabel.text

    readonly property real _padding: ScreenTools.defaultFontPixelWidth * 1.5

    implicitWidth:  cellLayout.implicitWidth + _padding * 2
    implicitHeight: cellLayout.implicitHeight + _padding * 2
    color:          qgcPal.windowCard
    border.color:   qgcPal.borderSubtle
    radius:         ScreenTools.defaultBorderRadius * 2

    QGCPalette { id: qgcPal; colorGroupEnabled: control.enabled }

    ColumnLayout {
        id:                 cellLayout
        anchors.fill:       parent
        anchors.margins:    control._padding
        spacing:            ScreenTools.defaultFontPixelHeight * 0.25

        QGCCaptionLabel {
            id:         captionLabel
            visible:    text !== ""
        }
    }
}
