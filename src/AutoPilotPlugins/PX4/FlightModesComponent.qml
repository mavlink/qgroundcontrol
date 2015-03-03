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

    property bool inRedistribution: false

    readonly property int tileWidth: 150
    readonly property int tileHeight: 30

    readonly property int progressBarHeight: 200


    QGCPalette { id: qgcPal; colorGroupEnabled: true }
    FlightModesComponentController { id: controller }

    color: qgcPal.window

    Component {
        id: dragHandle

        Item {
            id:     outerItem
            width:  parent.width
            height: parent.height

            Column {
                x: 4
                y: 4
                spacing: 3

                Repeater {
                    model: (outerItem.height - 8) / 4
                    Rectangle {
                        color: qgcPal.text
                        width: 15
                        height: 1
                    }
                }
            }
        }
    }

    // This component is used to create draggable tiles for unassigned mode switches. It also
    // creates the drop area for dragging an assigned mode switch tile back to an unassigned state.
    Component {
        id: unassignedModeTileComponent

        Rectangle {
            property bool dragEnabled: autopilot.parameters[tileParam].value == 0

            id:     outerRect
            width:  tileWidth
            height: tileHeight
            color:  qgcPal.windowShadeDark
            border.width: dragEnabled ? 1 : 0
            border.color: qgcPal.text

            Drag.active:    mouseArea.drag.active
            Drag.hotSpot.x: width / 2
            Drag.hotSpot.y: height / 2
            Drag.keys:      [ "unassigned"]

            states: [
                State {
                    when: dropArea.containsDrag
                    PropertyChanges {
                        target: outerRect
                        color: "red"
                    }
                }
            ]

            Loader {
                width: parent.width
                height: parent.height
                visible: dragEnabled
                sourceComponent: dragHandle
            }

            QGCLabel {
                width:                  parent.width
                height:                 parent.height
                text:                   tileLabel
                enabled:                dragEnabled
                horizontalAlignment:    Text.AlignHCenter
                verticalAlignment:      Text.AlignVCenter
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

            DropArea {
                // This will cause to tile to go back to unassigned if dropped here
                readonly property int channel: 0
                property bool dropAllowed: true

                id:     dropArea
                width:  parent.width
                height: parent.height

                keys: [ "assigned" ]
            }
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
            width:          tileWidth
            height:         tileHeight
            color:          qgcPal.windowShadeDark
            border.width:   tileDragEnabled ? 1 : 0
            border.color:   qgcPal.text
            visible:        tileVisible

            Drag.active:    mouseArea.drag.active
            Drag.hotSpot.x: width / 2
            Drag.hotSpot.y: height / 2
            Drag.keys:      [ "assigned" ]

            Loader {
                width: parent.width
                height: parent.height
                visible: tileDragEnabled
                sourceComponent: dragHandle
            }

            QGCLabel {
                width:                  parent.width
                height:                 parent.height
                enabled:                tileDragEnabled
                horizontalAlignment:    Text.AlignHCenter
                verticalAlignment:      Text.AlignVCenter
                text:                   tileLabel
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

    onModeChannelChanged: if (!inRedistribution) redistributeThresholds()
    onReturnChannelChanged: if (!inRedistribution) redistributeThresholds()
    onLoiterChannelChanged: if (!inRedistribution) redistributeThresholds()
    onPosCtlChannelChanged: if (!inRedistribution) redistributeThresholds()

    function redistributeThresholds() {
        if (modeChannel != 0) {
            var positions = 3  // Manual/Assist/Auto always exist

            var returnOnModeSwitch = modeChannel == returnChannel
            var loiterOnModeSwitch = modeChannel == loiterChannel
            var posCtlOnModeSwitch = modeChannel == posCtlChannel

            positions += returnOnModeSwitch ? 1 : 0
            positions += loiterOnModeSwitch ? 1 : 0
            positions += posCtlOnModeSwitch ? 1 : 0

            var increment = 1.0 / positions
            var currentThreshold = 0.0

            // Make sure we don't re-enter
            inRedistribution = true

            currentThreshold += increment
            autopilot.parameters["RC_ASSIST_TH"].value = currentThreshold
            if (posCtlOnModeSwitch) {
                currentThreshold += increment
                autopilot.parameters["RC_POSCTL_TH"].value = currentThreshold
            }
            currentThreshold += increment
            autopilot.parameters["RC_AUTO_TH"].value = currentThreshold
            if (loiterOnModeSwitch) {
                currentThreshold += increment
                autopilot.parameters["RC_LOITER_TH"].value = currentThreshold
            }
            if (returnOnModeSwitch) {
                currentThreshold += increment
                autopilot.parameters["RC_RETURN_TH"].value = currentThreshold
            }

            inRedistribution = false
        }

        if (returnChannel != 0 && returnChannel != modeChannel) {
            var positions = 2  // On/off always exist

            var loiterOnReturnSwitch = returnChannel == loiterChannel

            positions += loiterOnReturnSwitch ? 1 : 0

            var increment = 1.0 / positions
            var currentThreshold = 0.0

            // Make sure we don't re-enter
            inRedistribution = true

            if (loiterOnReturnSwitch) {
                currentThreshold += increment
                autopilot.parameters["RC_LOITER_TH"].value = currentThreshold
            }
            currentThreshold += increment
            autopilot.parameters["RC_RETURN_TH"].value = currentThreshold

            inRedistribution = false
        }

        if (loiterChannel != 0 && loiterChannel != modeChannel && loiterChannel != returnChannel) {
            var positions = 2  // On/off always exist

            var increment = 1.0 / positions
            var currentThreshold = 0.0

            // Make sure we don't re-enter
            inRedistribution = true

            currentThreshold += increment
            autopilot.parameters["RC_LOITER_TH"].value = currentThreshold

            inRedistribution = false
        }

        if (posCtlChannel != 0 & posCtlChannel != modeChannel) {
            var positions = 2  // On/off always exist

            var increment = 1.0 / positions
            var currentThreshold = 0.0

            // Make sure we don't re-enter
            inRedistribution = true

            currentThreshold += increment
            autopilot.parameters["RC_POSCTL_TH"].value = currentThreshold

            inRedistribution = false
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
            width: parent.width
            text: "Flight Mode switches can be assigned to any channel which is not currently being used for attitude control. All channels are displayed below. " +
                "You can drag Flight Modes from the Flight Modes section below to a channel and drop it there. You can also drag switches assigned to a channel " +
                "to another channel or back to the Unassigned Switches section. The Switch Display section at the very bottom will show you the results of your Flight Mode setup."
            wrapMode: Text.WordWrap
        }

        Item { height: 20; width: 10 } // spacer

        QGCLabel {
            text: "Channel Assignments"
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

                    property bool modeMapped:       channel == modeChannel
                    property bool posCtlMapped:     channel == posCtlChannel
                    property bool returnMapped:     channel == returnChannel
                    property bool loiterMapped:     channel == loiterChannel

                    property bool nonFlightModeMapping: throttleMapped | yawMapped | pitchMapped | rollMapped | flapsMapped | aux1Mapped | aux2Mapped
                    property bool unassignedMapping: !(nonFlightModeMapping | modeMapped | posCtlMapped | returnMapped | loiterMapped)

                    id:     channelTarget
                    width:  tileWidth
                    height: channelCol.implicitHeight

                    color:  qgcPal.windowShadeDark

                    states: [
                        State {
                            when: dropArea.containsDrag && dropArea.dropAllowed
                            PropertyChanges {
                                target: channelHeader
                                color: "red"
                            }
                        }
                    ]

                    Column {
                        id:         channelCol
                        spacing:    3

                        Rectangle {
                            id: channelHeader
                            width:  tileWidth
                            height: tileHeight
                            color:  qgcPal.windowShade

                            QGCLabel {
                                verticalAlignment:      Text.AlignVCenter
                                horizontalAlignment:    Text.AlignHCenter
                                text:                   "Channel " + (modelData + 1) + (nonFlightModeMapping ? ": Unavailable" : "")
                            }
                        }
                        Loader {
                            property string tileLabel:      "Available"
                            property bool tileVisible:      visible
                            property bool tileDragEnabled:  false

                            visible:            unassignedMapping
                            sourceComponent:    assignedModeTileComponent
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
                            property string tileLabel:      "Main Mode"
                            property bool tileVisible:      visible
                            property bool tileDragEnabled:  true
                            property string tileParam:      "RC_MAP_MODE_SW"

                            visible:            modeMapped
                            sourceComponent:    assignedModeTileComponent
                        }
                        Loader {
                            property string tileLabel:      "Return"
                            property bool tileVisible:      visible
                            property bool tileDragEnabled:  true
                            property string tileParam:      "RC_MAP_RETURN_SW"

                            visible:            returnMapped
                            sourceComponent:    assignedModeTileComponent
                        }
                        Loader {
                            property string tileLabel:      "Loiter"
                            property bool tileVisible:      visible
                            property bool tileDragEnabled:  true
                            property string tileParam:      "RC_MAP_LOITER_SW"

                            visible:            loiterMapped
                            sourceComponent:    assignedModeTileComponent
                        }
                        Loader {
                            property string tileLabel:      "PosCtl"
                            property bool tileVisible:      visible
                            property bool tileDragEnabled:  true
                            property string tileParam:      "RC_MAP_POSCTL_SW"

                            visible:            posCtlMapped
                            sourceComponent:    assignedModeTileComponent
                        }
                    }

                    DropArea {
                        // Drops are not allowed on channels which are mapped to non-flight mode switches
                        property bool dropAllowed: !nonFlightModeMapping
                        property int channel: parent.channel

                        id:     dropArea
                        width:  parent.width
                        height: parent.height

                        keys: [ "unassigned", "assigned" ]
                    }
                }
            }
        }

        Item { height: 20; width: 10 } // spacer

        QGCLabel {
            text: "Flight Modes"
        }
        Flow {
            width: parent.width
            spacing: 5

            Loader {
                property string tileLabel: "Main Mode"
                property string tileParam: "RC_MAP_MODE_SW"
                sourceComponent: unassignedModeTileComponent
            }
            Loader {
                property string tileLabel: "Return"
                property string tileParam: "RC_MAP_RETURN_SW"
                sourceComponent: unassignedModeTileComponent
            }
            Loader {
                property string tileLabel: "Loiter"
                property string tileParam: "RC_MAP_LOITER_SW"
                sourceComponent: unassignedModeTileComponent
            }
            Loader {
                property string tileLabel: "PosCtl"
                property string tileParam: "RC_MAP_POSCTL_SW"
                sourceComponent: unassignedModeTileComponent
            }
        }

        Item { height: 20; width: 10 } // spacer

        QGCLabel {
            text: "Switch Display"
        }
        Row {
            property bool modeSwitchVisible: modeChannel != 0
            property bool returnSwitchVisible: returnChannel != 0 && returnChannel != modeChannel
            property bool loiterSwitchVisible: loiterChannel != 0 && loiterChannel != modeChannel && loiterChannel != returnChannel
            property bool posCtlSwitchVisible: posCtlChannel != 0 && posCtlChannel != modeChannel

            width:      parent.width
            spacing:    20

            Column {
                visible: parent.modeSwitchVisible

                QGCLabel { text: "Mode Switch" }

                Row {
                    Item {
                        height: progressBarHeight
                        width:  150

                        QGCLabel {
                            width:                  parent.width
                            y:                      (parent.height * (1.0 - autopilot.parameters["RC_AUTO_TH"].value)) - (implicitHeight / 2)
                            visible:                modeChannel != returnChannel && modeChannel != loiterChannel
                            horizontalAlignment:    Text.AlignRight
                            text:                   "Auto"
                        }

                        QGCLabel {
                            width:                  parent.width
                            y:                      (parent.height * (1.0 - autopilot.parameters["RC_RETURN_TH"].value)) - (implicitHeight / 2)
                            visible:                modeChannel == returnChannel
                            horizontalAlignment:    Text.AlignRight
                            text:                   "Auto: Return"
                        }

                        QGCLabel {
                            width:                  parent.width
                            y:                      (parent.height * (1.0 - autopilot.parameters["RC_LOITER_TH"].value)) - (implicitHeight / 2)
                            visible:                modeChannel == loiterChannel
                            horizontalAlignment:    Text.AlignRight
                            text:                   "Auto: Loiter"
                        }

                        QGCLabel {
                            width:                  parent.width
                            y:                      (parent.height * (1.0 - autopilot.parameters["RC_AUTO_TH"].value)) - (implicitHeight / 2)
                            visible:                modeChannel == loiterChannel
                            horizontalAlignment:    Text.AlignRight
                            text:                   "Auto: Mission"
                        }

                        QGCLabel {
                            width:                  parent.width
                            y:                      (parent.height * (1.0 - autopilot.parameters["RC_AUTO_TH"].value)) - (implicitHeight / 2)
                            visible:                modeChannel == returnChannel && modeChannel != loiterChannel
                            horizontalAlignment:    Text.AlignRight
                            text:                   "Auto: Loiter/Mission"
                        }

                        QGCLabel {
                            width:                  parent.width
                            y:                      (parent.height * (1.0 - autopilot.parameters["RC_ASSIST_TH"].value)) - (implicitHeight / 2)
                            visible:                modeChannel != posCtlChannel
                            horizontalAlignment:    Text.AlignRight
                            text:                   "Assist"
                        }

                        QGCLabel {
                            width:                  parent.width
                            y:                      (parent.height * (1.0 - autopilot.parameters["RC_POSCTL_TH"].value)) - (implicitHeight / 2)
                            visible:                modeChannel == posCtlChannel
                            horizontalAlignment:    Text.AlignRight
                            text:                   "Assist: PosCtl"
                        }

                        QGCLabel {
                            width:                  parent.width
                            y:                      (parent.height * (1.0 - autopilot.parameters["RC_ASSIST_TH"].value)) - (implicitHeight / 2)
                            visible:                modeChannel == posCtlChannel
                            horizontalAlignment:    Text.AlignRight
                            text:                   "Assist: AltCtl"
                        }

                        QGCLabel {
                            width:              parent.width
                            y:                  parent.height - (implicitHeight / 2)
                            text:               "Manual"
                            horizontalAlignment: Text.AlignRight
                        }
                    }

                    ProgressBar {
                        height:         progressBarHeight
                        orientation:    Qt.Vertical
                        value:          1
                    }
                }
            }

            Column {
                visible: parent.returnSwitchVisible

                QGCLabel { text: "Return Switch" }

                Row {
                    Item {
                        height: progressBarHeight
                        width:  150

                        QGCLabel {
                            width:                  parent.width
                            y:                      (parent.height * (1.0 - autopilot.parameters["RC_RETURN_TH"].value)) - (implicitHeight / 2)
                            horizontalAlignment:    Text.AlignRight
                            text:                   "Auto: Return"
                        }

                        QGCLabel {
                            width:                  parent.width
                            y:                      (parent.height * (1.0 - autopilot.parameters["RC_LOITER_TH"].value)) - (implicitHeight / 2)
                            visible:                returnChannel == loiterChannel
                            horizontalAlignment:    Text.AlignRight
                            text:                   "Auto: Loiter"
                        }

                        QGCLabel {
                            width:                  parent.width
                            y:                      parent.height - (implicitHeight / 2)
                            visible:                returnChannel == loiterChannel
                            horizontalAlignment:    Text.AlignRight
                            text:                   "Auto: Mission"
                        }

                        QGCLabel {
                            width:                  parent.width
                            y:                      parent.height - (implicitHeight / 2)
                            visible:                returnChannel != loiterChannel
                            horizontalAlignment:    Text.AlignRight
                            text:                   "Auto: Return Off"
                        }
                    }

                    ProgressBar {
                        height:         progressBarHeight
                        orientation:    Qt.Vertical
                        value:          1
                    }
                }
            }

            Column {
                visible: parent.loiterSwitchVisible

                QGCLabel { text: "Loiter Switch" }

                Row {
                    Item {
                        height: progressBarHeight
                        width:  150

                        QGCLabel {
                            width:                  parent.width
                            y:                      (parent.height * (1.0 - autopilot.parameters["RC_LOITER_TH"].value)) - (implicitHeight / 2)
                            horizontalAlignment:    Text.AlignRight
                            text:                   "Auto: Loiter"
                        }

                        QGCLabel {
                            width:                  parent.width
                            y:                      parent.height - (implicitHeight / 2)
                            horizontalAlignment:    Text.AlignRight
                            text:                   "Auto: Mission"
                        }
                    }

                    ProgressBar {
                        height:         progressBarHeight
                        orientation:    Qt.Vertical
                        value:          1
                    }
                }
            }

            Column {
                visible: parent.posCtlSwitchVisible

                QGCLabel { text: "PosCtl Switch" }

                Row {
                    Item {
                        height: progressBarHeight
                        width:  150

                        QGCLabel {
                            width:                  parent.width
                            y:                      (parent.height * (1.0 - autopilot.parameters["RC_POSCTL_TH"].value)) - (implicitHeight / 2)
                            horizontalAlignment:    Text.AlignRight
                            text:                   "Assist: PosCtl"
                        }

                        QGCLabel {
                            width:                  parent.width
                            y:                      parent.height - (implicitHeight / 2)
                            horizontalAlignment:    Text.AlignRight
                            text:                   "Assist: AltCtl"
                        }
                    }

                    ProgressBar {
                        height:         progressBarHeight
                        orientation:    Qt.Vertical
                        value:          1
                    }
                }
            }
        }
    }
}
