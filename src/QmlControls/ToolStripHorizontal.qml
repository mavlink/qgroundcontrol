import QtQuick          2.11
import QtQuick.Controls 2.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0

Rectangle {
    id:         _root
    color:      qgcPal.toolbarBackground
    width:      Math.min(maxWidth, toolStripRow.width + (flickable.anchors.margins * 2))
    height:      _idealHeight < repeater.contentHeight ? repeater.contentHeight : _idealHeight
    radius:     ScreenTools.defaultFontPixelWidth / 2

    property alias  model:              repeater.model
    property real   maxWidth            ///< Maximum width for control, determines whether text is hidden to make control shorter
    property alias  title:              titleLabel.text
    property var    fontSize:           ScreenTools.smallFontPointSize
    property var    forceImageScale11:  false

    property var _dropPanel: dropPanel

    function simulateClick(buttonIndex) {
        buttonIndex = buttonIndex + 1 // skip over title label
        var button = toolStripRow.children[buttonIndex]
        if (button.checkable) {
            button.checked = !button.checked
        }
        button.clicked()
    }

    // Ensure we don't get narrower than content
    property real _idealHeight: (ScreenTools.isMobile ? ScreenTools.minTouchPixels : ScreenTools.defaultFontPixelWidth * 8) + toolStripRow.anchors.margins * 2

    signal dropped(int index)

    DeadMouseArea {
        anchors.fill: parent
    }

    QGCFlickable {
        id:                 flickable
        anchors.margins:    ScreenTools.defaultFontPixelWidth * 0.4
        anchors.top:        parent.top
        anchors.left:       parent.left
        anchors.bottom:     parent.bottom
        width:              parent.width
        contentWidth:       toolStripRow.width
        flickableDirection: Flickable.HorizontalFlick
        clip:               true

        Row {
            id:             toolStripRow
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            spacing:        ScreenTools.defaultFontPixelWidth * 0.25

            QGCLabel {
                id:                     titleLabel
                anchors.top:            parent.top
                anchors.bottom:         parent.bottom
                width:                  parent.height
                horizontalAlignment:    Text.AlignHCenter
                verticalAlignment:      Text.AlignVCenter
                font.pointSize:         ScreenTools.smallFontPointSize
                visible:                title != ""
                wrapMode:               Text.Wrap
                maximumLineCount:       2
                elide:                  Text.ElideRight
            }

            Repeater {
                id: repeater

                ToolStripHoverButton {
                    id:                 buttonTemplate
                    anchors.top:        toolStripRow.top
                    anchors.bottom:     toolStripRow.bottom
                    width:              height
                    radius:             ScreenTools.defaultFontPixelWidth / 2
                    fontPointSize:      _root.fontSize
                    toolStripAction:    modelData
                    dropPanel:          _dropPanel
                    onDropped:          _root.dropped(index)
                    forceImageScale11:  _root.forceImageScale11

                    onCheckedChanged: {
                        // We deal with exclusive check state manually since usinug autoExclusive caused all sorts of crazt problems
                        if (checked) {
                            for (var i=0; i<repeater.count; i++) {
                                if (i != index) {
                                    var button = repeater.itemAt(i)
                                    if (button.checked) {
                                        button.checked = false
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    DropPanel {
        id:         dropPanel
        toolStrip:  _root
    }
}
