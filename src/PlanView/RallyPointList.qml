import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// List of the rally points of the Rally Points tab
QGCListView {
    id: root

    required property var page

    signal itemActivated

    objectName: "planPanel_rallyList"
    model:      page.points
    spacing:    ScreenTools.defaultFontPixelHeight * 0.1
    clip:       true

    delegate: PlanPanelRow {
        id: row

        required property var object
        required property int index

        objectName: "planPanel_rallyRow_" + index
        width:      root.width
        selected:   object === root.page.current

        onClicked: {
            if (!mainWindow.allowViewSwitch()) {
                return
            }
            root.page.select(object)
            root.itemActivated()
        }

        Rectangle {
            Layout.preferredWidth:  ScreenTools.defaultFontPixelHeight * 1.5
            Layout.preferredHeight: Layout.preferredWidth
            radius:                 width / 2
            color:                  row.selected ? qgcPal.accent : "transparent"
            border.width:           2
            border.color:           row.selected ? qgcPal.accent : qgcPal.textPrimary

            QGCLabel {
                anchors.centerIn:   parent
                text:               String(row.index + 1)
                font.pointSize:     ScreenTools.smallFontPointSize
                font.bold:          true
                color:              row.selected ? qgcPal.accentText : qgcPal.textPrimary
            }
        }

        QGCLabel {
            Layout.fillWidth:   true
            elide:              Text.ElideRight
            text:               qsTr("%1, %2").arg(row.object.coordinate.latitude.toFixed(5)).arg(row.object.coordinate.longitude.toFixed(5))
        }

        QGCLabel {
            color:              qgcPal.textSecondary
            font.pointSize:     ScreenTools.smallFontPointSize
            text:               QGroundControl.unitsConversion.metersToAppSettingsVerticalDistanceUnitsString(row.object.coordinate.altitude)
        }
    }

    QGCPalette { id: qgcPal }
}
