/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick              2.3
import QtGraphicalEffects   1.0

import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0

Item {
    property real size:     _defaultSize
    property var  vehicle:  null

    property real _defaultSize: ScreenTools.defaultFontPixelHeight * (10)
    property real _sizeRatio:   ScreenTools.isTinyScreen ? (size / _defaultSize) * 0.5 : size / _defaultSize
    property int  _fontSize:    ScreenTools.defaultFontPointSize * _sizeRatio
    property real _heading:     vehicle ? vehicle.heading.rawValue : 0

    width:                  size
    height:                 size

    Rectangle {
        id:             borderRect
        anchors.fill:   parent
        radius:         width / 2
        color:          "black"
    }

    Item {
        id:             instrument
        anchors.fill:   parent
        visible:        false

        Image {
            id:                     pointer
            source:                 "/qmlimages/attitudePointer.svg"
            mipmap:                 true
            fillMode:               Image.PreserveAspectFit
            anchors.leftMargin:     _pointerMargin
            anchors.rightMargin:    _pointerMargin
            anchors.topMargin:      _pointerMargin
            anchors.bottomMargin:   _pointerMargin
            anchors.fill:           parent
            sourceSize.height:      parent.height

            transform: Rotation {
                origin.x:       pointer.width  / 2
                origin.y:       pointer.height / 2
                angle:          _heading
            }

            readonly property real _pointerMargin: -10
        }

        Image {
            source:             "/qmlimages/compassInstrumentDial.svg"
            mipmap:             true
            fillMode:           Image.PreserveAspectFit
            anchors.fill:       parent
            sourceSize.height:  parent.height
        }
    }

    Rectangle {
        id:             mask
        anchors.fill:   instrument
        radius:         width / 2
        color:          "black"
        visible:        false
    }

    OpacityMask {
        anchors.fill:   instrument
        source:         instrument
        maskSource:     mask
    }

}
