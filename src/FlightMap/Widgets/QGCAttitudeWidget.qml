/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief QGC Attitude Instrument
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.4
import QtGraphicalEffects 1.0

import QGroundControl.Controls 1.0

Item {
    id: root

    property bool active:       false  ///< true: actively connected to data provider, false: show inactive control
    property real rollAngle :   _defaultRollAngle
    property real pitchAngle:   _defaultPitchAngle
    property bool showPitch:    true
    property real size

    readonly property real _defaultRollAngle:   0
    readonly property real _defaultPitchAngle:  0

    property real _rollAngle:   active ? rollAngle  : _defaultRollAngle
    property real _pitchAngle:  active ? pitchAngle : _defaultPitchAngle

    width:  size
    height: size

    Item {
        id:             instrument
        anchors.fill:   parent
        visible:        false

        //----------------------------------------------------
        //-- Artificial Horizon
        QGCArtificialHorizon {
            rollAngle:          _rollAngle
            pitchAngle:         _pitchAngle
            anchors.fill:       parent
        }
        //----------------------------------------------------
        //-- Pointer
        Image {
            id:                 pointer
            source:             "/qmlimages/attitudePointer.svg"
            mipmap:             true
            fillMode:           Image.PreserveAspectFit
            anchors.fill:       parent
            sourceSize.height:  parent.height
        }
        //----------------------------------------------------
        //-- Instrument Dial
        Image {
            id:                 instrumentDial
            source:             "/qmlimages/attitudeDial.svg"
            mipmap:             true
            fillMode:           Image.PreserveAspectFit
            anchors.fill:       parent
            sourceSize.height:  parent.height
            transform: Rotation {
                origin.x:       root.width  / 2
                origin.y:       root.height / 2
                angle:          -_rollAngle
            }
        }
        //----------------------------------------------------
        //-- Pitch
        QGCPitchIndicator {
            id:                 pitchWidget
            visible:            root.showPitch
            size:               root.size * 0.5
            anchors.verticalCenter: parent.verticalCenter
            pitchAngle:         _pitchAngle
            rollAngle:          _rollAngle
            color:              Qt.rgba(0,0,0,0)
        }
        //----------------------------------------------------
        //-- Cross Hair
        Image {
            id:                 crossHair
            anchors.centerIn:   parent
            source:             "/qmlimages/crossHair.svg"
            mipmap:             true
            width:              size * 0.75
            sourceSize.width:   width
            fillMode:           Image.PreserveAspectFit
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
        anchors.fill: instrument
        source: instrument
        maskSource: mask
    }

    Rectangle {
        id:             borderRect
        anchors.fill:   parent
        radius:         width / 2
        color:          Qt.rgba(0,0,0,0)
        border.color:   "black"
        border.width:   2
    }

}
