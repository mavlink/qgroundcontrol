import QtQuick

import QGroundControl
import QGroundControl.Controls

Item {
    id: root

    property real size: Math.min(root.width, root.height)
    property bool isHorizontal: root.width >= root.height
    property int widthAmount:  root.height > 0 ? Math.floor(root.width / root.height) : 0
    property int heightAmount: root.width  > 0 ? Math.floor(root.height / root.width) : 0
    property bool legitValues: width > 0 && height > 0
    property var immediateSttHandler

    QGCPalette { id: qgcPalette }

    Repeater {
        model: {
            if(root.legitValues) {
                return root.isHorizontal ? root.widthAmount : root.heightAmount
            } else {
                return 0
            }
        }

        delegate: Item {
            required property int index

            width: root.size
            height: root.size
            x: root.isHorizontal ? index * width : 0
            y: !root.isHorizontal ? index * height : 0

            SVFlyViewDetectionButton {
                anchors.fill: parent
                detectionViewId: index
                immediateSttHandler: root.immediateSttHandler
                visible: SVState.hud
            }

            Rectangle {
                width: root.isHorizontal ? SVUnits.lineWidth : root.width
                height: !root.isHorizontal ? SVUnits.lineWidth : root.height
                x: root.isHorizontal ? height : 0
                y: !root.isHorizontal ? width : 0

                color: qgcPalette.windowShadeLight
            }
        }
    }

    Rectangle {
        x: root.isHorizontal ? root.widthAmount * root.height - 1: 0
        y: !root.isHorizontal ? root.heightAmount * root.width - 1: 0
        width: root.isHorizontal ? root.width % root.height : root.width 
        height: !root.isHorizontal ? root.height % root.width : root.height
                
        color: "black"
        border.width: 1
        border.color: qgcPalette.windowShadeLight
        visible: root.legitValues
    }

    SVBorder {
        id: cameraBorder
        anchors.fill: parent
        borderWidth: SVUnits.lineWidth * 1
        borderColor: qgcPalette.windowShadeLight
        borderVisible: true
    }

    

}
