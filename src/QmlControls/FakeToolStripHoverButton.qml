/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

// This qml class is meant to be used to aesthetically match the looks of
// ToolStripHoverButton, but without depending on the whole concept of 
// ToolStripAction, which is handy to work on toolbars with simpler action
// Complexity

import QtQuick            2.12
import QtQuick.Controls   2.2

import QGroundControl              1.0
import QGroundControl.Controls     1.0
import QGroundControl.ScreenTools  1.0
import QGroundControl.Palette      1.0

Button {
    id:             control
    height:         width
    hoverEnabled:   !ScreenTools.isMobile
    imageSource:    showAlternateIcon ? alternateIconSource : iconSource

    property bool   biColorIcon:         false
    property var    alternateIconSource: ""
    property bool   fullColorIcon:       false
    property var    iconSource:          ""
    property bool   showAlternateIcon:   false
    property var    dropPanel:           undefined
    property alias  radius:              buttonBkRect.radius
    property alias  fontPointSize:       innerText.font.pointSize
    property alias  imageSource:         innerImage.source
    property alias  contentWidth:        innerText.contentWidth
    property bool forceImageScale11: false
    property real imageScale:        forceImageScale11 && (text == "") ? 0.8 : 0.6
    property real contentMargins:    innerText.height * 0.1
    property color currentContentColor:  (checked || pressed) ? qgcPal.buttonHighlightText : qgcPal.buttonText
    property color currentContentColorSecondary:  (checked || pressed) ? qgcPal.buttonText : qgcPal.buttonHighlight

    QGCPalette { id: qgcPal; colorGroupEnabled: control.enabled }
    
    contentItem: Item {
        id:                 contentLayoutItem
        anchors.fill:       parent
        anchors.margins:    control.contentMargins

        Column {
            anchors.centerIn:   parent
            spacing:        control.contentMargins * 2

            Image {
                id:                         innerImageColorful
                height:                     contentLayoutItem.height * control.imageScale
                width:                      contentLayoutItem.width  * control.imageScale
                smooth:                     true
                mipmap:                     true
                fillMode:                   Image.PreserveAspectFit
                antialiasing:               true
                sourceSize.height:          height
                sourceSize.width:           width
                anchors.horizontalCenter:   parent.horizontalCenter
                source:                     control.imageSource
                visible:                    source != "" && fullColorIcon
            }

            QGCColoredImage {
                id:                         innerImage
                height:                     contentLayoutItem.height * control.imageScale
                width:                      contentLayoutItem.width  * control.imageScale
                smooth:                     true
                mipmap:                     true
                color:                      control.currentContentColor
                fillMode:                   Image.PreserveAspectFit
                antialiasing:               true
                sourceSize.height:          height
                sourceSize.width:           width
                anchors.horizontalCenter:   parent.horizontalCenter
                visible:                    source != "" && !fullColorIcon

                QGCColoredImage {
                    id:                         innerImageSecondColor
                    source:                     control.alternateIconSource
                    height:                     contentLayoutItem.height * control.imageScale
                    width:                      contentLayoutItem.width  * control.imageScale
                    smooth:                     true
                    mipmap:                     true
                    color:                      control.currentContentColorSecondary
                    fillMode:                   Image.PreserveAspectFit
                    antialiasing:               true
                    sourceSize.height:          height
                    sourceSize.width:           width
                    anchors.horizontalCenter:   parent.horizontalCenter
                    visible:                    source != "" && biColorIcon
                }
            }

            QGCLabel {
                id:                         innerText
                text:                       control.text
                color:                      control.currentContentColor
                anchors.horizontalCenter:   parent.horizontalCenter
                font.pointSize:             ScreenTools.smallFontPointSize
                font.weight:                !innerImage.visible && !innerImageColorful.visible ? Font.Bold : Font.Medium
                opacity:                    !innerImage.visible ? 0.8 : 1.0
            }
        }
    }

    background: Rectangle {
        id:             buttonBkRect
        color:          (control.checked || control.pressed) ?
                            qgcPal.buttonHighlight :
                            (control.hovered ? qgcPal.toolStripHoverColor : qgcPal.toolbarBackground)
        anchors.fill:   parent
        radius:         ScreenTools.defaultFontPixelWidth / 2  
    }
}