/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QGroundControl           1.0
import QGroundControl.Controls  1.0

ToolStrip {
    title: qsTr("Fly")

    property var  guidedActionsController
    property var  guidedActionList
    property bool usePreFlightChecklist

    signal displayPreFlightChecklist

    property bool _anyActionAvailable: guidedActionsController.showStartMission || guidedActionsController.showResumeMission || guidedActionsController.showChangeAlt || guidedActionsController.showLandAbort
    property var _actionModel: [
        {
            title:      guidedActionsController.startMissionTitle,
            text:       guidedActionsController.startMissionMessage,
            action:     guidedActionsController.actionStartMission,
            visible:    guidedActionsController.showStartMission
        },
        {
            title:      guidedActionsController.continueMissionTitle,
            text:       guidedActionsController.continueMissionMessage,
            action:     guidedActionsController.actionContinueMission,
            visible:    guidedActionsController.showContinueMission
        },
        {
            title:      guidedActionsController.changeAltTitle,
            text:       guidedActionsController.changeAltMessage,
            action:     guidedActionsController.actionChangeAlt,
            visible:    guidedActionsController.showChangeAlt
        },
        {
            title:      guidedActionsController.landAbortTitle,
            text:       guidedActionsController.landAbortMessage,
            action:     guidedActionsController.actionLandAbort,
            visible:    guidedActionsController.showLandAbort
        }
    ]

    model: [
        {
            name:               "Checklist",
            iconSource:         "/qmlimages/check.svg",
            buttonVisible:      usePreFlightChecklist,
            buttonEnabled:      usePreFlightChecklist && activeVehicle && !activeVehicle.armed,
        },
        {
            name:               guidedActionsController.takeoffTitle,
            iconSource:         "/res/takeoff.svg",
            buttonVisible:      guidedActionsController.showTakeoff || !guidedActionsController.showLand,
            buttonEnabled:      guidedActionsController.showTakeoff,
            action:             guidedActionsController.actionTakeoff
        },
        {
            name:               guidedActionsController.landTitle,
            iconSource:         "/res/land.svg",
            buttonVisible:      guidedActionsController.showLand && !guidedActionsController.showTakeoff,
            buttonEnabled:      guidedActionsController.showLand,
            action:             guidedActionsController.actionLand
        },
        {
            name:               guidedActionsController.rtlTitle,
            iconSource:         "/res/rtl.svg",
            buttonVisible:      true,
            buttonEnabled:      guidedActionsController.showRTL,
            action:             guidedActionsController.actionRTL
        },
        {
            name:               guidedActionsController.pauseTitle,
            iconSource:         "/res/pause-mission.svg",
            buttonVisible:      guidedActionsController.showPause,
            buttonEnabled:      guidedActionsController.showPause,
            action:             guidedActionsController.actionPause
        },
        {
            name:               qsTr("Action"),
            iconSource:         "/res/action.svg",
            buttonVisible:      _anyActionAvailable,
            buttonEnabled:      true,
            action:             -1
        }
    ]

    onClicked: {
        if(index === 0) {
            displayPreFlightChecklist()
        } else {
            guidedActionsController.closeAll()
            var action = model[index].action
            if (action === -1) {
                guidedActionList.model   = _actionModel
                guidedActionList.visible = true
            } else {
                guidedActionsController.confirmAction(action)
            }
        }

    }
}
