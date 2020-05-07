/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.11
import QtQuick.Controls 2.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0

Rectangle {
    id:         _root
    color:      qgcPal.globalTheme === QGCPalette.Light ? QGroundControl.corePlugin.options.toolbarBackgroundLight : QGroundControl.corePlugin.options.toolbarBackgroundDark
    width:      _idealWidth < repeater.contentWidth ? repeater.contentWidth : _idealWidth
    height:     Math.min(maxHeight, toolStripColumn.height + (flickable.anchors.margins * 2))
    radius:     ScreenTools.defaultFontPixelWidth / 2

    property alias  model:              repeater.model
    property real   maxHeight           ///< Maximum height for control, determines whether text is hidden to make control shorter
    property alias  title:              titleLabel.text

    function simulateClick(buttonIndex) {
        buttonIndex = buttonIndex + 1 // skip over title
        toolStripColumn.children[buttonIndex].clicked()
    }

    // Ensure we don't get narrower than content
    property real _idealWidth: (ScreenTools.isMobile ? ScreenTools.minTouchPixels : ScreenTools.defaultFontPixelWidth * 8) + toolStripColumn.anchors.margins * 2

    signal dropped(int index)

    DeadMouseArea {
        anchors.fill: parent
    }

    QGCFlickable {
        id:                 flickable
        anchors.margins:    ScreenTools.defaultFontPixelWidth * 0.4
        anchors.top:        parent.top
        anchors.left:       parent.left
        anchors.right:      parent.right
        height:             parent.height
        contentHeight:      toolStripColumn.height
        flickableDirection: Flickable.VerticalFlick
        clip:               true

        Column {
            id:             toolStripColumn
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        ScreenTools.defaultFontPixelWidth * 0.25

            QGCLabel {
                id:                     titleLabel
                anchors.left:           parent.left
                anchors.right:          parent.right
                horizontalAlignment:    Text.AlignHCenter
                font.pointSize:         ScreenTools.smallFontPointSize
                visible:                title != ""
            }

            Repeater {
                id: repeater

                QGCHoverButton {
                    id:             buttonTemplate

                    anchors.left:   toolStripColumn.left
                    anchors.right:  toolStripColumn.right
                    height:         width
                    radius:         ScreenTools.defaultFontPixelWidth / 2
                    fontPointSize:  ScreenTools.smallFontPointSize
                    autoExclusive:  true

                    enabled:        modelData.enabled
                    visible:        modelData.visible
                    imageSource:    modelData.showAlternateIcon ? modelData.alternateIconSource : modelData.iconSource
                    text:           modelData.text
                    checked:        modelData.checked
                    checkable:      modelData.dropPanelComponent || modelData.checkable

                    onCheckedChanged: modelData.checked = checked

                    onClicked: {
                        dropPanel.hide()
                        if (!modelData.dropPanelComponent) {
                            modelData.triggered(this)
                        } else if (checked) {
                            var panelEdgeTopPoint = mapToItem(_root, width, 0)
                            dropPanel.show(panelEdgeTopPoint, modelData.dropPanelComponent, this)
                            checked = true
                            _root.dropped(index)
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
