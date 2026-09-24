import QtQuick
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.Controls

/// Marker for the connected RTK base station's antenna position
MapQuickItem {
    id: root

    property var map

    readonly property var _receiver: QGroundControl.gpsManager.gpsRtk
    readonly property bool _final: root._receiver.basePositionFinal

    coordinate: root._receiver.basePosition
    visible: coordinate.isValid
    anchorPoint.x: badge.width / 2
    anchorPoint.y: badge.height / 2

    QGCPalette { id: qgcPal }

    sourceItem: Item {
        width: Math.max(badge.width, baseLabel.width)
        height: badge.height + baseLabel.height

        Rectangle {
            id: badge
            objectName: "rtkBaseMarker"
            anchors.horizontalCenter: parent.horizontalCenter
            width: ScreenTools.defaultFontPixelHeight * 1.5
            height: width
            radius: width / 2
            color: root._final ? qgcPal.mapIndicator : qgcPal.mapIndicatorChild
            border.color: qgcPal.mapWidgetBorderDark
            border.width: 1

            QGCColoredImage {
                anchors.centerIn: parent
                width: parent.width * 0.65
                height: width
                source: "/qmlimages/Gps.svg"
                sourceSize.height: height
                fillMode: Image.PreserveAspectFit
                color: qgcPal.mapWidgetBorderLight
            }
        }

        QGCMapLabel {
            id: baseLabel
            anchors.top: badge.bottom
            anchors.horizontalCenter: badge.horizontalCenter
            map: root.map
            font.pointSize: ScreenTools.smallFontPointSize
            text: root._final ? qsTr("RTK Base") : qsTr("RTK Base (surveying)")
        }
    }
}
