/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick              2.3
import QtQuick.Controls     2.4
import QtQuick.Layouts      1.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0

Item {
    id: _root

    property var planMasterController
    property var mapControl

    property real   _radius:    ScreenTools.defaultFontPixelWidth / 2
    property real   _margins:   ScreenTools.defaultFontPixelWidth

    function _mapCenter() {
        var centerPoint = Qt.point(mapControl.centerViewport.left + (mapControl.centerViewport.width / 2), mapControl.centerViewport.top + (mapControl.centerViewport.height / 2))
        return mapControl.toCoordinate(centerPoint, false /* clipToViewPort */)
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    Rectangle {
        anchors.fill:   parent
        radius:         _radius
        color:          "white"
        opacity:        0.75
    }

    // Close Icon
    QGCColoredImage {
        anchors.margins:    ScreenTools.defaultFontPixelWidth / 2
        anchors.top:        parent.top
        anchors.right:      parent.right
        width:              ScreenTools.defaultFontPixelHeight
        height:             width
        sourceSize.height:  width
        source:             "/res/XDelete.svg"
        fillMode:           Image.PreserveAspectFit
        mipmap:             true
        smooth:             true
        color:              "black"
        QGCMouseArea {
            fillItem:   parent
            onClicked:  _root.visible = false
        }
    }

    QGCLabel {
        id:                     title
        anchors.left:           parent.left
        anchors.right:          parent.right
        horizontalAlignment:    Text.AlignHCenter
        text:                   qsTr("Create Plan")
        color:                  "black"
    }

    QGCFlickable {
        id:                 flickable
        anchors.margins:    _margins
        anchors.top:        title.bottom
        anchors.bottom:     parent.bottom
        anchors.left:       parent.left
        anchors.right:      parent.right
        contentHeight:      creatorFlow.height
        contentWidth:       creatorFlow.width

        Flow {
            id:         creatorFlow
            width:      flickable.width
            spacing:    _margins

            Repeater {
                model: _planMasterController.planCreators

                Rectangle {
                    id:     button
                    width:  ScreenTools.defaultFontPixelHeight * 10
                    height: width
                    color:  button.pressed || button.highlighted ? qgcPal.buttonHighlight : qgcPal.button

                    property bool highlighted: mouseArea.containsMouse
                    property bool pressed:     mouseArea.pressed

                    Image {
                        anchors.margins:        _margins
                        anchors.left:           parent.left
                        anchors.right:          parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        source:                 object.imageResource
                        fillMode:               Image.PreserveAspectFit
                        mipmap:                 true
                    }

                    QGCLabel {
                        anchors.margins:        _margins
                        anchors.bottom:         parent.bottom
                        anchors.left:           parent.left
                        anchors.right:          parent.right
                        horizontalAlignment:    Text.AlignHCenter
                        text:                   object.name
                        color:                  button.pressed || button.highlighted ? qgcPal.buttonHighlightText : qgcPal.buttonText
                    }

                    QGCMouseArea {
                        id:                 mouseArea
                        anchors.fill:       parent
                        hoverEnabled:       true
                        preventStealing:    true
                        onClicked:          { object.createPlan(_mapCenter()); _root.visible = false }
                    }
                }
            }
        }
    }
}
