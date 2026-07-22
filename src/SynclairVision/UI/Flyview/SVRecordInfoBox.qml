import QtQuick

import QGroundControl
import QGroundControl.Controls

Item {
    id: root

    QGCPalette { id: qgcPalette }

    implicitWidth: background.implicitWidth
    implicitHeight: background.implicitHeight

    SVBackground {
        id: background
        implicitWidth: contentRow.implicitWidth + SVUnits.bigMargin * 2
        implicitHeight: contentRow.implicitHeight + SVUnits.bigMargin * 2

        width: implicitWidth
        height: implicitHeight

        borderWidth: 0
        radius: SVUnits.radius

        Row {
            id: contentRow
            anchors.centerIn: parent
            spacing: SVUnits.bigMargin

            Rectangle {
                width: SVUnits.width
                height: SVUnits.width
                radius: width / 2
                color: qgcPalette.colorRed
                anchors.verticalCenter: parent.verticalCenter
            }

            QGCLabel {
                text: SVState.recordElapsedText
                color: qgcPalette.text
            }
        }
    }

    
        
}
