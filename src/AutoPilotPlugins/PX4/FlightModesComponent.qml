/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Controllers 1.0
import QGroundControl.ScreenTools 1.0

Item {
    Loader {
        id:                 loader
        anchors.fill:       parent
        sourceComponent:    controller.validConfiguration ? validComponent : invalidComponent

        property FlightModesComponentController controller: FlightModesComponentController { factPanel: loader.item }
        property QGCPalette qgcPal: QGCPalette { colorGroupEnabled: true }
        property bool loading: true

        onLoaded: loading = false
    }

    Component {
        id: validComponent

        FactPanel {
            property Fact rc_map_throttle:      controller.getParameterFact(-1, "RC_MAP_THROTTLE")
            property Fact rc_map_yaw:           controller.getParameterFact(-1, "RC_MAP_YAW")
            property Fact rc_map_pitch:         controller.getParameterFact(-1, "RC_MAP_PITCH")
            property Fact rc_map_roll:          controller.getParameterFact(-1, "RC_MAP_ROLL")
            property Fact rc_map_flaps:         controller.getParameterFact(-1, "RC_MAP_FLAPS")
            property Fact rc_map_aux1:          controller.getParameterFact(-1, "RC_MAP_AUX1")
            property Fact rc_map_aux2:          controller.getParameterFact(-1, "RC_MAP_AUX2")

            property Fact rc_map_mode_sw:       controller.getParameterFact(-1, "RC_MAP_MODE_SW")
            property Fact rc_map_posctl_sw:     controller.getParameterFact(-1, "RC_MAP_POSCTL_SW")
            property Fact rc_map_return_sw:     controller.getParameterFact(-1, "RC_MAP_RETURN_SW")
            property Fact rc_map_offboard_sw:   controller.getParameterFact(-1, "RC_MAP_OFFB_SW")
            property Fact rc_map_loiter_sw:     controller.getParameterFact(-1, "RC_MAP_LOITER_SW")
            property Fact rc_map_acro_sw:       controller.getParameterFact(-1, "RC_MAP_ACRO_SW")

            property Fact rc_assist_th:         controller.getParameterFact(-1, "RC_ASSIST_TH")
            property Fact rc_posctl_th:         controller.getParameterFact(-1, "RC_POSCTL_TH")
            property Fact rc_auto_th:           controller.getParameterFact(-1, "RC_AUTO_TH")
            property Fact rc_loiter_th:         controller.getParameterFact(-1, "RC_LOITER_TH")
            property Fact rc_return_th:         controller.getParameterFact(-1, "RC_RETURN_TH")
            property Fact rc_offboard_th:       controller.getParameterFact(-1, "RC_OFFB_TH")
            property Fact rc_acro_th:           controller.getParameterFact(-1, "RC_ACRO_TH")

            property Fact rc_th_user:           controller.getParameterFact(-1, "RC_TH_USER")

            property int throttleChannel:   rc_map_throttle.value
            property int yawChannel:        rc_map_yaw.value
            property int pitchChannel:      rc_map_pitch.value
            property int rollChannel:       rc_map_roll.value
            property int flapsChannel:      rc_map_flaps.value
            property int aux1Channel:       rc_map_aux1.value
            property int aux2Channel:       rc_map_aux2.value

            property int modeChannel:       rc_map_mode_sw.value
            property int posCtlChannel:     rc_map_posctl_sw.value
            property int returnChannel:     rc_map_return_sw.value
            property int offboardChannel:   rc_map_offboard_sw.value
            property int loiterChannel:     rc_map_loiter_sw.value
            property int acroChannel:       rc_map_acro_sw.value

            property real rcThUserValue: rc_th_user.value

            readonly property int channelCount: controller.channelCount

            property bool inRedistribution: false

            readonly property int tileWidth: 150
            readonly property int tileHeight: 30

            readonly property int progressBarHeight: 200

            anchors.fill: parent

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
            // The following properties must be set in the Loader:
            //  tileLabel - label for tile
            //  tileParam - parameter to load from
            //  tileDragEnabled - true: this tile can be dragged
            Component {
                id: unassignedModeTileComponent

                Rectangle {
                    property Fact fact: controller.getParameterFact(-1, tileParam)
                    property bool dragEnabled: fact.value == 0

                    id:             outerRect
                    width:          tileWidth
                    height:         tileHeight
                    color:          qgcPal.windowShadeDark
                    border.width:   dragEnabled ? 1 : 0
                    border.color:   qgcPal.text

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
                                if (!singleSwitchRequired || parent.Drag.target.unassignedChannel) {
                                    fact.value = parent.Drag.target.channel
                                }
                            }
                        }
                    }

                    DropArea {
                        // This will cause to tile to go back to unassigned if dropped here
                        readonly property int channel:      0
                        property bool dropAllowed:          true
                        property bool unassignedChannel:    true


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
            //  tileParam - parameter to load from
            Component {
                id: assignedModeTileComponent

                Rectangle {
                    Fact{ id: nullFact }
                    property Fact fact: tileDragEnabled ? controller.getParameterFact(-1, tileParam) : nullFact

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
                                if (!singleSwitchRequired || parent.Drag.target.unassignedChannel) {
                                    fact.value = parent.Drag.target.channel
                                }
                            }
                        }
                    }
                }
            }

            onModeChannelChanged: if (!inRedistribution) redistributeThresholds()
            onReturnChannelChanged: if (!inRedistribution) redistributeThresholds()
            onOffboardChannelChanged: if (!inRedistribution) redistributeThresholds()
            onLoiterChannelChanged: if (!inRedistribution) redistributeThresholds()
            onPosCtlChannelChanged: if (!inRedistribution) redistributeThresholds()
            onAcroChannelChanged: if (!inRedistribution) redistributeThresholds()
            onRcThUserValue: if (!inRedistribution) redistributeThresholds()

            function redistributeThresholds() {
                if (loading || rcThUserValue != 0) {
                    // User is specifying thresholds, do not auto-calculate
                    return
                }

                if (modeChannel != 0) {
                    var positions = 3  // Manual/Assist/Auto always exist

                    var loiterOnModeSwitch = modeChannel == loiterChannel
                    var posCtlOnModeSwitch = modeChannel == posCtlChannel
                    var acroOnModeSwitch = modeChannel == acroChannel

                    positions += loiterOnModeSwitch ? 1 : 0
                    positions += posCtlOnModeSwitch ? 1 : 0
                    positions += acroOnModeSwitch ? 1 : 0

                    var increment = 1.0 / positions
                    var currentThreshold = 0.0

                    // Make sure we don't re-enter
                    inRedistribution = true

                    if (acroOnModeSwitch) {
                        currentThreshold += increment
                        rc_acro_th.value = currentThreshold
                    }

                    currentThreshold += increment
                    rc_assist_th.value = currentThreshold
                    if (posCtlOnModeSwitch) {
                        currentThreshold += increment
                        rc_posctl_th.value = currentThreshold
                    }

                    currentThreshold += increment
                    rc_auto_th.value = currentThreshold
                    if (loiterOnModeSwitch) {
                        currentThreshold += increment
                        rc_loiter_th.value = currentThreshold
                    }

                    inRedistribution = false
                }

                if (returnChannel != 0) {
                    inRedistribution = true

                    // If only two positions don't set threshold at midrange. Setting to 0.25
                    // allows for this channel to work with either two or three position switch
                    rc_return_th.value = 0.25

                    inRedistribution = false
                }

                if (offboardChannel != 0) {
                    inRedistribution = true

                    // If only two positions don't set threshold at midrange. Setting to 0.25
                    // allows for this channel to work with either two or three position switch
                    rc_offboard_th.value = 0.25

                    inRedistribution = false
                }

                if (loiterChannel != 0 && loiterChannel != modeChannel) {
                    inRedistribution = true

                    // If only two positions don't set threshold at midrange. Setting to 0.25
                    // allows for this channel to work with either two or three position switch
                    rc_loiter_th.value = 0.25

                    inRedistribution = false
                }

                if (posCtlChannel != 0 & posCtlChannel != modeChannel) {
                    inRedistribution = true

                    // If only two positions don't set threshold at midrange. Setting to 0.25
                    // allows for this channel to work with either two or three position switch
                    rc_posctl_th.value = 0.25

                    inRedistribution = false
                }

                if (acroChannel != 0 & acroChannel != modeChannel) {
                    inRedistribution = true

                    // If only two positions don't set threshold at midrange. Setting to 0.25
                    // allows for this channel to work with either two or three position switch
                    rc_acro_th.value = 0.25

                    inRedistribution = false
                }
            }

            Column {
                anchors.fill: parent

                QGCLabel {
                    text: "FLIGHT MODES CONFIG"
                    font.pixelSize: ScreenTools.largeFontPixelSize
                }

                Item { height: 20; width: 10 } // spacer

                QGCLabel {
                    width: parent.width
                    text: "The Main Mode, Loiter, PostCtl and Acro switches can be assigned to any channel which is not currently being used for attitude control. The Return and Offboard switches must be assigned to their seperate channel. " +
                            "All channels are displayed below. " +
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
                        model: channelCount

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
                            property bool offboardMapped:   channel == offboardChannel
                            property bool loiterMapped:     channel == loiterChannel
                            property bool acroMapped:       channel == acroChannel

                            property bool nonFlightModeMapping: throttleMapped | yawMapped | pitchMapped | rollMapped | flapsMapped | aux1Mapped | aux2Mapped
                            property bool unassignedMapping: !(nonFlightModeMapping | modeMapped | posCtlMapped | returnMapped | offboardMapped | loiterMapped | acroMapped)
                            property bool singleSwitchMapping: returnMapped | offboardMapped

                            id:     channelTarget
                            width:  tileWidth
                            height: channelCol.implicitHeight

                            color:  qgcPal.windowShadeDark

                            states: [
                                State {
                                    when: dropArea.containsDrag && dropArea.dropAllowed && (!dropArea.drag.source.singleSwitchRequired || dropArea.unassignedChannel)
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
                                    property string tileLabel:          "Main Mode"
                                    property bool tileVisible:          visible
                                    property bool tileDragEnabled:      true
                                    property string tileParam:          "RC_MAP_MODE_SW"
                                    property bool singleSwitchRequired: false

                                    visible:            modeMapped
                                    sourceComponent:    assignedModeTileComponent
                                }
                                Loader {
                                    property string tileLabel:          "Return"
                                    property bool tileVisible:          visible
                                    property bool tileDragEnabled:      true
                                    property string tileParam:          "RC_MAP_RETURN_SW"
                                    property bool singleSwitchRequired: true

                                    visible:            returnMapped
                                    sourceComponent:    assignedModeTileComponent
                                }
                                Loader {
                                    property string tileLabel:          "Offboard"
                                    property bool tileVisible:          visible
                                    property bool tileDragEnabled:      true
                                    property string tileParam:          "RC_MAP_OFFB_SW"
                                    property bool singleSwitchRequired: true

                                    visible:            offboardMapped
                                    sourceComponent:    assignedModeTileComponent
                                }
                                Loader {
                                    property string tileLabel:          "Loiter"
                                    property bool tileVisible:          visible
                                    property bool tileDragEnabled:      true
                                    property string tileParam:          "RC_MAP_LOITER_SW"
                                    property bool singleSwitchRequired: false

                                    visible:            loiterMapped
                                    sourceComponent:    assignedModeTileComponent
                                }
                                Loader {
                                    property string tileLabel:          "PosCtl"
                                    property bool tileVisible:          visible
                                    property bool tileDragEnabled:      true
                                    property string tileParam:          "RC_MAP_POSCTL_SW"
                                    property bool singleSwitchRequired: false

                                    visible:            posCtlMapped
                                    sourceComponent:    assignedModeTileComponent
                                }
                                Loader {
                                    property string tileLabel:          "Acro/Stabilize"
                                    property bool tileVisible:          visible
                                    property bool tileDragEnabled:      true
                                    property string tileParam:          "RC_MAP_ACRO_SW"
                                    property bool singleSwitchRequired: false

                                    visible:            acroMapped
                                    sourceComponent:    assignedModeTileComponent
                                }
                            }

                            DropArea {
                                // Drops are not allowed on channels which are mapped to non-flight mode switches
                                property bool dropAllowed: !nonFlightModeMapping && !singleSwitchMapping
                                property bool unassignedChannel: unassignedMapping
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

                Row {
                    spacing: 5

                    QGCLabel {
                        text: "Flight Modes"
                    }
                    QGCLabel {
                        text: "(Mode Switch must be assigned to channel before flight is allowed)"
                        visible: rc_map_mode_sw.value == 0
                    }
                }
                Flow {
                    width: parent.width
                    spacing: 5

                    Loader {
                        property string tileLabel:          "Main Mode"
                        property string tileParam:          "RC_MAP_MODE_SW"
                        property bool singleSwitchRequired: false
                        sourceComponent:                    unassignedModeTileComponent
                    }
                    Loader {
                        property string tileLabel:          "Loiter"
                        property string tileParam:          "RC_MAP_LOITER_SW"
                        property bool singleSwitchRequired: false
                        sourceComponent:                    unassignedModeTileComponent
                    }
                    Loader {
                        property string tileLabel:          "PosCtl"
                        property string tileParam:          "RC_MAP_POSCTL_SW"
                        property bool singleSwitchRequired: false
                        sourceComponent:                    unassignedModeTileComponent
                    }
                    Loader {
                        property string tileLabel:          "Acro/Stabilize"
                        property string tileParam:          "RC_MAP_ACRO_SW"
                        property bool singleSwitchRequired: false
                        sourceComponent:                    unassignedModeTileComponent
                    }
                    Loader {
                        property string tileLabel:          "Return"
                        property string tileParam:          "RC_MAP_RETURN_SW"
                        property bool singleSwitchRequired: true
                        sourceComponent:                    unassignedModeTileComponent
                    }
                    Loader {
                        property string tileLabel:          "Offboard"
                        property string tileParam:          "RC_MAP_OFFB_SW"
                        property bool singleSwitchRequired: true
                        sourceComponent:                    unassignedModeTileComponent
                    }
                }

                Item { height: 20; width: 10 } // spacer

                FactCheckBox {
                    checkedValue:   0
                    uncheckedValue: 1
                    fact:           rc_th_user
                    text:           "Allow setup to generate the thresholds for the flight mode positions within a switch (recommended)"
                }

                Item { height: 20; width: 10 } // spacer

                Row {
                spacing: 20

                    QGCLabel {
                        text: "Switch Display"
                    }
                    QGCCheckBox {
                        checked:    controller.sendLiveRCSwitchRanges
                        text:       "Show live RC display"

                        onClicked: {
                            controller.sendLiveRCSwitchRanges = checked
                        }
                    }
                }

                Item { height: 20; width: 10 } // spacer

                Row {
                    property bool modeSwitchVisible:        modeChannel != 0
                    property bool loiterSwitchVisible:      loiterChannel != 0 && loiterChannel != modeChannel && loiterChannel != returnChannel
                    property bool posCtlSwitchVisible:      posCtlChannel != 0 && posCtlChannel != modeChannel
                    property bool acroSwitchVisible:        acroChannel != 0 && acroChannel != modeChannel
                    property bool returnSwitchVisible:      returnChannel != 0
                    property bool offboardSwitchVisible:    offboardChannel != 0

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
                                    y:                      (parent.height * (1.0 - rc_auto_th.value)) - (implicitHeight / 2)
                                    visible:                modeChannel != returnChannel && modeChannel != loiterChannel
                                    horizontalAlignment:    Text.AlignRight
                                    text:                   "Auto"
                                }

                                QGCLabel {
                                    width:                  parent.width
                                    y:                      (parent.height * (1.0 - rc_loiter_th.value)) - (implicitHeight / 2)
                                    visible:                modeChannel == loiterChannel
                                    horizontalAlignment:    Text.AlignRight
                                    text:                   "Auto: Loiter"
                                }

                                QGCLabel {
                                    width:                  parent.width
                                    y:                      (parent.height * (1.0 - rc_auto_th.value)) - (implicitHeight / 2)
                                    visible:                modeChannel == loiterChannel
                                    horizontalAlignment:    Text.AlignRight
                                    text:                   "Auto: Mission"
                                }

                                QGCLabel {
                                    width:                  parent.width
                                    y:                      (parent.height * (1.0 - rc_auto_th.value)) - (implicitHeight / 2)
                                    visible:                modeChannel == returnChannel && modeChannel != loiterChannel
                                    horizontalAlignment:    Text.AlignRight
                                    text:                   "Auto: Loiter/Mission"
                                }

                                QGCLabel {
                                    width:                  parent.width
                                    y:                      (parent.height * (1.0 - rc_assist_th.value)) - (implicitHeight / 2)
                                    visible:                modeChannel != posCtlChannel
                                    horizontalAlignment:    Text.AlignRight
                                    text:                   "Assist"
                                }

                                QGCLabel {
                                    width:                  parent.width
                                    y:                      (parent.height * (1.0 - rc_posctl_th.value)) - (implicitHeight / 2)
                                    visible:                modeChannel == posCtlChannel
                                    horizontalAlignment:    Text.AlignRight
                                    text:                   "Assist: PosCtl"
                                }

                                QGCLabel {
                                    width:                  parent.width
                                    y:                      (parent.height * (1.0 - rc_assist_th.value)) - (implicitHeight / 2)
                                    visible:                modeChannel == posCtlChannel
                                    horizontalAlignment:    Text.AlignRight
                                    text:                   "Assist: AltCtl"
                                }

                                QGCLabel {
                                    width:                  parent.width
                                    y:                      (parent.height * (1.0 - rc_acro_th.value)) - (implicitHeight / 2)
                                    visible:                modeChannel == acroChannel
                                    horizontalAlignment:    Text.AlignRight
                                    text:                   "Acro/Stabilize"
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
                                value:          controller.modeSwitchLiveRange
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
                                    y:                      (parent.height * (1.0 - rc_loiter_th.value)) - (implicitHeight / 2)
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
                                value:          controller.loiterSwitchLiveRange
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
                                    y:                      (parent.height * (1.0 - rc_posctl_th.value)) - (implicitHeight / 2)
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
                                value:          controller.posCtlSwitchLiveRange
                            }
                        }
                    }

                    Column {
                        visible: parent.acroSwitchVisible

                        QGCLabel { text: "Acro/Stabilize Switch" }

                        Row {
                            Item {
                                height: progressBarHeight
                                width:  150

                                QGCLabel {
                                    width:                  parent.width
                                    y:                      (parent.height * (1.0 - rc_acro_th.value)) - (implicitHeight / 2)
                                    horizontalAlignment:    Text.AlignRight
                                    text:                   "Acro/Stabilize"
                                }

                                QGCLabel {
                                    width:                  parent.width
                                    y:                      parent.height - (implicitHeight / 2)
                                    horizontalAlignment:    Text.AlignRight
                                    text:                   "Manual"
                                }
                            }

                            ProgressBar {
                                height:         progressBarHeight
                                orientation:    Qt.Vertical
                                value:          controller.acroSwitchLiveRange
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
                                    y:                      (parent.height * (1.0 - rc_return_th.value)) - (implicitHeight / 2)
                                    horizontalAlignment:    Text.AlignRight
                                    text:                   "Return"
                                }

                                QGCLabel {
                                    width:                  parent.width
                                    y:                      parent.height - (implicitHeight / 2)
                                    horizontalAlignment:    Text.AlignRight
                                    text:                   "Return Off"
                                }
                            }

                            ProgressBar {
                                height:         progressBarHeight
                                orientation:    Qt.Vertical
                                value:          controller.returnSwitchLiveRange
                            }
                        }
                    }

                    Column {
                        visible: parent.offboardSwitchVisible

                        QGCLabel { text: "Offboard Switch" }

                        Row {
                            Item {
                                height: progressBarHeight
                                width:  150

                                QGCLabel {
                                    width:                  parent.width
                                    y:                      (parent.height * (1.0 - rc_return_th.value)) - (implicitHeight / 2)
                                    horizontalAlignment:    Text.AlignRight
                                    text:                   "Offboard"
                                }

                                QGCLabel {
                                    width:                  parent.width
                                    y:                      parent.height - (implicitHeight / 2)
                                    visible:                returnChannel != loiterChannel
                                    horizontalAlignment:    Text.AlignRight
                                    text:                   "Offboard Off"
                                }
                            }

                            ProgressBar {
                                height:         progressBarHeight
                                orientation:    Qt.Vertical
                                value:          controller.offboardSwitchLiveRange
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: invalidComponent

        FactPanel {
            anchors.fill: parent
            color: qgcPal.window

            Column {
                width: parent.width
                spacing: 20

                QGCLabel {
                    text:           "FLIGHT MODES CONFIG"
                    font.pixelSize: ScreenTools.largeFontPixelSize
                }

                QGCLabel {
                    width:      parent.width
                    wrapMode:   Text.WordWrap
                    text:       "There are errors in your current configuration which will need to be fixed before you can used Flight Config setup. " +
                                "You will need to change Parameters directly using Parameters Setup to remove these errors."
                }

                QGCLabel {
                    width:      parent.width
                    wrapMode:   Text.WordWrap
                    text:       controller.configurationErrors
                }
            }
        }
    }
}
