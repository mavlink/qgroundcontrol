import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Controls.Private 1.0

import QGroundControl.Palette 1.0

Button {
    id: button
    property bool repaintChevron: false
    property var  __qgcPal: QGCPalette { colorGroupEnabled: enabled }
    style: ButtonStyle {
        background: Item {
            anchors.margins: 3
            Canvas {
                id: chevron
                anchors.fill: parent
                antialiasing: true
                Connections {
                    target: button
                    onHoveredChanged: chevron.requestPaint()
                    onPressedChanged: chevron.requestPaint()
                    onCheckedChanged: chevron.requestPaint()
                    onRepaintChevronChanged: {
                        if(repaintChevron) {
                            chevron.requestPaint()
                            repaintChevron = false;
                        }
                    }
                }
                onPaint: {
                    var vMiddle = height / 2;
                    var context = getContext("2d");
                    context.reset();
                    context.beginPath();
                    context.lineWidth = 6;
                    context.beginPath();
                    context.moveTo(0, 0);
                    context.lineTo(width - 12 - 3, 0);
                    context.lineTo(width - 3, vMiddle);
                    context.lineTo(width - 12 - 3, height);
                    context.lineTo(0, height);
                    context.closePath();
                    context.strokeStyle = __qgcPal.windowShade
                    context.fillStyle = (button.hovered && !button.pressed) ? __qgcPal.buttonHighlight : (button.checked ? __qgcPal.buttonHighlight : __qgcPal.button);
                    context.stroke();
                    context.fill();
                }
            }
        }
        label: Label {
            text: button.text
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: (button.hovered && !button.pressed) ? __qgcPal.buttonHighlightText : (button.checked ? __qgcPal.primaryButtonText : __qgcPal.buttonText)
        }
    }
}
