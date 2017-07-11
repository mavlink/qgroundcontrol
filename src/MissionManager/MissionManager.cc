/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "MissionManager.h"
#include "Vehicle.h"
#include "FirmwarePlugin.h"
#include "MAVLinkProtocol.h"
#include "QGCApplication.h"
#include "MissionCommandTree.h"
#include "MissionCommandUIInfo.h"

QGC_LOGGING_CATEGORY(MissionManagerLog, "MissionManagerLog")

MissionManager::MissionManager(Vehicle* vehicle)
    : PlanManager(vehicle, MAV_MISSION_TYPE_MISSION)
{

}

MissionManager::~MissionManager()
{

}
void MissionManager::writeArduPilotGuidedMissionItem(const QGeoCoordinate& gotoCoord, bool altChangeOnly)
{
    if (inProgress()) {
        qCDebug(MissionManagerLog) << "writeArduPilotGuidedMissionItem called while transaction in progress";
        return;
    }

    _transactionInProgress = TransactionWrite;

    mavlink_message_t       messageOut;
    mavlink_mission_item_t  missionItem;

    memset(&missionItem, 8, sizeof(missionItem));
    missionItem.target_system =     _vehicle->id();
    missionItem.target_component =  _vehicle->defaultComponentId();
    missionItem.seq =               0;
    missionItem.command =           MAV_CMD_NAV_WAYPOINT;
    missionItem.param1 =            0;
    missionItem.param2 =            0;
    missionItem.param3 =            0;
    missionItem.param4 =            0;
    missionItem.x =                 gotoCoord.latitude();
    missionItem.y =                 gotoCoord.longitude();
    missionItem.z =                 gotoCoord.altitude();
    missionItem.frame =             MAV_FRAME_GLOBAL_RELATIVE_ALT;
    missionItem.current =           altChangeOnly ? 3 : 2;
    missionItem.autocontinue =      true;

    _dedicatedLink = _vehicle->priorityLink();
    mavlink_msg_mission_item_encode_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                         qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                         _dedicatedLink->mavlinkChannel(),
                                         &messageOut,
                                         &missionItem);

    _vehicle->sendMessageOnLink(_dedicatedLink, messageOut);
    _startAckTimeout(AckGuidedItem);
    emit inProgressChanged(true);
}

void MissionManager::generateResumeMission(int resumeIndex)
{
    if (_vehicle->isOfflineEditingVehicle()) {
        return;
    }

    if (inProgress()) {
        qCDebug(MissionManagerLog) << "generateResumeMission called while transaction in progress";
        return;
    }

    for (int i=0; i<_missionItems.count(); i++) {
        MissionItem* item = _missionItems[i];
        if (item->command() == MAV_CMD_DO_JUMP) {
            qgcApp()->showMessage(tr("Unable to generate resume mission due to MAV_CMD_DO_JUMP command."));
            return;
        }
    }

    resumeIndex = qMin(resumeIndex, _missionItems.count() - 1);

    int seqNum = 0;
    QList<MissionItem*> resumeMission;

    QList<MAV_CMD> includedResumeCommands;

    // If any command in this list occurs before the resumeIndex it will be added to the front of the mission
    includedResumeCommands << MAV_CMD_DO_CONTROL_VIDEO
                           << MAV_CMD_DO_SET_ROI
                           << MAV_CMD_DO_DIGICAM_CONFIGURE
                           << MAV_CMD_DO_DIGICAM_CONTROL
                           << MAV_CMD_DO_MOUNT_CONFIGURE
                           << MAV_CMD_DO_MOUNT_CONTROL
                           << MAV_CMD_DO_SET_CAM_TRIGG_DIST
                           << MAV_CMD_DO_FENCE_ENABLE
                           << MAV_CMD_IMAGE_START_CAPTURE
                           << MAV_CMD_IMAGE_STOP_CAPTURE
                           << MAV_CMD_VIDEO_START_CAPTURE
                           << MAV_CMD_VIDEO_STOP_CAPTURE
                           << MAV_CMD_DO_CHANGE_SPEED;
    if (_vehicle->fixedWing() && _vehicle->px4Firmware()) {
        includedResumeCommands << MAV_CMD_NAV_TAKEOFF;
    }

    bool addHomePosition = _vehicle->firmwarePlugin()->sendHomePositionToVehicle();
    int setCurrentIndex = addHomePosition ? 1 : 0;

    int resumeCommandCount = 0;
    for (int i=0; i<_missionItems.count(); i++) {
        MissionItem* oldItem = _missionItems[i];
        if ((i == 0 && addHomePosition) || i >= resumeIndex || includedResumeCommands.contains(oldItem->command())) {
            if (i < resumeIndex) {
                resumeCommandCount++;
            }
            MissionItem* newItem = new MissionItem(*oldItem, this);
            newItem->setIsCurrentItem( i == setCurrentIndex);
            newItem->setSequenceNumber(seqNum++);
            resumeMission.append(newItem);
        }
    }

    // De-dup and remove no-ops from the commands which were added to the front of the mission
    bool foundROI = false;
    bool foundCamTrigDist = false;
    QList<int> imageStartCameraIds;
    QList<int> imageStopCameraIds;
    QList<int> videoStartCameraIds;
    QList<int> videoStopCameraIds;
    while (resumeIndex >= 0) {
        MissionItem* resumeItem = resumeMission[resumeIndex];
        switch (resumeItem->command()) {
        case MAV_CMD_DO_SET_ROI:
            // Only keep the last one
            if (foundROI) {
                resumeMission.removeAt(resumeIndex);
            }
            foundROI = true;
            break;
        case MAV_CMD_DO_SET_CAM_TRIGG_DIST:
            // Only keep the last one
            if (foundCamTrigDist) {
                resumeMission.removeAt(resumeIndex);
            }
            foundCamTrigDist = true;
            break;
        case MAV_CMD_IMAGE_START_CAPTURE:
        {
            // FIXME: Handle single image capture
            int cameraId = resumeItem->param1();

            if (resumeItem->param3() == 1) {
                // This is an individual image capture command, remove it
                resumeMission.removeAt(resumeIndex);
                break;
            }
            // If we already found an image stop, then all image start/stop commands are useless
            // De-dup repeated image start commands
            // Otherwise keep only the last image start
            if (imageStopCameraIds.contains(cameraId) || imageStartCameraIds.contains(cameraId)) {
                resumeMission.removeAt(resumeIndex);
            }
            if (!imageStopCameraIds.contains(cameraId)) {
                imageStopCameraIds.append(cameraId);
            }
        }
            break;
        case MAV_CMD_IMAGE_STOP_CAPTURE:
        {
            int cameraId = resumeItem->param1();
            // Image stop only matters to kill all previous image starts
            if (!imageStopCameraIds.contains(cameraId)) {
                imageStopCameraIds.append(cameraId);
            }
            resumeMission.removeAt(resumeIndex);
        }
            break;
        case MAV_CMD_VIDEO_START_CAPTURE:
        {
            int cameraId = resumeItem->param1();
            // If we've already found a video stop, then all video start/stop commands are useless
            // De-dup repeated video start commands
            // Otherwise keep only the last video start
            if (videoStopCameraIds.contains(cameraId) || videoStopCameraIds.contains(cameraId)) {
                resumeMission.removeAt(resumeIndex);
            }
            if (!videoStopCameraIds.contains(cameraId)) {
                videoStopCameraIds.append(cameraId);
            }
        }
            break;
        case MAV_CMD_VIDEO_STOP_CAPTURE:
        {
            int cameraId = resumeItem->param1();
            // Video stop only matters to kill all previous video starts
            if (!videoStopCameraIds.contains(cameraId)) {
                videoStopCameraIds.append(cameraId);
            }
            resumeMission.removeAt(resumeIndex);
        }
            break;
        default:
            break;
        }

        resumeIndex--;
    }

    // Send to vehicle
    _clearAndDeleteWriteMissionItems();
    for (int i=0; i<resumeMission.count(); i++) {
        _writeMissionItems.append(new MissionItem(*resumeMission[i], this));
    }
    _resumeMission = true;
    _writeMissionItemsWorker();

    // Clean up no longer needed resume items
    for (int i=0; i<resumeMission.count(); i++) {
        resumeMission[i]->deleteLater();
    }
}
