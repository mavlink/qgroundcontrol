/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief QGC Attitude Widget
 *   @author Gus Grubba <gus@auterion.com>
 */

import QtQuick 2.3
import QGroundControl.ScreenTools 1.0

Item {
    id: root

    property bool active:       false  ///< true: actively connected to data provider, false: show inactive control
    property real rollAngle :   _defaultRollAngle
    property real pitchAngle:   _defaultPitchAngle
    property bool showPitch:    true

    readonly property real _defaultRollAngle:   0
    readonly property real _defaultPitchAngle:  0

    property real _rollAngle:   active ? rollAngle : _defaultRollAngle
    property real _pitchAngle:  active ? pitchAngle : _defaultPitchAngle

    anchors.centerIn: parent

    Image {
        id: rollDial
        anchors { bottom: root.verticalCenter; horizontalCenter: parent.horizontalCenter }
        source:             "/qmlimages/rollDialWhite.svg"
        mipmap:             true
        width:              parent.width
        sourceSize.width:   width
        fillMode:           Image.PreserveAspectFit
        transform: Rotation {
            origin.x:       rollDial.width / 2
            origin.y:       rollDial.height
            angle:          -_rollAngle
        }
    }

    Image {
        id: pointer
        anchors { bottom: root.verticalCenter; horizontalCenter: parent.horizontalCenter }
        source:             "/qmlimages/rollPointerWhite.svg"
        mipmap:             true
        width:              rollDial.width
        sourceSize.width:   width
        fillMode:           Image.PreserveAspectFit
    }

    Image {
        id:                 crossHair
        anchors.centerIn:   parent
        source:             "/qmlimages/crossHair.svg"
        mipmap:             true
        width:              parent.width
        sourceSize.width:   width
        fillMode:           Image.PreserveAspectFit
    }

    QGCPitchIndicator {
        id:                 pitchIndicator
        anchors.verticalCenter: parent.verticalCenter
        visible:            showPitch
        pitchAngle:         _pitchAngle
        rollAngle:          _rollAngle
        color:              Qt.rgba(0,0,0,0)
        size:               ScreenTools.defaultFontPixelHeight * (10)
    }
}
