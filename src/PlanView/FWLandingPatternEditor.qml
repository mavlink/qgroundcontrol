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

    ExclusiveGroup { id: distanceGlideGroup }

    Column {
        id:                 editorColumn
        anchors.margins:    _margin
        anchors.left:       parent.left
        anchors.right:      parent.right
        spacing:            _margin
        visible:            missionItem.landingCoordSet

        SectionHeader {
            text: qsTr("Loiter point")
        }

        Item { width: 1; height: _spacer }

        FactTextFieldGrid {
            anchors.left:   parent.left
            anchors.right:  parent.right
            factList:       [ missionItem.loiterAltitude, missionItem.loiterRadius ]
            factLabels:     [ qsTr("Altitude"), qsTr("Radius") ]
        }

        Item { width: 1; height: _spacer }

        QGCCheckBox {
            text:           qsTr("Loiter clockwise")
            checked:        missionItem.loiterClockwise
            onClicked:      missionItem.loiterClockwise = checked
        }

        SectionHeader { text: qsTr("Landing point") }

        Item { width: 1; height: _spacer }

        GridLayout {
            anchors.left:    parent.left
            anchors.right:   parent.right
            columns:         2

                QGCLabel { text: qsTr("Heading") }

                FactTextField {
                    Layout.fillWidth:   true
                    fact:               missionItem.landingHeading
                }

                QGCLabel { text: qsTr("Altitude") }

                FactTextField {
                    Layout.fillWidth:   true
                    fact:               missionItem.landingAltitude
                }

            QGCRadioButton {
                id:                 specifyLandingDistance
                text:               qsTr("Landing Dist")
                checked:            missionItem.valueSetIsDistance
                exclusiveGroup:     distanceGlideGroup
                onClicked:          missionItem.valueSetIsDistance = checked
                Layout.fillWidth:   true
            }

            FactTextField {
                fact:               missionItem.landingDistance
                enabled:            specifyLandingDistance.checked
                Layout.fillWidth:   true
            }

            QGCRadioButton {
                id:                 specifyGlideSlope
                text:               qsTr("Glide Slope")
                checked:            !missionItem.valueSetIsDistance
                exclusiveGroup:     distanceGlideGroup
                onClicked:          missionItem.valueSetIsDistance = !checked
                Layout.fillWidth:   true
            }

            FactTextField {
                fact:               missionItem.glideSlope
                enabled:            specifyGlideSlope.checked
                Layout.fillWidth:   true
            }
        }

        Item { width: 1; height: _spacer }

        QGCCheckBox {
            anchors.right:  parent.right
            text:           qsTr("Altitudes relative to home")
            checked:        missionItem.altitudesAreRelative
            onClicked:      missionItem.altitudesAreRelative = checked
        }
    }

    Column {
        id:                 editorColumnNeedLandingPoint
        anchors.margins:    _margin
        anchors.top:        parent.top
        anchors.left:       parent.left
        anchors.right:      parent.right
        visible:            !missionItem.landingCoordSet
        spacing:            ScreenTools.defaultFontPixelHeight

        QGCLabel {
            anchors.left:   parent.left
            anchors.right:  parent.right
            wrapMode:       Text.WordWrap
            text:           qsTr("Click in map to set landing point.")
        }
    }
}
