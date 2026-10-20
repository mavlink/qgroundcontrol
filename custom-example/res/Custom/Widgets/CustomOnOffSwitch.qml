import QtQuick


import QGroundControl
import QGroundControl.Controls

Rectangle {
    id:     _root
    height: Math.round(ScreenTools.defaultFontPixelHeight * 2)
    width:  ScreenTools.defaultFontPixelWidth  * 10
    color:  qgcPal.windowCard
    border.color: qgcPal.borderSubtle
    border.width: 0

    property bool checked: true

    signal  clicked

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    Rectangle {
        width:      parent.width  * 0.5
        height:     parent.height
        color:      checked ? qgcPal.windowCard : qgcPal.accent
        border.color: qgcPal.textPrimary
        border.width: 0
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        QGCLabel {
            text: qsTr("Off")
            anchors.centerIn: parent
            color:      qgcPal.textPrimary
        }
    }
    Rectangle {
        width:      parent.width  * 0.5
        height:     parent.height
        color:      checked ? qgcPal.accent : qgcPal.windowCard
        border.color: qgcPal.textPrimary
        border.width: 0
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        QGCLabel {
            text:       qsTr("On")
            color:      qgcPal.accentText
            anchors.centerIn: parent
        }
    }
    MouseArea {
        anchors.fill:   parent
        onClicked: {
            checked = !checked
            _root.clicked()
        }
    }
}
