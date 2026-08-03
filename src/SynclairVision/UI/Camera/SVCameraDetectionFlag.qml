import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls 

Item {
    id: root

    property int radius: SVUnits.radius
    property color normalColor
    property color hoverColor

    property int index

    readonly property var digiview: QGroundControl.digiviewManager


    Canvas {
        id: triangleCanvas
        anchors.fill: parent

        // Properties to dynamic set colors & dimensions
        property color fillColor: mouseArea.containsMouse ? hoverColor : normalColor      // Inner fill color
        property color strokeColor: "white"    // Outline color
        property real strokeWidth: 1             // Outline thickness
        property real cornerRadius: root.radius / 2           // Corner radius

        // Redraw if any visual property changes
        onFillColorChanged: requestPaint()
        onStrokeColorChanged: requestPaint()
        onStrokeWidthChanged: requestPaint()
        onCornerRadiusChanged: requestPaint()

        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();

            var w = width;
            var h = height;
            var r = cornerRadius;
            var pad = strokeWidth / 2; // Offset by half stroke to keep outline sharp inside bounds

            // Three vertices of the half-square triangle (Top-Left, Bottom-Right, Bottom-Left)
            var p1 = { x: pad, y: pad };
            var p2 = { x: w - pad, y: pad };
            var p3 = { x: pad, y: h - pad };

            ctx.beginPath();
            
            // Start near top-left, draw rounded corner to bottom-right
            ctx.moveTo(p1.x + r, p1.y);
            ctx.arcTo(p2.x, p2.y, p3.x, p3.y, r / 2);   // Top-Right to Bottom-Right corner
            ctx.arcTo(p3.x, p3.y, p1.x, p1.y, r / 2);   // Bottom-Right to Bottom-Left corner
            ctx.arcTo(p1.x, p1.y, p2.x, p2.y, r);   // Bottom-Left to Top-Left corner

            ctx.closePath();

            // 1. Fill the inside
            ctx.fillStyle = fillColor;
            ctx.fill();

            // 2. Draw the outline
            if (strokeWidth > 0) {
                ctx.lineWidth = strokeWidth;
                ctx.strokeStyle = strokeColor;
                ctx.lineJoin = "round"; // Ensures smooth outer corners
                ctx.stroke();
            }
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true

        onClicked: (mouse) => {
            digiview.clearDetectionTracking(index)
            SVState.deselectTracking()
        }
    }
}
