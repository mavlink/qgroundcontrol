/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.11
import QtQuick.Controls 1.2

import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Item {
    id:     _root
    width:  iconLogo.width
    clip:   true

    property string         source:         ""
    property string         text:           ""
    property bool           checked:        false
    property bool           logo:           false
    property ExclusiveGroup exclusiveGroup: null

    readonly property real _topBottomMargins: ScreenTools.defaultFontPixelHeight * 0.5

    signal clicked()

    onExclusiveGroupChanged: {
        if (exclusiveGroup) {
            exclusiveGroup.bindCheckable(_root)
        }
    }

    Row {
        id:                         iconLogo
        spacing:                    ScreenTools.defaultFontPixelWidth
        anchors.top:                parent.top
        anchors.bottom:             parent.bottom
        QGCColoredImage {
            source:                 _root.source
            color:                  checked ? "#5FCDF3" : "#FFFFFF"
            width:                  height
            sourceSize.height:      height
            anchors.top:            parent.top
            anchors.topMargin:      _topBottomMargins
            anchors.bottom:         parent.bottom
            anchors.bottomMargin:   _topBottomMargins
            fillMode:               Image.PreserveAspectFit
            visible:                !logo
        }
        Image {
            source:                 _root.source
            width:                  height
            sourceSize.height:      height
            anchors.top:            parent.top
            anchors.topMargin:      _topBottomMargins
            anchors.bottom:         parent.bottom
            anchors.bottomMargin:   _topBottomMargins
            fillMode:               Image.PreserveAspectFit
            visible:                logo
        }
        QGCLabel {
            id:                     logoLabel
            color:                  "#5FCDF3"
            text:                   _root.text
            visible:                checked && !logo
            font.pointSize:         ScreenTools.smallFontPointSize
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            checked = true
            _root.clicked()
        }
    }
}
