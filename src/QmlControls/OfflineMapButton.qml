/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.5
import QtQuick.Controls 1.2

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Rectangle {
    id:             mapButton
    anchors.margins: ScreenTools.defaultFontPixelWidth
    color:          _showHighlight ? qgcPal.buttonHighlight : qgcPal.button
    border.width:   _showBorder ? 1: 0
    border.color:   qgcPal.buttonText

    property bool   checked:    false
    property bool   complete:   false
    property alias  text:       nameLabel.text
    property int    tiles:      0
    property string size:       ""

    property bool   _showHighlight: (_pressed | _hovered | checked) && !_forceHoverOff
    property bool   _showBorder:    qgcPal.globalTheme === QGCPalette.Light

    property bool   _forceHoverOff:    false
    property int    _lastGlobalMouseX: 0
    property int    _lastGlobalMouseY: 0
    property bool   _pressed:          false
    property bool   _hovered:          false

    signal clicked()

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    Row {
        anchors.centerIn: parent
        QGCLabel {
            id:     nameLabel
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

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onMouseXChanged: {
            _lastGlobalMouseX = ScreenTools.mouseX()
            _lastGlobalMouseY = ScreenTools.mouseY()
        }
        onMouseYChanged: {
            _lastGlobalMouseX = ScreenTools.mouseX()
            _lastGlobalMouseY = ScreenTools.mouseY()
        }
        onEntered:  { _hovered = true;  _forceHoverOff = false; hoverTimer.start() }
        onExited:   { _hovered = false; _forceHoverOff = false; hoverTimer.stop()  }
        onPressed:  { _pressed = true;  }
        onReleased: { _pressed = false; }
        onClicked:  mapButton.clicked()
    }

    Timer {
        id:         hoverTimer
        interval:   250
        repeat:     true
        onTriggered: {
            if (_lastGlobalMouseX !== ScreenTools.mouseX() || _lastGlobalMouseY !== ScreenTools.mouseY()) {
                _forceHoverOff = true
            } else {
                _forceHoverOff = false
            }
        }
    }
}
