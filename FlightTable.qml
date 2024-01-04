import QtQuick        2.5
import QtQuick.Window 2.5

import QGroundControl.Palette     1.0
import QGroundControl.ScreenTools 1.0

Window {
    width: 640
    height: 480
    color: qgcPal.window

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    ListView {
        id: listView
        anchors.fill: parent
        model: mainWindow.planMasterControllerPlan.missionController.visualItems
        delegate: Rectangle {
            width: listView.width
            height: Math.round(ScreenTools.defaultFontPixelHeight + label.height)
            color:  index % 2 == 0 ? qgcPal.window : qgcPal.windowShade

            Text {
                id: label
                x: ScreenTools.defaultFontPixelWidth
                color: qgcPal.text
                anchors.verticalCenter: parent.verticalCenter
                text: {
                    var point = listView.model.get(index)
                    return point.sequenceNumber + ". " + point.commandName
                }
            }
        }
    }
}
