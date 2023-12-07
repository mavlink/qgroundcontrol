import QtQuick          2.15
import QtQuick.Layouts  1.15

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0

ColumnLayout {
    id:         _root
    spacing:    ScreenTools.defaultFontPixelHeight / 2

    property alias heading:     headingLabel.text
    property bool  showDivider: true

    default property alias contentItem: _contentItem.data

    QGCLabel { 
        id:         headingLabel
        visible:    heading !== ""
    }

    ColumnLayout {
        id:                 _contentItem
        Layout.fillWidth:   children[0].Layout.fillWidth
        Layout.leftMargin:  heading === "" ? 0 : ScreenTools.defaultFontPixelWidth * 2
    }

    Rectangle {
        height:             1
        Layout.fillWidth:   true
        color:              QGroundControl.globalPalette.text
        visible:            showDivider
    }
}
