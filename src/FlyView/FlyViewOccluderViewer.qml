pragma ComponentBehavior: Bound

import QtQuick

import QGroundControl
import QGroundControl.Controls

// Development tool which outlines each Fly View occluder; anchor it to fill the widget layer so the coordinates line up
Item {
    id: _root

    property var occluders
    property var customOccluders: []

    component OccluderOutline: Rectangle {
        id: outline

        property rect   occluderRect
        property string label

        x:              occluderRect.x
        y:              occluderRect.y
        width:          occluderRect.width
        height:         occluderRect.height
        color:          "transparent"
        border.color:   "yellow"
        border.width:   2

        QGCLabel {
            anchors.centerIn:   outline
            text:               outline.label
            color:              "yellow"
        }
    }

    Repeater {
        model: _root.occluders ? _root.occluders.types : []

        OccluderOutline {
            required property string modelData

            occluderRect:   _root.occluders[modelData]
            label:          modelData
        }
    }

    Repeater {
        model: _root.customOccluders

        OccluderOutline {
            required property rect modelData

            occluderRect:   modelData
            label:          "custom"
        }
    }
}
