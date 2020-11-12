/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 2.4

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Button {
    id:                 mapButton
    height:             ScreenTools.defaultFontPixelHeight * 4
    autoExclusive:      true

    background: Rectangle {
        anchors.fill:   parent
        color:          _showHighlight ? qgcPal.buttonHighlight : qgcPal.button
        border.width:   _showBorder ? 1: 0
        border.color:   qgcPal.buttonText
    }

    property var    tileSet:    null
    property var    currentSet: null
    property bool   complete:   false
    property int    tiles:      0
    property string size:       ""

    property bool   _showHighlight: (_pressed | _hovered | checked) && !_forceHoverOff
    property bool   _showBorder:    qgcPal.globalTheme === QGCPalette.Light

    property bool   _forceHoverOff:    false
    property int    _lastGlobalMouseX: 0
    property int    _lastGlobalMouseY: 0
    property bool   _pressed:          false
    property bool   _hovered:          false

    contentItem: Row {
        anchors.centerIn:   parent
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        QGCLabel {
            text:   mapButton.text
            width:  mapButton.width * 0.4
            color:  _showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText
            anchors.verticalCenter: parent.verticalCenter
        }
        QGCLabel {
            id:     sizeLabel
            width:  mapButton.width * 0.4
            horizontalAlignment: Text.AlignRight
            anchors.verticalCenter: parent.verticalCenter
            color:  _showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText
            text:   mapButton.size + (tiles > 0 ? " (" + tiles + " tiles)" : "")
        }
        Item {
            width:  ScreenTools.defaultFontPixelWidth * 2
            height: 1
        }
        Rectangle {
            width:   sizeLabel.height * 0.5
            height:  sizeLabel.height * 0.5
            radius:  width / 2
            color:   complete ? "#31f55b" : "#fc5656"
            opacity: sizeLabel.text.length > 0 ? 1 : 0
            anchors.verticalCenter: parent.verticalCenter
        }
        Item {
            width:  ScreenTools.defaultFontPixelWidth * 2
            height: 1
        }
        QGCColoredImage {
            width:      sizeLabel.height * 0.8
            height:     sizeLabel.height * 0.8
            sourceSize.height:  height
            source:     "/res/buttonRight.svg"
            mipmap:     true
            fillMode:   Image.PreserveAspectFit
            color:      _showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
