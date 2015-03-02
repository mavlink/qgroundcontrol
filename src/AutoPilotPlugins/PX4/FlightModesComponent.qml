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

                DropArea {
                    property int channel:           modelData + 1
                    property bool throttleMapped:   channel == throttleChannel
                    property bool yawMapped:        channel == yawChannel
                    property bool pitchMapped:      channel == pitchChannel
                    property bool rollMapped:       channel == rollChannel
                    property bool flapsMapped:      channel == flapsChannel
                    property bool aux1Mapped:       channel == aux1Channel
                    property bool aux2Mapped:       channel == aux2Channel
                    property alias dropProxy:       dropTarget

                    // Drops are not allowed on channels which are mapped to non-flight mode switches
                    property bool dropAllowed: !(throttleMapped || yawMapped || pitchMapped || rollMapped || flapsMapped || aux1Mapped || aux2Mapped)

                    id:     dropTarget
                    width:  100
                    height: channelCol.implicitHeight

                    Rectangle {
                        id: channelTarget


                        width:  parent.width
                        height: parent.height
                        color:  qgcPal.windowShade

                        states: [
                            State {
                                when: dropTarget.containsDrag && dropAllowed
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
                            QGCLabel {
                                text: "Throttle"
                                visible: throttleMapped
                            }
                            QGCLabel {
                                text: "Rudder"
                                visible: yawMapped
                            }
                            QGCLabel {
                                text: "Pitch"
                                visible: pitchMapped
                            }
                            QGCLabel {
                                text: "Roll"
                                visible: rollMapped
                            }
                            QGCLabel {
                                text: "Flaps Switch"
                                visible: flapsMapped
                            }
                            QGCLabel {
                                text: "Aux1 Switch"
                                visible: aux1Mapped
                            }
                            QGCLabel {
                                text: "Aux2 Switch"
                                visible: aux2Mapped
                            }
                            QGCLabel {
                                text: "Mode Switch"
                                visible: channel == modeChannel
                            }
                            QGCLabel {
                                text: "PosCtl Switch"
                                visible: channel == posCtlChannel
                            }
                            QGCLabel {
                                text: "Return Switch"
                                visible: channel == returnChannel
                            }
                            QGCLabel {
                                text: "Loiter Switch"
                                visible: channel == loiterChannel
                            }
                        }
                    }
                }
            }
        }

        Item { height: 20; width: 10 } // spacer

        Component {
            id: modeTileComponent

            MouseArea {
                width:          100
                height:         20
                visible:        autopilot.parameters[tileParam].value == 0
                drag.target:    switchTile

                onReleased: {
                    // Move tile back to original position
                    switchTile.x = 0; switchTile.y = 0;

                    // If dropped over a channel target remap switch
                    if (switchTile.Drag.target && switchTile.Drag.target.dropAllowed) {
                        autopilot.parameters[tileParam].value = switchTile.Drag.target.channel
                    }
                }

                Rectangle {
                    id:     switchTile
                    width:  parent.width
                    height: parent.height
                    color:  qgcPal.windowShade

                    Drag.active:    parent.drag.active
                    Drag.hotSpot.x: width / 2
                    Drag.hotSpot.y: height / 2

                    QGCLabel {
                        text: tileLabel
                    }
                }
            }
        }

        QGCLabel {
            text: "Unassigned switches"
        }
        Flow {
            width: parent.width
            spacing: 5

            Loader {
                property string tileLabel: "Mode Switch"
                property string tileParam: "RC_MAP_MODE_SW"
                sourceComponent: modeTileComponent
            }
            Loader {
                property string tileLabel: "Return Switch"
                property string tileParam: "RC_MAP_RETURN_SW"
                sourceComponent: modeTileComponent
            }
            Loader {
                property string tileLabel: "Loiter Switch"
                property string tileParam: "RC_MAP_LOITER_SW"
                sourceComponent: modeTileComponent
            }
            Loader {
                property string tileLabel: "PosCtl Switch"
                property string tileParam: "RC_MAP_POSCTL_SW"
                sourceComponent: modeTileComponent
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
