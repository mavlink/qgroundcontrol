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
import QGroundControl.Controllers   1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.Palette       1.0

/// Dialog showing list of available guided actions
Rectangle {
    id:         _root
    width:      actionColumn.width  + (_margins * 4)
    height:     actionColumn.height + (_margins * 4)
    radius:     _margins / 2
    color:      qgcPal.window
    opacity:    0.9
    visible:    false

    property var    guidedController
    property var    guidedValueSlider

    function show() {
        visible = true
    }

    property real _margins:             Math.round(ScreenTools.defaultFontPixelHeight * 0.66)
    property real _actionWidth:         ScreenTools.defaultFontPixelWidth * 25
    property real _actionHorizSpacing:  ScreenTools.defaultFontPixelHeight * 2

    property var _model: [
        {
            title:      guidedController.startMissionTitle,
            text:       guidedController.startMissionMessage,
            action:     guidedController.actionStartMission,
            visible:    guidedController.showStartMission
        },
        {
            title:      guidedController.continueMissionTitle,
            text:       guidedController.continueMissionMessage,
            action:     guidedController.actionContinueMission,
            visible:    guidedController.showContinueMission
        },
        {
            title:      guidedController.changeAltTitle,
            text:       guidedController.changeAltMessage,
            action:     guidedController.actionChangeAlt,
            visible:    guidedController.showChangeAlt
        },
        {
            title:      guidedController.landAbortTitle,
            text:       guidedController.landAbortMessage,
            action:     guidedController.actionLandAbort,
            visible:    guidedController.showLandAbort
        },
        {
            title:      guidedController.changeSpeedTitle,
            text:       guidedController.changeSpeedMessage,
            action:     guidedController.actionChangeSpeed,
            visible:    guidedController.showChangeSpeed
        },
        {
            title:      guidedController.gripperTitle,
            text:       guidedController.gripperMessage,
            action:     guidedController.actionGripper,
            visible:    guidedController.showGripper
        }
    ]

    property var _customManager: CustomActionManager {
        id: customManager
    }
    readonly property bool hasCustomActions: QGroundControl.settingsManager.flyViewSettings.enableCustomActions.rawValue && customManager.hasActions

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

            property real _width: Math.min((_actionWidth * 3) + _actionHorizSpacing*2, actionRow.width)

            RowLayout {
                id:         actionRow
                spacing:    _actionHorizSpacing

                // These are the pre-defined Actions
                Repeater {
                    id:     actionRepeater
                    model:  _model

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
                    } // ColumnLayout
                } // Repeater

                // These are the user-defined Custom Actions
                Repeater {
                    id:     customRepeater
                    model:  customManager.actions

                    ColumnLayout {
                        spacing:            ScreenTools.defaultFontPixelHeight / 2
                        visible:            _root.hasCustomActions
                        Layout.fillHeight:  true

                        QGCLabel {
                            id:                     customMessage
                            text:                   "Custom Action #" + (index + 1)
                            horizontalAlignment:    Text.AlignHCenter
                            wrapMode:               Text.WordWrap
                            Layout.minimumWidth:    _actionWidth
                            Layout.maximumWidth:    _actionWidth
                            Layout.fillHeight:      true
                        }

                        QGCButton {
                            id:                 customButton
                            text:               object.label
                            Layout.alignment:   Qt.AlignCenter

                            onClicked: {
                                var vehicle = QGroundControl.multiVehicleManager.activeVehicle
                                object.sendTo(vehicle)
                            }
                        }
                    } // ColumnLayout
                } // Repeater
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
