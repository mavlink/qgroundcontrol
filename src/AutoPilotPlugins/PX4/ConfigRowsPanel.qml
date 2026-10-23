import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Rounded panel holding a row-based setup page. Children are laid out in a column.
Rectangle {
    default property alias panelData: panelLayout.data

    readonly property real _margins: ScreenTools.defaultFontPixelHeight * 0.5

    Layout.fillWidth:   true
    implicitWidth:      panelLayout.implicitWidth + _margins * 2
    implicitHeight:     panelLayout.implicitHeight + _margins * 2
    color:              qgcPal.windowPanel
    border.color:       qgcPal.borderSubtle
    radius:             ScreenTools.defaultBorderRadius * 2

    QGCPalette { id: qgcPal }

    ColumnLayout {
        id:                 panelLayout
        anchors.fill:       parent
        anchors.margins:    _margins
        spacing:            _margins
    }
}
