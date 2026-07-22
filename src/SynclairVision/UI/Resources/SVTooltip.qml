import QtQuick

import QGroundControl
import QGroundControl.Controls

Item {
    id: root

    property bool isVertical: true
    property bool isLeft: true
    property bool isTop: true
    property string text: ""

    anchors.fill: parent


    SVBackground {
        id: tooltip
        implicitWidth: textItem.implicitWidth + SVUnits.bigMargin * 2
        implicitHeight: textItem.implicitHeight + SVUnits.bigMargin * 2

        width: implicitWidth
        height: implicitHeight

        borderWidth: 0
        radius: SVUnits.radius

        anchors.left: isVertical && isLeft ? parent.right : undefined
        anchors.leftMargin: isVertical && isLeft ? SVUnits.margin : 0

        anchors.right: isVertical && !isLeft ? parent.left : undefined
        anchors.rightMargin: isVertical && !isLeft ? SVUnits.margin : 0

        anchors.top: !isVertical && isTop ? parent.bottom : undefined
        anchors.topMargin: !isVertical && isTop ? SVUnits.margin : 0

        anchors.bottom: !isVertical && !isTop ? parent.top : undefined
        anchors.bottomMargin: !isVertical && !isTop ? SVUnits.margin : 0


        anchors.horizontalCenter: !isVertical ? parent.horizontalCenter : undefined
        anchors.verticalCenter: isVertical ? parent.verticalCenter : undefined

        

        visible: tooltipVisible

        QGCLabel {
            id: textItem
            text: root.text
            font.pointSize: ScreenTools.smallFontPointSize
            color: qgcPalette.text
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}