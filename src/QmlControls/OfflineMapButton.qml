import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

Button {
    id:                 mapButton
    height:             ScreenTools.defaultFontPixelHeight * 4
    autoExclusive:      true

    background: Rectangle {
        anchors.fill:   parent
        color:          _showSelected ? qgcPal.accentSubtle : qgcPal.windowCard
        border.width:   1
        border.color:   _showHighlight ? qgcPal.accent : qgcPal.borderSubtle
        radius:         ScreenTools.defaultBorderRadius
    }

    property var    tileSet:    null
    property var    currentSet: null
    property bool   complete:   false
    property int    tiles:      0
    property string size:       ""

    property bool   _showHighlight: (_pressed || _hovered || checked) && !_forceHoverOff
    property bool   _showSelected:  (_pressed || checked) && !_forceHoverOff

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
            color:  qgcPal.textPrimary
            wrapMode: Text.Wrap
            maximumLineCount: 2
            anchors.verticalCenter: parent.verticalCenter
        }
        QGCLabel {
            id:     sizeLabel
            width:  mapButton.width * 0.4
            horizontalAlignment: Text.AlignRight
            anchors.verticalCenter: parent.verticalCenter
            color:  qgcPal.textPrimary
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
            color:      qgcPal.textPrimary
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
