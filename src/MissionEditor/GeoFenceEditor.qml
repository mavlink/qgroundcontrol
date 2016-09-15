import QtQuick          2.2
import QtQuick.Controls 1.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0

QGCFlickable {
    id:             root
    width:          availableWidth
    height:         Math.min(availableHeight, geoFenceEditorRect.height)
    contentHeight:  geoFenceEditorRect.height
    clip:           true

    readonly property real  _editFieldWidth:    Math.min(width - _margin * 2, ScreenTools.defaultFontPixelWidth * 15)
    readonly property real  _margin:            ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _radius:            ScreenTools.defaultFontPixelWidth / 2

    property var polygon: geoFenceController.polygon

    Rectangle {
        id:     geoFenceEditorRect
        width:  parent.width
        height: geoFenceItems.y + geoFenceItems.height + (_margin * 2)
        radius: _radius
        color:  qgcPal.buttonHighlight

        QGCLabel {
            id:                 geoFenceLabel
            anchors.margins:    _margin
            anchors.left:       parent.left
            anchors.top:        parent.top
            text:               qsTr("GeoFence (WIP careful!)")
            color:              "black"
        }

        Rectangle {
            id:                 geoFenceItems
            anchors.margins:    _margin
            anchors.left:       parent.left
            anchors.right:      parent.right
            anchors.top:        geoFenceLabel.bottom
            height:             editorLoader.y + editorLoader.height + (_margin * 2)
            color:              qgcPal.windowShadeDark
            radius:             _radius

            QGCLabel {
                id:                 geoLabel
                anchors.margins:    _margin
                anchors.top:        parent.top
                anchors.left:       parent.left
                anchors.right:      parent.right
                wrapMode:           Text.WordWrap
                font.pointSize:     ScreenTools.smallFontPointSize
                text:               qsTr("GeoFencing allows you to set a virtual ‘fence’ around the area you want to fly in.")
            }

            Loader {
                id:                 editorLoader
                anchors.margins:    _margin
                anchors.top:        geoLabel.bottom
                anchors.left:       parent.left
                source:             geoFenceController.editorQml

                property real availableWidth: parent.width - (_margin * 2)
            }
        }
    }
}
