/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.Palette

/// This is the dial background for the compass

Item {
    id: control

    property real offsetRadius: width / 2 - ScreenTools.defaultFontPixelHeight / 2

    function translateCenterToAngleX(radius, angle) {
        return radius * Math.sin(angle * (Math.PI / 180))
    } 

    function translateCenterToAngleY(radius, angle) {
        return -radius * Math.cos(angle * (Math.PI / 180))
    }

    QGCLabel {
        anchors.centerIn:   parent
        text:               "N"

        transform: Translate {
            x: translateCenterToAngleX(control.offsetRadius, 0)
            y: translateCenterToAngleY(control.offsetRadius, 0)
        }
    }

    QGCLabel {
        anchors.centerIn:   parent
        text:               "E"

        transform: Translate {
            x: translateCenterToAngleX(control.offsetRadius, 90)
            y: translateCenterToAngleY(control.offsetRadius, 90)
        }
    }

    QGCLabel {
        anchors.centerIn:   parent
        text:               "S"

        transform: Translate {
            x: translateCenterToAngleX(control.offsetRadius, 180)
            y: translateCenterToAngleY(control.offsetRadius, 180)
        }
    }

    QGCLabel {
        anchors.centerIn:   parent
        text:               "W"

        transform: Translate {
            x: translateCenterToAngleX(control.offsetRadius, 270)
            y: translateCenterToAngleY(control.offsetRadius, 270)
        }
    }

    // Major tick marks
    Repeater {
        model: 4

        Rectangle {
            id:                 majorTick
            x:                  size / 2
            width:              1
            height:             ScreenTools.defaultFontPixelHeight * 0.5
            color:              qgcPal.text

            transform: Rotation {
                origin.x:   0
                origin.y:   size / 2
                angle:      45 + (90 * index)
            }
        }
    }

    // Minor tick marks
    Repeater {
        model: 8

        Rectangle {
            id:                 majorTick
            x:                  size / 2
            y:                  _margin
            width:              1
            height:             _margin
            color:              qgcPal.text

            property real _margin: ScreenTools.defaultFontPixelHeight * 0.25

            transform: Rotation {
                origin.x:   0
                origin.y:   size / 2 - _margin
                angle:      45 / 2 + (45 * index)
            }
        }
    }    
}
