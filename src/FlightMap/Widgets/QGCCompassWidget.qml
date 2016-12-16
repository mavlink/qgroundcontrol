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
 *   @brief QGC Compass Widget
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.4
import QtGraphicalEffects 1.0

import QGroundControl.Controls 1.0
import QGroundControl.ScreenTools 1.0

Item {
    id:                     root

    property bool active:   false  ///< true: actively connected to data provider, false: show inactive control
    property real heading:  0
    property real size:     _defaultSize

    property real _defaultSize: ScreenTools.defaultFontPixelHeight * (10)
    property real _sizeRatio:   ScreenTools.isTinyScreen ? (size / _defaultSize) * 0.5 : size / _defaultSize
    property int  _fontSize:    ScreenTools.defaultFontPointSize * _sizeRatio

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
            id:                 pointer
            source:             "/qmlimages/compassInstrumentArrow.svg"
            mipmap:             true
            width:              size * 0.75
            sourceSize.width:   width
            fillMode:           Image.PreserveAspectFit
            anchors.centerIn:   parent
            transform: Rotation {
                origin.x:       pointer.width  / 2
                origin.y:       pointer.height / 2
                angle:          heading
            }
        }

        Image {
            id:                 compassDial
            source:             "/qmlimages/compassInstrumentDial.svg"
            mipmap:             true
            fillMode:           Image.PreserveAspectFit
            anchors.fill:       parent
            sourceSize.height:  parent.height
        }

        Rectangle {
            anchors.centerIn:   parent
            width:              size * 0.35
            height:             size * 0.2
            border.color:       Qt.rgba(1,1,1,0.15)
            color:              Qt.rgba(0,0,0,0.65)

            QGCLabel {
                text:           active ? heading.toFixed(0) : qsTr("OFF")
                font.family:    active ? ScreenTools.demiboldFontFamily : ScreenTools.normalFontFamily
                font.pointSize: _fontSize < 8 ? 8 : _fontSize;
                color:          "white"
                anchors.centerIn: parent
            }
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
