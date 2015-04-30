import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Controls.Private 1.0

import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0

QGCButton {
    id: button
    property bool repaintChevron: false
    property var  __qgcPal: QGCPalette { colorGroupEnabled: enabled }
    property bool showHighlight: __showHighlight
    style: ButtonStyle {
        background: Item {
            anchors.margins: height * 0.1 // 3
            Canvas {
                id: chevron
                anchors.fill: parent
                antialiasing: true
                Connections {
                    target: button
                    onHoveredChanged: chevron.requestPaint()
                    onPressedChanged: chevron.requestPaint()
                    onCheckedChanged: chevron.requestPaint()
                    onShowHighlightChanged: chevron.requestPaint()
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
                    var w12 = button.height * 0.4 // 12
                    var w3  = button.height * 0.1 // 3
                    var w15 = w12 + w3
                    context.reset();
                    context.beginPath();
                    context.lineWidth = button.height * 0.2; // 6
                    context.beginPath();
                    context.moveTo(0, 0);
                    context.lineTo(width - w15, 0);
                    context.lineTo(width - w3,  vMiddle);
                    context.lineTo(width - w15, height);
                    context.lineTo(0, height);
                    context.closePath();
                    context.strokeStyle = __qgcPal.windowShade
                    context.fillStyle = showHighlight ? __qgcPal.buttonHighlight : (button.checked ? __qgcPal.buttonHighlight : __qgcPal.button);
                    context.stroke();
                    context.fill();
                }
            }
        }
        label: QGCLabel {
            text: button.text
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment:   Text.AlignVCenter
            color: showHighlight ? __qgcPal.buttonHighlightText : (button.checked ? __qgcPal.primaryButtonText : __qgcPal.buttonText)
        }
    }
}
