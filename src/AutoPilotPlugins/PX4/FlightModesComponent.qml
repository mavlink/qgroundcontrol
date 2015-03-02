import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Controllers 1.0

Rectangle {
    property int throttleChannel:   autopilot.parameters["RC_MAP_THROTTLE"].value
    property int yawChannel:        autopilot.parameters["RC_MAP_YAW"].value
    property int pitchChannel:      autopilot.parameters["RC_MAP_PITCH"].value
    property int rollChannel:       autopilot.parameters["RC_MAP_ROLL"].value
    property int flapsChannel:      autopilot.parameters["RC_MAP_FLAPS"].value
    property int aux1Channel:       autopilot.parameters["RC_MAP_AUX1"].value
    property int aux2Channel:       autopilot.parameters["RC_MAP_AUX2"].value

    property int modeChannel:   autopilot.parameters["RC_MAP_MODE_SW"].value
    property int posCtlChannel: autopilot.parameters["RC_MAP_POSCTL_SW"].value
    property int returnChannel: autopilot.parameters["RC_MAP_RETURN_SW"].value
    property int loiterChannel: autopilot.parameters["RC_MAP_LOITER_SW"].value

    QGCPalette { id: qgcPal; colorGroupEnabled: true }
    FlightModesComponentController { id: controller }

    color: qgcPal.window

    // This component is used to create draggable tiles for unassigned mode switches. It also
    // creates the drop area for dragging an assigned mode switch tile back to an unassigned state.
    Component {
        id: unassignedModeTileComponent

        Rectangle {
            property bool dragEnabled: autopilot.parameters[tileParam].value == 0

            id:     outerRect
            width:  100
            height: 20
            color:  qgcPal.windowShade

            Drag.active:    mouseArea.drag.active
            Drag.hotSpot.x: width / 2
            Drag.hotSpot.y: height / 2

/*
            states: [
                State {
                    when: dropArea.containsDrag
                    PropertyChanges {
                        target: outerRect
                        color: "red"
                    }
                }
            ]
*/

            QGCLabel {
                text: tileLabel
                enabled: dragEnabled
            }

            MouseArea {
                id:             mouseArea
                width:          parent.width
                height:         parent.height
                drag.target:    dragEnabled ? parent : null

                onReleased: {
                    // Move tile back to original position
                    parent.x = 0; parent.y = 0;

                    // If dropped over a channel target remap switch
                    if (parent.Drag.target && parent.Drag.target.dropAllowed) {
                        autopilot.parameters[tileParam].value = parent.Drag.target.channel
                    }
                }
            }

/*
            DropArea {
                // This will cause to tile to go back to unassigned if dropped here
                //readonly property int channel: 0

                id:     dropArea
                width:  parent.width
                height: parent.height
            }
*/
        }
    }

    // This component is used to create draggable tiles for currently assigned mode switches. The following
    // properties must be set in the Loader:
    //  tileLabel - label for tile
    //  tileVisible - visibility for tile
    //  tileDragEnabled - true: this tile can be dragged
    Component {
        id: assignedModeTileComponent

        Rectangle {
            width:      label.implicitWidth
            height:     label.implicitHeight
            color:      qgcPal.windowShade
            visible:    tileVisible

            Drag.active:    mouseArea.drag.active
            Drag.hotSpot.x: width / 2
            Drag.hotSpot.y: height / 2

            QGCLabel {
                id:         label
                text:       tileLabel
                enabled:    tileDragEnabled
            }

            MouseArea {
                id:             mouseArea
                width:          parent.width
                height:         parent.height
                drag.target:    tileDragEnabled ? parent : null

                onReleased: {
                    // Move tile back to original position
                    parent.x = 0; parent.y = 0;

                    // If dropped over a channel target remap switch
                    if (parent.Drag.target && parent.Drag.target.dropAllowed) {
                        autopilot.parameters[tileParam].value = parent.Drag.target.channel
                    }
                }
            }
        }
    }

    Column {
        anchors.fill: parent

        QGCLabel {
            text: "FLIGHT MODES CONFIG"
            font.pointSize: 20
        }

        Item { height: 20; width: 10 } // spacer

        QGCLabel {
            text: "Channel assignments"
        }
        Flow {
            width: parent.width
            spacing: 5

            Repeater {
                model: 18

                Rectangle {
                    property int channel:           modelData + 1
                    property bool throttleMapped:   channel == throttleChannel
                    property bool yawMapped:        channel == yawChannel
                    property bool pitchMapped:      channel == pitchChannel
                    property bool rollMapped:       channel == rollChannel
                    property bool flapsMapped:      channel == flapsChannel
                    property bool aux1Mapped:       channel == aux1Channel
                    property bool aux2Mapped:       channel == aux2Channel


                    id:     channelTarget
                    width:  100
                    height: channelCol.implicitHeight

                    color:  qgcPal.windowShade

                    states: [
                        State {
                            when: dropArea.containsDrag && dropArea.dropAllowed
                            PropertyChanges {
                                target: channelTarget
                                color: "red"
                            }
                        }
                    ]

                    Column {
                        id: channelCol

                        QGCLabel {
                            text: "Channel " + (modelData + 1)
                        }
                        Loader {
                            property string tileLabel:      "Throttle"
                            property bool tileVisible:      visible
                            property bool tileDragEnabled:  false

                            visible:            throttleMapped
                            sourceComponent:    assignedModeTileComponent
                        }
                        Loader {
                            property string tileLabel:      "Yaw"
                            property bool tileVisible:      visible
                            property bool tileDragEnabled:  false

                            visible:            yawMapped
                            sourceComponent:    assignedModeTileComponent
                        }
                        Loader {
                            property string tileLabel:      "Pitch"
                            property bool tileVisible:      visible
                            property bool tileDragEnabled:  false

                            visible:            pitchMapped
                            sourceComponent:    assignedModeTileComponent
                        }
                        Loader {
                            property string tileLabel:      "Roll"
                            property bool tileVisible:      visible
                            property bool tileDragEnabled:  false

                            visible:            rollMapped
                            sourceComponent:    assignedModeTileComponent
                        }
                        Loader {
                            property string tileLabel:      "Flaps Switch"
                            property bool tileVisible:      visible
                            property bool tileDragEnabled:  false

                            visible:            flapsMapped
                            sourceComponent:    assignedModeTileComponent
                        }
                        Loader {
                            property string tileLabel:      "Aux1 Switch"
                            property bool tileVisible:      visible
                            property bool tileDragEnabled:  false

                            visible:            aux1Mapped
                            sourceComponent:    assignedModeTileComponent
                        }
                        Loader {
                            property string tileLabel:      "Aux2 Switch"
                            property bool tileVisible:      visible
                            property bool tileDragEnabled:  false

                            visible:            aux2Mapped
                            sourceComponent:    assignedModeTileComponent
                        }
                        Loader {
                            property string tileLabel:      "Mode Switch"
                            property bool tileVisible:      visible
                            property bool tileDragEnabled:  true
                            property string tileParam:      "RC_MAP_MODE_SW"

                            visible:            channel == modeChannel
                            sourceComponent:    assignedModeTileComponent
                        }
                        Loader {
                            property string tileLabel:      "PosCtl Switch"
                            property bool tileVisible:      visible
                            property bool tileDragEnabled:  true
                            property string tileParam:      "RC_MAP_POSCTL_SW"

                            visible:            channel == posCtlChannel
                            sourceComponent:    assignedModeTileComponent
                        }
                        Loader {
                            property string tileLabel:      "Return Switch"
                            property bool tileVisible:      visible
                            property bool tileDragEnabled:  true
                            property string tileParam:      "RC_MAP_RETURN_SW"

                            visible:            channel == returnChannel
                            sourceComponent:    assignedModeTileComponent
                        }
                        Loader {
                            property string tileLabel:      "Loiter Switch"
                            property bool tileVisible:      visible
                            property bool tileDragEnabled:  true
                            property string tileParam:      "RC_MAP_LOITER_SW"

                            visible:            channel == loiterChannel
                            sourceComponent:    assignedModeTileComponent
                        }
                    }

                    DropArea {
                        // Drops are not allowed on channels which are mapped to non-flight mode switches
                        property bool dropAllowed: !(throttleMapped || yawMapped || pitchMapped || rollMapped || flapsMapped || aux1Mapped || aux2Mapped)
                        property int channel: parent.channel

                        id:     dropArea
                        width:  parent.width
                        height: parent.height
                    }
                }
            }
        }

        Item { height: 20; width: 10 } // spacer

        QGCLabel {
            text: "Unassigned switches"
        }
        Flow {
            width: parent.width
            spacing: 5

            Loader {
                property string tileLabel: "Mode Switch"
                property string tileParam: "RC_MAP_MODE_SW"
                sourceComponent: unassignedModeTileComponent
            }
            Loader {
                property string tileLabel: "Return Switch"
                property string tileParam: "RC_MAP_RETURN_SW"
                sourceComponent: unassignedModeTileComponent
            }
            Loader {
                property string tileLabel: "Loiter Switch"
                property string tileParam: "RC_MAP_LOITER_SW"
                sourceComponent: unassignedModeTileComponent
            }
            Loader {
                property string tileLabel: "PosCtl Switch"
                property string tileParam: "RC_MAP_POSCTL_SW"
                sourceComponent: unassignedModeTileComponent
            }
        }

        Item { height: 20; width: 10 } // spacer

        QGCLabel {
            text: "Switch settings"
        }
        Flow {
            width: parent.width
            spacing: 20

            Column {
                visible: modeChannel != 0

                QGCLabel { text: "Mode Switch" }
                QGCLabel {
                    text: "  Auto"
                    visible: modeChannel != returnChannel && modeChannel != loiterChannel
                }
                QGCLabel {
                    text: "  Auto: Return";
                    visible: modeChannel == returnChannel
                }
                QGCLabel {
                    text: "  Auto: Loiter";
                    visible: modeChannel == loiterChannel
                }
                QGCLabel {
                    text: "  Auto: Mission";
                    visible: modeChannel == loiterChannel
                }
                QGCLabel {
                    text: "  Auto: Loiter/Mission";
                    visible: modeChannel == returnChannel && modeChannel != loiterChannel
                }
                QGCLabel {
                    text: "  Assist"
                    visible: modeChannel != posCtlChannel
                }
                QGCLabel {
                    text: "  Assist: PosCtl";
                    visible: modeChannel == posCtlChannel
                }
                QGCLabel {
                    text: "  Assist: AltCtl";
                    visible: modeChannel == posCtlChannel
                }
                QGCLabel { text: "  Manual" }
            }

            Column {
                visible: returnChannel != 0 && returnChannel != modeChannel

                QGCLabel { text: "Return Switch" }
                QGCLabel {
                    text: "  Auto: Return";
                }
                QGCLabel {
                    text: "  Auto: Loiter";
                    visible: returnChannel == loiterChannel
                }
                QGCLabel {
                    text: "  Auto: Mission";
                    visible: returnChannel == loiterChannel
                }
                QGCLabel {
                    text: "  Auto: Loiter/Mission";
                    visible: returnChannel != loiterChannel
                }
                QGCLabel {
                    text: "  Assist: PosCtl";
                    visible: returnChannel == posCtlChannel
                }
                QGCLabel {
                    text: "  Assist: AltCtl";
                    visible: returnChannel == posCtlChannel
                }
            }

            Column {
                visible: loiterChannel != 0 && loiterChannel != modeChannel && loiterChannel != returnChannel

                QGCLabel { text: "Loiter Switch" }
                QGCLabel {
                    text: "  Auto: Loiter";
                }
                QGCLabel {
                    text: "  Auto: Mission";
                }
                QGCLabel {
                    text: "  Assist: PosCtl";
                    visible: loiterChannel == posCtlChannel
                }
                QGCLabel {
                    text: "  Assist: AltCtl";
                    visible: loiterChannel == posCtlChannel
                }
            }

            Column {
                visible: posCtlChannel != 0 && posCtlChannel != modeChannel && posCtlChannel != loiterChannel && posCtlChannel != returnChannel

                QGCLabel { text: "PosCtl Switch" }
                QGCLabel { text: "  Assist: PosCtl" }
                QGCLabel { text: "  Assist: AltCtl" }
            }
        }
    }
}
