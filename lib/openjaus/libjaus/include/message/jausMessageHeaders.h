/*****************************************************************************
 *  Copyright (c) 2008, University of Florida
 *  All rights reserved.
 *  
 *  This file is part of OpenJAUS.  OpenJAUS is distributed under the BSD 
 *  license.  See the LICENSE file for details.
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of the University of Florida nor the names of its 
 *       contributors may be used to endorse or promote products derived from 
 *       this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ****************************************************************************/
#ifndef JAUS_MESSAGE_HEADERS_H
#define JAUS_MESSAGE_HEADERS_H

#include "jausMessage.h"

// Command Class
#include "command/core/setComponentAuthorityMessage.h"
#include "command/core/shutdownMessage.h"
#include "command/core/standbyMessage.h"
#include "command/core/resumeMessage.h"
#include "command/core/resetMessage.h"
#include "command/core/setEmergencyMessage.h"
#include "command/core/clearEmergencyMessage.h"
#include "command/core/createServiceConnectionMessage.h"
#include "command/core/confirmServiceConnectionMessage.h"
#include "command/core/activateServiceConnectionMessage.h"
#include "command/core/suspendServiceConnectionMessage.h"
#include "command/core/terminateServiceConnectionMessage.h"
#include "command/core/requestComponentControlMessage.h"
#include "command/core/releaseComponentControlMessage.h"
#include "command/core/confirmComponentControlMessage.h"
#include "command/core/rejectComponentControlMessage.h"
#include "command/core/setTimeMessage.h"
#include "command/communications/setDataLinkStatusMessage.h"
#include "command/communications/setDataLinkSelectMessage.h"
#include "command/communications/setSelectedDataLinkStateMessage.h"
#include "command/environment/selectCameraMessage.h"
#include "command/environment/setCameraPoseMessage.h"
#include "command/environment/setCameraCapabilitiesMessage.h"
#include "command/environment/setCameraFormatOptionsMessage.h"
#include "command/manipulator/setJointEffortsMessage.h"
#include "command/manipulator/setJointPositionsMessage.h"
#include "command/manipulator/setJointVelocitiesMessage.h"
#include "command/manipulator/setToolPointMessage.h"
#include "command/manipulator/setEndEffectorPoseMessage.h"
#include "command/manipulator/setEndEffectorVelocityStateMessage.h"
#include "command/manipulator/setJointMotionMessage.h"
#include "command/manipulator/setEndEffectorPathMotionMessage.h"
#include "command/platform/setWrenchEffortMessage.h"
#include "command/platform/setDiscreteDevicesMessage.h"
#include "command/platform/setGlobalVectorMessage.h"
#include "command/platform/setLocalVectorMessage.h"
#include "command/platform/setTravelSpeedMessage.h"
#include "command/platform/setGlobalWaypointMessage.h"
#include "command/platform/setLocalWaypointMessage.h"
#include "command/platform/setVelocityStateMessage.h"
#include "command/platform/setGlobalPathSegmentMessage.h"
#include "command/platform/setLocalPathSegmentMessage.h"
#include "command/planning/abortMissionMessage.h"
#include "command/planning/pauseMissionMessage.h"
#include "command/planning/removeMessagesMessage.h"
#include "command/planning/resumeMissionMessage.h"
#include "command/planning/runMissionMessage.h"
#include "command/planning/spoolMissionMessage.h"
#include "command/planning/replaceMessagesMessage.h"
#include "command/worldModel/createVksObjectsMessage.h"
#include "command/worldModel/deleteVksObjectsMessage.h"
#include "command/worldModel/setVksFeatureClassMetadataMessage.h"
#include "command/worldModel/terminateVksDataTransferMessage.h"
#include "command/payload/setPayloadDataElementMessage.h"
#include "command/event/cancelEventMessage.h"
#include "command/event/confirmEventRequestMessage.h"
#include "command/event/createEventMessage.h"
#include "command/event/updateEventMessage.h"
#include "command/event/rejectEventRequestMessage.h"


// Inform Class
#include "inform/core/reportComponentAuthorityMessage.h"
#include "inform/core/reportComponentStatusMessage.h"
#include "inform/core/reportTimeMessage.h"
#include "inform/core/reportComponentControlMessage.h"
#include "inform/communications/reportDataLinkStatusMessage.h"
#include "inform/communications/reportSelectedDataLinkStatusMessage.h"
#include "inform/communications/reportHeartbeatPulseMessage.h"
#include "inform/environment/reportImageMessage.h"
#include "inform/environment/reportCameraPoseMessage.h"
#include "inform/environment/reportCameraCountMessage.h"
#include "inform/environment/reportCameraCapabilitiesMessage.h"
#include "inform/environment/reportCameraFormatOptionsMessage.h"
#include "inform/environment/reportRelativeObjectPositionMessage.h"
#include "inform/environment/reportSelectedCameraMessage.h"
#include "inform/manipulator/reportManipulatorSpecificationsMessage.h"
#include "inform/manipulator/reportJointEffortsMessage.h"
#include "inform/manipulator/reportJointPositionsMessage.h"
#include "inform/manipulator/reportJointVelocitiesMessage.h"
#include "inform/manipulator/reportJointForceTorquesMessage.h"
#include "inform/manipulator/reportToolPointMessage.h"
#include "inform/platform/reportPlatformSpecificationsMessage.h"
#include "inform/platform/reportPlatformOperationalDataMessage.h"
#include "inform/platform/reportGlobalPoseMessage.h"
#include "inform/platform/reportLocalPoseMessage.h"
#include "inform/platform/reportVelocityStateMessage.h"
#include "inform/platform/reportWrenchEffortMessage.h"
#include "inform/platform/reportDiscreteDevicesMessage.h"
#include "inform/platform/reportGlobalVectorMessage.h"
#include "inform/platform/reportLocalVectorMessage.h"
#include "inform/platform/reportTravelSpeedMessage.h"
#include "inform/platform/reportWaypointCountMessage.h"
#include "inform/platform/reportGlobalWaypointMessage.h"
#include "inform/platform/reportLocalWaypointMessage.h"
#include "inform/platform/reportPathSegmentCountMessage.h"
#include "inform/platform/reportGlobalPathSegmentMessage.h"
#include "inform/platform/reportLocalPathSegmentMessage.h"
#include "inform/planning/reportMissionStatusMessage.h"
#include "inform/planning/reportSpoolingPreferenceMessage.h"
#include "inform/dynamicConfiguration/reportIdentificationMessage.h"
#include "inform/dynamicConfiguration/reportConfigurationMessage.h"
#include "inform/dynamicConfiguration/reportServicesMessage.h"
#include "inform/dynamicConfiguration/reportSubsystemListMessage.h"
#include "inform/payload/reportPayloadDataElementMessage.h"
#include "inform/payload/reportPayloadInterfaceMessage.h"
#include "inform/worldModel/reportVksBoundsMessage.h"
#include "inform/worldModel/reportVksFeatureClassMetadataMessage.h"
#include "inform/worldModel/reportVksObjectsCreationMessage.h"
#include "inform/worldModel/reportVksObjectsMessage.h"
#include "inform/worldModel/reportVksDataTransferTerminationMessage.h"
#include "inform/event/reportEventsMessage.h"
#include "inform/event/eventMessage.h"

// Query Class
#include "query/core/queryComponentAuthorityMessage.h"
#include "query/core/queryComponentStatusMessage.h"
#include "query/core/queryTimeMessage.h"
#include "query/core/queryComponentControlMessage.h"
#include "query/communications/queryDataLinkStatusMessage.h"
#include "query/communications/querySelectedDataLinkStatusMessage.h"
#include "query/environment/queryImageMessage.h"
#include "query/environment/queryCameraPoseMessage.h"
#include "query/environment/queryCameraCountMessage.h"
#include "query/environment/queryCameraCapabilitiesMessage.h"
#include "query/environment/queryCameraFormatOptionsMessage.h"
#include "query/environment/queryRelativeObjectPositionMessage.h"
#include "query/environment/querySelectedCameraMessage.h"
#include "query/manipulator/queryManipulatorSpecificationsMessage.h"
#include "query/manipulator/queryJointEffortsMessage.h"
#include "query/manipulator/queryJointPositionsMessage.h"
#include "query/manipulator/queryJointVelocitiesMessage.h"
#include "query/manipulator/queryToolPointMessage.h"
#include "query/manipulator/queryJointForceTorquesMessage.h"
#include "query/platform/queryPlatformSpecificationsMessage.h"
#include "query/platform/queryPlatformOperationalDataMessage.h"
#include "query/platform/queryGlobalPoseMessage.h"
#include "query/platform/queryLocalPoseMessage.h"
#include "query/platform/queryVelocityStateMessage.h"
#include "query/platform/queryWrenchEffortMessage.h"
#include "query/platform/queryDiscreteDevicesMessage.h"
#include "query/platform/queryGlobalVectorMessage.h"
#include "query/platform/queryLocalVectorMessage.h"
#include "query/platform/queryTravelSpeedMessage.h"
#include "query/platform/queryWaypointCountMessage.h"
#include "query/platform/queryGlobalWaypointMessage.h"
#include "query/platform/queryLocalWaypointMessage.h"
#include "query/platform/queryPathSegmentCountMessage.h"
#include "query/platform/queryGlobalPathSegmentMessage.h"
#include "query/platform/queryLocalPathSegmentMessage.h"
#include "query/planning/queryMissionStatusMessage.h"
#include "query/planning/querySpoolingPreferenceMessage.h"
#include "query/communications/queryHeartbeatPulseMessage.h"
#include "query/dynamicConfiguration/queryIdentificationMessage.h"
#include "query/dynamicConfiguration/queryConfigurationMessage.h"
#include "query/dynamicConfiguration/queryServicesMessage.h"
#include "query/dynamicConfiguration/querySubsystemListMessage.h"
#include "query/payload/queryPayloadDataElementMessage.h"
#include "query/payload/queryPayloadInterfaceMessage.h"
#include "query/worldModel/queryVksBoundsMessage.h"
#include "query/worldModel/queryVksFeatureClassMetadataMessage.h"
#include "query/worldModel/queryVksObjectsMessage.h"
#include "query/event/queryEventsMessage.h"

// Experimental

#endif //JAUS_MESSAGE_HEADERS_H
