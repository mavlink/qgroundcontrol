import QtQuick 2.4
import QtQuick.Controls 1.2
import QtGraphicalEffects 1.0


Item {
    id: _root

    property alias          source:  icon.source
    property bool           checked: false
    property ExclusiveGroup exclusiveGroup:  null

    signal   clicked()

    onExclusiveGroupChanged: {
        if (exclusiveGroup) {
            exclusiveGroup.bindCheckable(_root)
        }
    }

    Image {
        id:             icon
        width:          parent.height * 0.9
        height:         parent.height * 0.9
        mipmap:         true
        fillMode:       Image.PreserveAspectFit
        visible:        false
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
    }

    ColorOverlay {
        id:             iconOverlay
        anchors.fill:   icon
        source:         icon
        color:          (checked ? "#e4e428" : "#ffffff")
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            checked = true
            _root.clicked()
        }
    }
}

/*
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
*/
