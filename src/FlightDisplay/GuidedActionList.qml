/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0

/// Dialog showing list of available guided actions
Rectangle {
    id:         _root
    width:      actionColumn.width  + (_margins * 4)
    height:     actionColumn.height + (_margins * 4)
    radius:     _margins / 2
    color:      qgcPal.window
    opacity:    0.9
    z:          guidedController.z
    visible:    false

    property var    guidedController
    property var    altitudeSlider
    property alias  model:              actionRepeater.model

    property real _margins:             Math.round(ScreenTools.defaultFontPixelHeight * 0.66)
    property real _actionWidth:         ScreenTools.defaultFontPixelWidth * 25
    property real _actionHorizSpacing:  ScreenTools.defaultFontPixelHeight * 2


    QGCPalette { id: qgcPal }

    DeadMouseArea {
        anchors.fill: parent
    }

    ColumnLayout {
        id:                 actionColumn
        anchors.margins:    _root._margins
        anchors.centerIn:   parent
        spacing:            _margins

        QGCLabel {
            text:               qsTr("Select Action")
            Layout.alignment:   Qt.AlignHCenter
        }

        QGCFlickable {
            contentWidth:           actionRow.width
            contentHeight:          actionRow.height
            Layout.minimumHeight:   actionRow.height
            Layout.maximumHeight:   actionRow.height
            Layout.minimumWidth:    _width
            Layout.maximumWidth:    _width

            property real _width: Math.min((_actionWidth * 2) + _actionHorizSpacing, actionRow.width)

            RowLayout {
                id:         actionRow
                spacing:    _actionHorizSpacing

                Repeater {
                    id: actionRepeater

                    ColumnLayout {
                        spacing:            ScreenTools.defaultFontPixelHeight / 2
                        visible:            modelData.visible
                        Layout.fillHeight:  true

                        QGCLabel {
                            id:                     actionMessage
                            text:                   modelData.text
                            horizontalAlignment:    Text.AlignHCenter
                            wrapMode:               Text.WordWrap
                            Layout.minimumWidth:    _actionWidth
                            Layout.maximumWidth:    _actionWidth
                            Layout.fillHeight:      true

                            property real _width: ScreenTools.defaultFontPixelWidth * 25
                        }

                        QGCButton {
                            id:                 actionButton
                            text:               modelData.title
                            Layout.alignment:   Qt.AlignCenter

                            onClicked: {
                                _root.visible = false
                                guidedController.confirmAction(modelData.action)
                            }
                        }
                    }
                }
            }
        }
    }

    QGCColoredImage {
        anchors.margins:    _margins
        anchors.top:        parent.top
        anchors.right:      parent.right
        width:              ScreenTools.defaultFontPixelHeight
        height:             width
        sourceSize.height:  width
        source:             "/res/XDelete.svg"
        fillMode:           Image.PreserveAspectFit
        color:              qgcPal.text

        QGCMouseArea {
            fillItem:   parent
            onClicked:  _root.visible = false
        }
    }
}
