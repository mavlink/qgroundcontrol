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

    property AbstractButton lastClickedButton: null

    function simulateClick(buttonIndex) {
        toolStripColumn.children[buttonIndex].checked = true
        toolStripColumn.children[buttonIndex].clicked()
    }

    // Ensure we don't get narrower than content
    property real _idealWidth: (ScreenTools.isMobile ? ScreenTools.minTouchPixels : ScreenTools.defaultFontPixelWidth * 8) + toolStripColumn.anchors.margins * 2

    signal clicked(int index, bool checked)
    signal dropped(int index)

    function setChecked(idx, check) {
        repeater.itemAt(idx).checked = check
    }

    function getChecked(idx) {
        return repeater.itemAt(idx).checked
    }

    ButtonGroup {
        id:         buttonGroup
        buttons:    toolStripColumn.children
    }

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

        Column {
            id:             toolStripColumn
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        ScreenTools.defaultFontPixelWidth * 0.25

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

                    enabled:        modelData.buttonEnabled
                    visible:        modelData.buttonVisible
                    imageSource:    modelData.showAlternateIcon ? modelData.alternateIconSource : modelData.iconSource
                    text:           modelData.name
                    checked:        modelData.checked !== undefined ? modelData.checked : checked

                    ButtonGroup.group: buttonGroup
                    // Only drop panel and toggleable are checkable
                    checkable: modelData.dropPanelComponent !== undefined || (modelData.toggle !== undefined && modelData.toggle)

                    onClicked: {
                        dropPanel.hide()    // DropPanel will call hide on "lastClickedButton"
                        if (modelData.dropPanelComponent === undefined) {
                            _root.clicked(index, checked)
                        } else if (checked) {
                            var panelEdgeTopPoint = mapToItem(_root, width, 0)
                            dropPanel.show(panelEdgeTopPoint, height, modelData.dropPanelComponent)
                            _root.dropped(index)
                        }
                        if(_root && buttonTemplate)
                            _root.lastClickedButton = buttonTemplate
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
