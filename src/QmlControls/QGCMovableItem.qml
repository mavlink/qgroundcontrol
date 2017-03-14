import QtQuick 2.3
import QtQuick.Controls 1.2
import QGroundControl.ScreenTools 1.0

// This item can be dragged around within its parent.
// Double click issues a signal the parent can use to
// reset its default position.

Item {
    id: root
    property bool   allowDragging:  true
    property real   minimumWidth:   ScreenTools.defaultFontPixelHeight * (5)
    property real   minimumHeight:  ScreenTools.defaultFontPixelHeight * (5)
    property alias  tForm:          tform
    signal          resetRequested()
    transform: Scale {
        id: tform
    }
    MouseArea {
        property double factor: 25
        enabled:            root.allowDragging
        cursorShape:        Qt.OpenHandCursor
        anchors.fill:       parent
        drag.target:        parent
        drag.axis:          Drag.XAndYAxis
        drag.minimumX:      0
        drag.minimumY:      0
        drag.maximumX:      root.parent.width  - (root.width  * tform.xScale)
        drag.maximumY:      root.parent.height - (root.height * tform.yScale)
        drag.filterChildren: true
        onPressed: {
            root.anchors.left  = undefined
            root.anchors.right = undefined
        }
        onDoubleClicked: {
            root.resetRequested();
        }
        onWheel:
        {
            var zoomFactor = 1;
            if(wheel.angleDelta.y > 0)
                zoomFactor = 1 + (1/factor)
            else
                zoomFactor = 1 - (1/factor)
            var realX = wheel.x * tform.xScale
            var realY = wheel.y * tform.yScale
            var tx = root.x + (1-zoomFactor)*realX
            var ty = root.y + (1-zoomFactor)*realY
            if(tx < 0) tx = 0
            if(ty < 0) ty = 0
            var ts = tform.xScale * zoomFactor
            if(root.width * ts >= root.minimumWidth) {
                if(root.height * ts >= root.minimumHeight) {
                    if(((root.width * ts) + tx) < root.parent.width && ((root.height * ts) + ty) < root.parent.height) {
                        root.x = tx
                        root.y = ty
                        tform.xScale = ts
                        tform.yScale = ts
                    }
                }
            }
        }
    }
}
