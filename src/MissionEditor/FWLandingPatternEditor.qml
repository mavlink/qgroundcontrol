/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0

// Editor for Fixed Wing Landing Pattern complex mission item
Rectangle {
    id:         _root
    height:     visible ? ((editorColumn.visible ? editorColumn.height : editorColumnNeedLandingPoint.height) + (_margin * 2)) : 0
    width:      availableWidth
    color:      qgcPal.windowShadeDark
    radius:     _radius

    // The following properties must be available up the hierarchy chain
    //property real   availableWidth    ///< Width for control
    //property var    missionItem       ///< Mission Item for editor

    property real _margin: ScreenTools.defaultFontPixelWidth / 2
    property real _spacer: ScreenTools.defaultFontPixelWidth / 2

    Column {
        id:                 editorColumn
        anchors.margins:    _margin
        anchors.left:       parent.left
        anchors.right:      parent.right
        visible:            missionItem.landingCoordSet

        QGCLabel {
            anchors.left:   parent.left
            anchors.right:  parent.right
            wrapMode:       Text.WordWrap
            font.pointSize: ScreenTools.smallFontPointSize
            text:           qsTr("WIP (NOT FOR REAL FLIGHT!)")
        }

        SectionHeader {
            text: qsTr("Loiter point")
        }

        Item { width: 1; height: _spacer }

        FactTextFieldGrid {
            anchors.left:   parent.left
            anchors.right:  parent.right
            factList:       [ missionItem.loiterAltitude, missionItem.loiterRadius ]
        }

        Item { width: 1; height: _spacer }

        QGCCheckBox {
            id:             loiterAltRelative
            anchors.right:  parent.right
            text:           qsTr("Altitude relative to home")
            checked:        missionItem.loiterAltitudeRelative
            onClicked:      missionItem.loiterAltitudeRelative = checked
        }

        Item { width: 1; height: _spacer }

        QGCCheckBox {
            anchors.left:   loiterAltRelative.left
            text:           qsTr("Loiter clockwise")
            checked:        missionItem.loiterClockwise
            onClicked:      missionItem.loiterClockwise = checked
        }

        SectionHeader { text: qsTr("Landing point") }

        Item { width: 1; height: _spacer }

        FactTextFieldGrid {
            anchors.left:   parent.left
            anchors.right:  parent.right
            factList:       [ missionItem.landingAltitude, missionItem.landingDistance, missionItem.landingHeading ]
        }

        Item { width: 1; height: _spacer }

        QGCCheckBox {
            anchors.right:  parent.right
            text:           qsTr("Altitude relative to home")
            checked:        missionItem.landingAltitudeRelative
            onClicked:      missionItem.landingAltitudeRelative = checked
        }
    }

    Column {
        id:                 editorColumnNeedLandingPoint
        anchors.margins:    _margin
        anchors.left:       parent.left
        anchors.right:      parent.right
        visible:            !missionItem.landingCoordSet
        spacing:            ScreenTools.defaultFontPixelHeight

        QGCLabel {
            anchors.left:   parent.left
            anchors.right:  parent.right
            wrapMode:       Text.WordWrap
            font.pointSize: ScreenTools.smallFontPointSize
            text:           qsTr("WIP (NOT FOR REAL FLIGHT!)")
        }

        QGCLabel {
            anchors.left:   parent.left
            anchors.right:  parent.right
            wrapMode:       Text.WordWrap
            text:           qsTr("Click in map to set landing point.")
        }
    }
}
