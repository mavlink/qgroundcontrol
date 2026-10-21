import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

Item {
    id: root

    default property alias contentItem: mainLayout.data
    property int sectionFilter: -1

    QGCFlickable {
        objectName:     "settingsPageFlickable"
        anchors.fill:   parent
        contentWidth:   mainLayout.width
        contentHeight:  mainLayout.height

        ColumnLayout {
            id:         mainLayout
            objectName: "settingsPageLayout"
            x:          Math.max(0, root.width / 2 - width / 2)
            width:      Math.min(Math.max(implicitWidth, ScreenTools.defaultFontPixelWidth * 50), root.width)
            spacing:    ScreenTools.defaultFontPixelHeight
        }
    }
}
