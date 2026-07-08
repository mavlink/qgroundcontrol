/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Palette
import QGroundControl.Controls

Rectangle {
    id:         _root
    color:      "transparent"
    width:      Math.max(ScreenTools.defaultFontPixelWidth * 9.0 * sizeScale,
                         ScreenTools.minTouchPixels * 1.38 * sizeScale)
    height:     Math.min(maxHeight, toolStripColumn.height + (flickable.anchors.margins * 2))
    radius:     Math.round(ScreenTools.defaultFontPixelWidth * 0.78)
    border.color: "transparent"
    border.width: 0

    property alias  model:              repeater.model
    property real   maxHeight           ///< Maximum height for control, determines whether text is hidden to make control shorter
    property alias  title:              titleLabel.text
    property var    fontSize:           ScreenTools.titleFontPointSize
    property var    backdropSourceItem: null
    property real   sizeScale:          1.0

    property var _dropPanel: dropPanel

    QGCPalette { id: qgcPal }

    GlassBackdrop {
        anchors.fill:       parent
        sourceItem:         _root.backdropSourceItem
        backdropBlurEnabled:true
        targetItem:         _root
        cornerRadius:       _root.radius
        sourceScale:        0.46
        blurAmount:         0.94
        blurMax:            42
        sourceBrightness:   -0.01
        sourceSaturation:   0.62
        tintColor:          Qt.rgba(0.045, 0.048, 0.052, 0.68)
        sheenColor:         "transparent"
    }

    function simulateClick(buttonIndex) {
        buttonIndex = buttonIndex + 1 // skip over title label
        var button = toolStripColumn.children[buttonIndex]
        if (button.checkable) {
            button.checked = !button.checked
        }
        button.clicked()
    }

    signal dropped(int index)

    DeadMouseArea {
        anchors.fill: parent
    }

    QGCFlickable {
        id:                 flickable
        anchors.margins:    ScreenTools.defaultFontPixelWidth * 0.36 * _root.sizeScale
        anchors.top:        parent.top
        anchors.left:       parent.left
        anchors.right:      parent.right
        height:             parent.height - anchors.margins * 2
        contentHeight:      toolStripColumn.height
        flickableDirection: Flickable.VerticalFlick
        clip:               true

        Column {
            id:             toolStripColumn
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        ScreenTools.defaultFontPixelWidth * 0.28 * _root.sizeScale

            QGCLabel {
                id:                     titleLabel
                anchors.left:           parent.left
                anchors.right:          parent.right
                horizontalAlignment:    Text.AlignHCenter
                font.pointSize:         ScreenTools.captionFontPointSize
                color:                  qgcPal.buttonText
                visible:                title != ""
            }

            Repeater {
                id: repeater

                ToolStripHoverButton {
                    id:                 buttonTemplate
                    anchors.left:       toolStripColumn.left
                    anchors.right:      toolStripColumn.right
                    height:             Math.max(ScreenTools.minTouchPixels * 0.98 * _root.sizeScale,
                                                 Math.min(toolStripColumn.width * 0.96,
                                                          ScreenTools.defaultFontPixelHeight * 3.60 * _root.sizeScale))
                    radius:             Math.round(ScreenTools.defaultFontPixelWidth * 0.30 * _root.sizeScale)
                    fontPointSize:      _root.fontSize * _root.sizeScale
                    toolStripAction:    modelData
                    dropPanel:          _dropPanel
                    onDropped: (index) => _root.dropped(index)

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

    ToolStripDropPanel {
        id:         dropPanel
        toolStrip:  _root
    }
}
