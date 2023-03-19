import QtQuick          2.15
import QtQuick.Layouts  1.15

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0

ColumnLayout {
    spacing: ScreenTools.defaultFontPixelHeight / 2

    property alias heading: headingLabel.text

    default property alias contentItem: _contentItem.data

    QGCLabel { 
        id:         headingLabel
        visible:    heading !== ""
    }

    Item {
        id:                 _contentItem
        Layout.fillWidth:   true
        Layout.leftMargin:  heading === "" ? 0 : ScreenTools.defaultFontPixelWidth * 5
        implicitWidth:      childrenRect.width
        implicitHeight:     childrenRect.height
    }

    Rectangle {
        height:             1
        Layout.fillWidth:   true
        color:              QGroundControl.globalPalette.text
    }
}
