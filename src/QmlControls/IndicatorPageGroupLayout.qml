import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.ScreenTools

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
