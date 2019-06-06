/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.11
import QtQuick.Controls 2.2

import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0

Rectangle {
    id:         _root
    color:      qgcPal.windowShade
    width:      _idealWidth < repeater.contentWidth ? repeater.contentWidth : _idealWidth
    height:     toolStripColumn.height + (toolStripColumn.anchors.margins * 2)
    radius:     ScreenTools.defaultFontPixelWidth / 2

    property alias  model:              repeater.model
    property var    rotateImage         ///< List of bool values, one for each button in strip - true: animation rotation, false: static image
    property var    animateImage        ///< List of bool values, one for each button in strip - true: animate image, false: static image
    property var    buttonEnabled       ///< List of bool values, one for each button in strip - true: button enabled, false: button disabled
    property var    buttonVisible       ///< List of bool values, one for each button in strip - true: button visible, false: button invisible
    property real   maxHeight           ///< Maximum height for control, determines whether text is hidden to make control shorter
    property var    showAlternateIcon   ///< List of bool values, one for each button in strip - true: show alternate icon, false: show normal icon

    property AbstractButton lastClickedButton: null

    // Ensure we don't get narrower than content
    property real _idealWidth: (ScreenTools.isMobile ? ScreenTools.minTouchPixels : ScreenTools.defaultFontPixelWidth * 8) + toolStripColumn.anchors.margins * 2

    signal clicked(int index, bool checked)

    ButtonGroup {
        id: buttonGroup
        exclusive: false
    }

    Column {
        id:                 toolStripColumn
        anchors.margins:    ScreenTools.defaultFontPixelWidth * 0.4
        anchors.top:        parent.top
        anchors.left:       parent.left
        anchors.right:      parent.right
        spacing:            ScreenTools.defaultFontPixelWidth * 0.25

        Repeater {
            id: repeater

            QGCHoverButton {
                id: buttonTemplate

                anchors.left:   toolStripColumn.left
                anchors.right:  toolStripColumn.right
                height:         width
                radius:         ScreenTools.defaultFontPixelWidth / 2
                fontPointSize:  ScreenTools.smallFontPointSize

                enabled:        _root.buttonEnabled ? _root.buttonEnabled[index] : true
                visible:        _root.buttonVisible ? _root.buttonVisible[index] : true
                imageSource:    (_root.showAlternateIcon && _root.showAlternateIcon[index]) ? _alternateIconSource : _iconSource
                text:           modelData.name

                property var    _iconSource:            modelData.iconSource
                property var    _alternateIconSource:   modelData.alternateIconSource

                ButtonGroup.group: buttonGroup
                // Only drop pannel and toggleable are checkable
                checkable: modelData.dropPanelComponent !== undefined || (modelData.toggle !== undefined && modelData.toggle)

                onClicked: {
                    dropPanel.hide()    // DropPanel will call hide on "lastClickedButton"

                    // Uncheck other checked buttons
                    // TODO: Implement ButtonGroup exclusive with checkable and uncheckable and get rid of this workaround
                    for(var i = 0; i < buttonGroup.buttons.length; i++) {
                        var b = buttonGroup.buttons[i]
                        if(b !== buttonTemplate) {
                            b.checked = false;
                        }
                    }

                    if (modelData.dropPanelComponent === undefined) {
                        _root.clicked(index, checked)
                    } else if (checked) {
                        var panelEdgeTopPoint = mapToItem(_root, width, 0)
                        dropPanel.show(panelEdgeTopPoint, height, modelData.dropPanelComponent)
                    }
                    lastClickedButton = buttonTemplate
                }
            }
        }
    }

    DropPanel {
        id:         dropPanel
        toolStrip:  _root
    }
}
