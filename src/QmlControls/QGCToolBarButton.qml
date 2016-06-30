/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.4
import QtQuick.Controls 1.2

import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Item {
    id:     _root
    state:  "HelpShown"
    clip:   true

    property alias          text:           helpText.text
    property alias          source:         icon.source
    property bool           checked:        false
    property ExclusiveGroup exclusiveGroup:  null

    signal   clicked()

    readonly property real _topBottomMargins: ScreenTools.defaultFontPixelHeight / 2

    property var    _imageColor:            checked ? qgcPal.buttonHighlight : qgcPal.buttonText
    property real   _helpTextBottomMargin:  0
    property real   _imageBottomMargin:     0

    onExclusiveGroupChanged: {
        if (exclusiveGroup) {
            exclusiveGroup.bindCheckable(_root)
        }
    }

    QGCPalette { id: qgcPal }

    states: [
        State {
            name: "HelpShown"
        },
        State {
            name: "HelpHidden"
            PropertyChanges { target: imageAnimation; running: true  }
            PropertyChanges { target: helpTextAnimation; running: true  }
        }
    ]

    PropertyAnimation {
        id:             imageAnimation
        target:         _root
        property:       "_imageBottomMargin"
        duration:       1000
        easing.type:    Easing.InOutQuad
        to:             _topBottomMargins
        from:           0
    }

    PropertyAnimation {
        id:             helpTextAnimation
        target:         _root
        property:       "_helpTextBottomMargin"
        duration:       1000
        easing.type:    Easing.InOutQuad
        to:             -helpText.height
        from:           0
    }

    Timer {
        interval:       10000
        running:        true
        onTriggered:    _root.state = "HelpHidden"
    }

    QGCColoredImage {
        id:                     icon
        anchors.left:           parent.left
        anchors.right:          parent.right
        height:                 _root.height - (_topBottomMargins * 2)
        anchors.bottomMargin:   _imageBottomMargin
        anchors.bottom:         helpText.top
        sourceSize.height:      parent.height
        fillMode:               Image.PreserveAspectFit
        color:                  _imageColor
    }

    Rectangle {
        anchors.left:   parent.left
        anchors.right:  parent.right
        anchors.bottom: parent.bottom
        height:         _topBottomMargins * 0.25
        color:          qgcPal.buttonHighlight
        visible:        checked
    }

    QGCLabel {
        id:                     helpText
        anchors.left:           parent.left
        anchors.right:          parent.right
        anchors.bottomMargin:   _helpTextBottomMargin
        anchors.bottom:         parent.bottom
        text:                   "Button"
        horizontalAlignment:    Text.AlignHCenter
        color:                  _imageColor
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            checked = true
            _root.clicked()
        }
    }
}
