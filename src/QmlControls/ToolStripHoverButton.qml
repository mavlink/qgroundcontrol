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

import QGroundControl.ScreenTools
import QGroundControl.Palette

Button {
    id:             control
    width:          contentLayoutItem.contentWidth + (contentMargins * 2)
    hoverEnabled:   !ScreenTools.isMobile
    enabled:        toolStripAction.enabled
    visible:        toolStripAction.visible
    imageSource:    toolStripAction.showAlternateIcon ? modelData.alternateIconSource : modelData.iconSource
    text:           toolStripAction.text
    checked:        toolStripAction.checked
    checkable:      toolStripAction.dropPanelComponent || modelData.checkable

    property var    toolStripAction:    undefined
    property var    dropPanel:          undefined
    property alias  radius:             buttonBkRect.radius
    property alias  fontPointSize:      innerText.font.pointSize
    property alias  imageSource:        innerImage.source
    property alias  contentWidth:       innerText.contentWidth

    property bool forceImageScale11: false
    property real imageScale:        forceImageScale11 && (text == "") ? 0.72 : 0.56
    property real contentMargins:    ScreenTools.defaultFontPixelWidth * 0.20

    property color _currentContentColor:  checked ? qgcPal.text : qgcPal.buttonText
    property color _currentContentColorSecondary:  qgcPal.colorGreen

    signal dropped(int index)

    onCheckedChanged: toolStripAction.checked = checked

    onClicked: {
        if (mainWindow.allowViewSwitch()) {
            dropPanel.hide()
            if (!toolStripAction.dropPanelComponent) {
                toolStripAction.triggered(this)
            } else if (checked) {
                var panelEdgeTopPoint = mapToItem(_root, width, 0)
                dropPanel.show(panelEdgeTopPoint, toolStripAction.dropPanelComponent, this)
                checked = true
                control.dropped(index)
            }
        } else if (checkable) {
            checked = !checked
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: control.enabled }

    contentItem: Item {
        id:                 contentLayoutItem
        anchors.fill:       parent
        anchors.margins:    contentMargins

        Column {
            anchors.centerIn:   parent
            width:              parent.width
            spacing:            contentMargins * 0.55

            Image {
                id:                         innerImageColorful
                height:                     contentLayoutItem.height * imageScale
                width:                      Math.min(contentLayoutItem.width, contentLayoutItem.height) * imageScale
                smooth:                     true
                mipmap:                     true
                fillMode:                   Image.PreserveAspectFit
                antialiasing:               true
                sourceSize.height:          height
                sourceSize.width:           width
                anchors.horizontalCenter:   parent.horizontalCenter
                source:                     control.imageSource
                visible:                    source != "" && modelData.fullColorIcon
            }

            QGCColoredImage {
                id:                         innerImage
                height:                     contentLayoutItem.height * imageScale
                width:                      Math.min(contentLayoutItem.width, contentLayoutItem.height) * imageScale
                smooth:                     true
                mipmap:                     true
                color:                      _currentContentColor
                fillMode:                   Image.PreserveAspectFit
                antialiasing:               true
                sourceSize.height:          height
                sourceSize.width:           width
                anchors.horizontalCenter:   parent.horizontalCenter
                visible:                    source != "" && !modelData.fullColorIcon
                
                QGCColoredImage {
                    id:                         innerImageSecondColor
                    source:                     modelData.alternateIconSource
                    height:                     contentLayoutItem.height * imageScale
                    width:                      Math.min(contentLayoutItem.width, contentLayoutItem.height) * imageScale
                    smooth:                     true
                    mipmap:                     true
                    color:                      _currentContentColorSecondary
                    fillMode:                   Image.PreserveAspectFit
                    antialiasing:               true
                    sourceSize.height:          height
                    sourceSize.width:           width
                    anchors.horizontalCenter:   parent.horizontalCenter
                    visible:                    source != "" && modelData.biColorIcon
                }
            }

            QGCLabel {
                id:                         innerText
                text:                       control.text
                color:                      _currentContentColor
                anchors.horizontalCenter:   parent.horizontalCenter
                width:                      parent.width
                horizontalAlignment:        Text.AlignHCenter
                wrapMode:                   Text.NoWrap
                maximumLineCount:           1
                elide:                      Text.ElideRight
                font.bold:                  control.checked || (!innerImage.visible && !innerImageColorful.visible)
                opacity:                    control.enabled ? 1.0 : 0.45
            }
        }
    }

    background: Rectangle {
        id:             buttonBkRect
        color:          control.pressed ? Qt.rgba(0.135, 0.140, 0.150, 0.24) :
                            ((control.enabled && control.hovered) ? Qt.rgba(1, 1, 1, 0.06) : "transparent")
        border.color:   "transparent"
        border.width:   0
        anchors.fill:   parent

        Rectangle {
            anchors.left:       parent.left
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            width:              Math.max(2, ScreenTools.defaultFontPixelWidth * 0.20)
            radius:             0
            color:              qgcPal.primaryButton
            visible:            control.checked
        }
    }
}
