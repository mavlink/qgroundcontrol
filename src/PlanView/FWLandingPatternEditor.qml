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
import QGroundControl.FactSystem    1.0
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

    property real   _margin:                    ScreenTools.defaultFontPixelWidth / 2
    property real   _spacer:                    ScreenTools.defaultFontPixelWidth / 2
    property var    _activeVehicle:             QGroundControl.multiVehicleManager.activeVehicle
    property string _setToVehicleHeadingStr:    qsTr("Set to vehicle heading")
    property string _setToVehicleLocationStr:   qsTr("Set to vehicle location")


    ExclusiveGroup { id: distanceGlideGroup }

    Column {
        id:                 editorColumn
        anchors.margins:    _margin
        anchors.left:       parent.left
        anchors.right:      parent.right
        spacing:            _margin
        visible:            missionItem.landingCoordSet

        SectionHeader {
            id:     loiterPointSection
            text:   qsTr("Loiter point")
        }

        Column {
            anchors.left:       parent.left
            anchors.right:      parent.right
            spacing:            _margin
            visible:            loiterPointSection.checked

            Item { width: 1; height: _spacer }

            FactTextFieldGrid {
                anchors.left:   parent.left
                anchors.right:  parent.right
                factList:       [ missionItem.loiterAltitude, missionItem.loiterRadius ]
                factLabels:     [ qsTr("Approach Alt"), qsTr("Radius") ]
            }

            Item { width: 1; height: _spacer }

            QGCCheckBox {
                text:           qsTr("Loiter clockwise")
                checked:        missionItem.loiterClockwise
                onClicked:      missionItem.loiterClockwise = checked
            }

            QGCButton {
                text:       _setToVehicleHeadingStr
                visible:    _activeVehicle
                onClicked:  missionItem.landingHeading.rawValue = _activeVehicle.heading.rawValue
            }
        }

        SectionHeader {
            id:     landingPointSection
            text:   qsTr("Landing point")
        }

        Column {
            anchors.left:       parent.left
            anchors.right:      parent.right
            spacing:            _margin
            visible:            landingPointSection.checked

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
                    checked:            missionItem.valueSetIsDistance.rawValue
                    exclusiveGroup:     distanceGlideGroup
                    onClicked:          missionItem.valueSetIsDistance.rawValue = checked
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
                    checked:            !missionItem.valueSetIsDistance.rawValue
                    exclusiveGroup:     distanceGlideGroup
                    onClicked:          missionItem.valueSetIsDistance.rawValue = !checked
                    Layout.fillWidth:   true
                }

                FactTextField {
                    fact:               missionItem.glideSlope
                    enabled:            specifyGlideSlope.checked
                    Layout.fillWidth:   true
                }

                QGCButton {
                    text:               _setToVehicleLocationStr
                    visible:            _activeVehicle
                    Layout.columnSpan:  2
                    onClicked:          missionItem.landingCoordinate = _activeVehicle.coordinate
                }
            }
        }

        Item { width: 1; height: _spacer }

        QGCCheckBox {
            anchors.right:  parent.right
            text:           qsTr("Altitudes relative to home")
            checked:        missionItem.altitudesAreRelative
            visible:        QGroundControl.corePlugin.options.showMissionAbsoluteAltitude || !missionItem.altitudesAreRelative
            onClicked:      missionItem.altitudesAreRelative = checked
        }

        SectionHeader {
            id:     cameraSection
            text:   qsTr("Camera")
        }

        Column {
            anchors.left:       parent.left
            anchors.right:      parent.right
            spacing:            _margin
            visible:            cameraSection.checked

            Item { width: 1; height: _spacer }

            FactCheckBox {
                text:       _stopTakingPhotos.shortDescription
                fact:       _stopTakingPhotos

                property Fact _stopTakingPhotos: missionItem.stopTakingPhotos
            }

            FactCheckBox {
                text:       _stopTakingVideo.shortDescription
                fact:       _stopTakingVideo

                property Fact _stopTakingVideo: missionItem.stopTakingVideo
            }
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
            anchors.left:           parent.left
            anchors.right:          parent.right
            wrapMode:               Text.WordWrap
            horizontalAlignment:    Text.AlignHCenter
            text:                   qsTr("Click in map to set landing point.")
        }

        QGCLabel {
            anchors.left:           parent.left
            anchors.right:          parent.right
            horizontalAlignment:    Text.AlignHCenter
            text:                   qsTr("- or -")
            visible:                _activeVehicle
        }

        QGCButton {
            anchors.horizontalCenter:   parent.horizontalCenter
            text:                       _setToVehicleLocationStr
            visible:                    _activeVehicle

            onClicked: {
                missionItem.landingCoordinate = _activeVehicle.coordinate
                missionItem.landingHeading.rawValue = _activeVehicle.heading.rawValue
            }
        }
    }
}
