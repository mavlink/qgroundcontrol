/*****************************************************************************
 *  Copyright (c) 2008, University of Florida.
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
// File:		wd.c 
//
// Version:		1.0
//
// Written by:	Danny Kent (jaus AT dannykent DOT com)
//
// Date:		08/24/06
//
// Description:	This file contains the skeleton C code for implementing a JAUS component in a Linux environment
//				This code is designed to work with the node manager and JAUS library software written by CIMAR

#include <jaus.h>			// JAUS message set (USER: JAUS libraries must be installed first)
#include <openJaus.h>	// Node managment functions for sending and receiving JAUS messages (USER: Node Manager must be installed)
#include <stdlib.h>	
#include <string.h>
#include "utm/utmLib.h"
// USER: Add include files here as appropriate

#include "wd.h"
#include "vehicle.h"

#if defined (WIN32)
	#define _USE_MATH_DEFINES
	#include <math.h>
#endif

// Incoming Service Connection Defines
#define WD_INC_SC_TIMEOUT_SEC 			1.0		// The timeout between receiving service connection messages
#define WD_INC_SC_UPDATE_RATE_HZ 		20.0	// Requested service connection update rate
#define WD_INC_SC_PRESENCE_VECTOR 		0xFF	// The GPOS Presence Vector, set this to the fields desired (default = ALL)
#define WD_INC_SC_QUEUE_SIZE			1		// The Service Connection Manager's queue size (0 = infinite)

#define MPH_PER_MPS						2.23694
#define MPS_PER_MPH						0.44704
//#define ACCEL_GRAVITY_MPSPS				9.81	// Acceleration due to gravity

#define REQUEST_TIMEOUT_SEC				5.0 	// How long to wait between any requests

#define WAYPOINT_POP_DISTANCE_M						3.0
#define MISSION_FINISHED_DISTANCE_M					3.0
#define WD_DEFAULT_MIN_SPEED_MPS					1.0
#define WD_DEFAULT_MAX_SPEED_MPS					4.0 //11.18

// Gains
#define ACCELERATION_MPSPS				0.75	// Mps2
#define DECCELERATION_MPSPS				2.0		// Mps2
#define LINEAR_EFFORT_K_FF				14.0 	// %Effort / Mps
#define LINEAR_EFFORT_BIAS_FF  			-57.5	// %Effort
#define LINEAR_EFFORT_K_P				15.0 	// %Effort / (Mps Error)
#define LINEAR_EFFORT_K_I				6.5 	// %Effort / (M Error)
#define LINEAR_EFFORT_I_LIM				6.5 	// M Error
#define LINEAR_EFFORT_K_D				20.0
#define BRAKE_K							-1.3	// %Effort / %Effort
#define THROTTLE_K						0.65	// %Effort / %Effort
#define STICKTION_SPEED_MPS				0.5
#define STICKTION_EFFORT				-17.5
//#define PITCH_FF_EFFORT_PER_RAD			150
#define STEERING_FF_EFFORT				200
#define WHEEL_ROTATION_EFFORT_PER_RAD	100	
#define WHEEL_ROTATION_RAD_PER_EFFORT	0.006

// Private function prototypes
void wdProcessMessage(OjCmpt wd, JausMessage message);
void wdInitState(OjCmpt wd);
void wdStandbyState(OjCmpt wd);
void wdReadyState(OjCmpt wd);

// USER: Insert any private function prototypes here
void wdManagePdControl(OjCmpt wd);
void wdExcecuteControl(OjCmpt wd, VehicleState);
double wdAngleSubtract(double a, double b);

typedef struct
{
	ReportWrenchEffortMessage reportWrench;
	ReportGlobalPoseMessage reportGpos;
	ReportVelocityStateMessage reportVss;
	SetWrenchEffortMessage setWrench;
	SetTravelSpeedMessage setSpeed;
	SetGlobalWaypointMessage setWaypoint;

	int gposSc;
	int vssSc;
	int pdWrenchSc;
	int pdStatusSc;

	VehicleState vehicleState;

	JausState pdState;
	JausAddress pdAddress;
	
	JausBoolean inControl;
	JausBoolean requestControl;
	
	JausArray waypoints;					// Array of commaned waypoints
	int currentWaypoint;
	double waypointDistance;

}WdData;

OjCmpt wdCreate(void)
{
	OjCmpt cmpt;
	WdData *data;
	JausAddress addr;
	
	cmpt = ojCmptCreate("Waypoint Driver", JAUS_GLOBAL_WAYPOINT_DRIVER, WD_THREAD_DESIRED_RATE_HZ);

	ojCmptAddService(cmpt, JAUS_GLOBAL_WAYPOINT_DRIVER);

	ojCmptAddServiceInputMessage(cmpt, JAUS_GLOBAL_WAYPOINT_DRIVER, JAUS_SET_TRAVEL_SPEED, 0xFF);
	ojCmptAddServiceInputMessage(cmpt, JAUS_GLOBAL_WAYPOINT_DRIVER, JAUS_REPORT_GLOBAL_POSE, 0xFF);
	ojCmptAddServiceInputMessage(cmpt, JAUS_GLOBAL_WAYPOINT_DRIVER, JAUS_REPORT_VELOCITY_STATE, 0xFF);
	ojCmptAddServiceInputMessage(cmpt, JAUS_GLOBAL_WAYPOINT_DRIVER, JAUS_REPORT_WRENCH_EFFORT, 0xFF);
	ojCmptAddServiceInputMessage(cmpt, JAUS_GLOBAL_WAYPOINT_DRIVER, JAUS_SET_GLOBAL_WAYPOINT, 0xFF);
	ojCmptAddServiceInputMessage(cmpt, JAUS_GLOBAL_WAYPOINT_DRIVER, JAUS_QUERY_GLOBAL_WAYPOINT, 0xFF);
	ojCmptAddServiceInputMessage(cmpt, JAUS_GLOBAL_WAYPOINT_DRIVER, JAUS_QUERY_WAYPOINT_COUNT, 0xFF);
	ojCmptAddServiceOutputMessage(cmpt, JAUS_GLOBAL_WAYPOINT_DRIVER, JAUS_REPORT_WAYPOINT_COUNT, 0xFF);
	ojCmptAddServiceOutputMessage(cmpt, JAUS_GLOBAL_WAYPOINT_DRIVER, JAUS_REPORT_GLOBAL_WAYPOINT, 0xFF);

	ojCmptSetMessageProcessorCallback(cmpt, wdProcessMessage);
	ojCmptSetStateCallback(cmpt, JAUS_INITIALIZE_STATE, wdInitState);
	ojCmptSetStateCallback(cmpt, JAUS_STANDBY_STATE, wdStandbyState);
	ojCmptSetStateCallback(cmpt, JAUS_READY_STATE, wdReadyState);
	ojCmptSetState(cmpt, JAUS_INITIALIZE_STATE);
	
	ojCmptSetAuthority(cmpt, 127);
	
	addr = ojCmptGetAddress(cmpt);
	
	data = (WdData*)malloc(sizeof(WdData));
	
	data->setWrench = setWrenchEffortMessageCreate();
	jausAddressCopy(data->setWrench->source, addr);
	data->setWrench->presenceVector = 0;
	jausUnsignedShortSetBit(&data->setWrench->presenceVector, JAUS_WRENCH_PV_PROPULSIVE_LINEAR_X_BIT);
	jausUnsignedShortSetBit(&data->setWrench->presenceVector, JAUS_WRENCH_PV_PROPULSIVE_ROTATIONAL_Z_BIT);
	jausUnsignedShortSetBit(&data->setWrench->presenceVector, JAUS_WRENCH_PV_RESISTIVE_LINEAR_X_BIT);
	
	data->reportWrench = reportWrenchEffortMessageCreate();
	data->reportGpos = reportGlobalPoseMessageCreate();
	data->reportVss = reportVelocityStateMessageCreate();
	data->setSpeed = setTravelSpeedMessageCreate();
	data->setWaypoint = NULL;

	data->pdAddress = jausAddressCreate();
	data->pdAddress->subsystem = addr->subsystem;
	data->pdAddress->component = JAUS_PRIMITIVE_DRIVER;
	data->pdState = JAUS_UNDEFINED_STATE;
	
	data->inControl = JAUS_FALSE;
	data->requestControl = JAUS_FALSE;

	addr->subsystem = 0;
	addr->node = 0;
	addr->instance = 0;
	addr->component = JAUS_GLOBAL_POSE_SENSOR;
	data->gposSc = ojCmptEstablishSc(	cmpt, 
										JAUS_REPORT_GLOBAL_POSE,
										WD_INC_SC_PRESENCE_VECTOR, 
										addr, 
										WD_INC_SC_UPDATE_RATE_HZ, 
										WD_INC_SC_TIMEOUT_SEC, 
										WD_INC_SC_QUEUE_SIZE);
	addr->component = JAUS_VELOCITY_STATE_SENSOR;
	data->vssSc = ojCmptEstablishSc(	cmpt, 
										JAUS_REPORT_VELOCITY_STATE,
										WD_INC_SC_PRESENCE_VECTOR, 
										addr, 
										WD_INC_SC_UPDATE_RATE_HZ, 
										WD_INC_SC_TIMEOUT_SEC, 
										WD_INC_SC_QUEUE_SIZE);
	addr->component = JAUS_PRIMITIVE_DRIVER;
	data->pdWrenchSc = ojCmptEstablishSc(	cmpt, 
											JAUS_REPORT_WRENCH_EFFORT,
											WD_INC_SC_PRESENCE_VECTOR, 
											addr, 
											WD_INC_SC_UPDATE_RATE_HZ, 
											WD_INC_SC_TIMEOUT_SEC, 
											WD_INC_SC_QUEUE_SIZE);
	data->pdStatusSc = ojCmptEstablishSc(	cmpt, 
											JAUS_REPORT_COMPONENT_STATUS,
											WD_INC_SC_PRESENCE_VECTOR, 
											addr, 
											WD_INC_SC_UPDATE_RATE_HZ, 
											WD_INC_SC_TIMEOUT_SEC, 
											WD_INC_SC_QUEUE_SIZE);
	jausAddressDestroy(addr);	
	
	data->waypoints = jausArrayCreate();	
	data->vehicleState = vehicleStateCreate();
	data->waypointDistance = 0;
	data->currentWaypoint = 0;
	
	ojCmptSetUserData(cmpt, (void *)data);
		
	if(ojCmptRun(cmpt))
	{
		ojCmptDestroy(cmpt);
		return NULL;
	}

	return cmpt;
}

void wdDestroy(OjCmpt wd)
{
	ReleaseComponentControlMessage releaseControl = NULL;
	JausMessage txMessage;
	JausAddress address;
	WdData *data;
	
	data = (WdData*)ojCmptGetUserData(wd);

	if(ojCmptIsIncomingScActive(wd, data->gposSc))
	{
		ojCmptTerminateSc(wd, data->gposSc);
	}
	if(ojCmptIsIncomingScActive(wd, data->vssSc))
	{
		ojCmptTerminateSc(wd, data->vssSc);
	}
	if(ojCmptIsIncomingScActive(wd, data->pdWrenchSc))
	{
		ojCmptTerminateSc(wd, data->pdWrenchSc);
	}
	if(ojCmptIsIncomingScActive(wd, data->pdStatusSc))
	{
		ojCmptTerminateSc(wd, data->pdStatusSc);
	}

	if(data->inControl)
	{
		releaseControl = releaseComponentControlMessageCreate();
		address = ojCmptGetAddress(wd);
		jausAddressCopy(releaseControl->source, address);
		jausAddressDestroy(address);
		jausAddressCopy(releaseControl->destination, data->pdAddress);
		
		txMessage = releaseComponentControlMessageToJausMessage(releaseControl);
		ojCmptSendMessage(wd, txMessage);
		jausMessageDestroy(txMessage);
		
		releaseComponentControlMessageDestroy(releaseControl);
	}
	
	ojCmptDestroy(wd);

	// Destory Global Messages
	if(data->setSpeed)
	{
		setTravelSpeedMessageDestroy(data->setSpeed);
	}
	if(data->reportGpos)
	{
		reportGlobalPoseMessageDestroy(data->reportGpos);
	}
	if(data->reportVss)
	{
		reportVelocityStateMessageDestroy(data->reportVss);
	}
	if(data->reportWrench)
	{
		reportWrenchEffortMessageDestroy(data->reportWrench);
	}
	jausArrayDestroy(data->waypoints, (void *)setGlobalWaypointMessageDestroy);
	setWrenchEffortMessageDestroy(data->setWrench);
	jausAddressDestroy(data->pdAddress);
	vehicleStateDestroy(data->vehicleState);	
	free(data);
}

ReportWrenchEffortMessage wdGetReportedWrench(OjCmpt wd)
{ 
	WdData *data;
	data = (WdData*)ojCmptGetUserData(wd);
	
	if(ojCmptIsIncomingScActive(wd, data->pdWrenchSc))
	{
		return data->reportWrench; 
	}
	else
	{
		return NULL;
	}
}

SetWrenchEffortMessage wdGetCommandedWrench(OjCmpt wd)
{ 
	WdData *data;
	data = (WdData*)ojCmptGetUserData(wd);

	return data->setWrench; 
}

ReportVelocityStateMessage wdGetVss(OjCmpt wd)
{ 
	WdData *data;
	data = (WdData*)ojCmptGetUserData(wd);

	return data->reportVss; 
}

ReportGlobalPoseMessage wdGetGpos(OjCmpt wd)
{ 
	WdData *data;
	data = (WdData*)ojCmptGetUserData(wd);

	return data->reportGpos; 
}

SetTravelSpeedMessage wdGetTravelSpeed(OjCmpt wd)
{
	WdData *data;
	data = (WdData*)ojCmptGetUserData(wd);

	return data->setSpeed;
}

SetGlobalWaypointMessage wdGetGlobalWaypoint(OjCmpt wd)
{
	WdData *data;
	data = (WdData*)ojCmptGetUserData(wd);

	if(data->currentWaypoint < data->waypoints->elementCount)
	{
		return (SetGlobalWaypointMessage) data->waypoints->elementData[data->currentWaypoint];
	}
	else
	{
		return NULL;
	} 
}

JausState wdGetPdState(OjCmpt wd)
{
	WdData *data;
	data = (WdData*)ojCmptGetUserData(wd);

	return data->pdState;
}

JausBoolean wdGetPdWrenchScStatus(OjCmpt wd)
{
	WdData *data;
	data = (WdData*)ojCmptGetUserData(wd);

	return ojCmptIsIncomingScActive(wd, data->pdWrenchSc);
}

JausBoolean wdGetPdStatusScStatus(OjCmpt wd)
{
	WdData *data;
	data = (WdData*)ojCmptGetUserData(wd);

	return ojCmptIsIncomingScActive(wd, data->pdStatusSc);
}

JausBoolean wdGetVssScStatus(OjCmpt wd)
{
	WdData *data;
	data = (WdData*)ojCmptGetUserData(wd);

	return ojCmptIsIncomingScActive(wd, data->vssSc);
}

JausBoolean wdGetGposScStatus(OjCmpt wd)
{
	WdData *data;
	data = (WdData*)ojCmptGetUserData(wd);

	return ojCmptIsIncomingScActive(wd, data->gposSc);
}

JausBoolean wdGetInControlStatus(OjCmpt wd)
{
	WdData *data;
	data = (WdData*)ojCmptGetUserData(wd);

	return data->inControl;
}

VehicleState wdGetDesiredVehicleState(OjCmpt wd)
{
	WdData *data;
	data = (WdData*)ojCmptGetUserData(wd);

	return data->vehicleState;
}

double wdGetWaypointDistanceM(OjCmpt wd)
{
	WdData *data;
	data = (WdData*)ojCmptGetUserData(wd);

	return data->waypointDistance;
}

int wdGetActiveWaypoint(OjCmpt wd)
{
	WdData *data;
	data = (WdData*)ojCmptGetUserData(wd);

	return data->currentWaypoint;
}

int wdGetWaypointCount(OjCmpt wd)
{
	WdData *data;
	data = (WdData*)ojCmptGetUserData(wd);

	return data->waypoints->elementCount;
}

void wdToggleRequestControl(OjCmpt wd)
{
	WdData *data;
	data = (WdData*)ojCmptGetUserData(wd);

	data->requestControl = !data->requestControl;
}

JausBoolean wdGetRequestControl(OjCmpt wd)
{
	WdData *data;
	data = (WdData*)ojCmptGetUserData(wd);

	return data->requestControl;
}

void wdCreateWaypoint(OjCmpt wd)
{
	SetGlobalWaypointMessage setWaypoint = NULL;
	JausMessage message = NULL;
	
	setWaypoint = setGlobalWaypointMessageCreate();
	setWaypoint->latitudeDegrees = 29.001;
	setWaypoint->longitudeDegrees = -80.001;
	
	message = setGlobalWaypointMessageToJausMessage(setWaypoint);
	setGlobalWaypointMessageDestroy(setWaypoint);
	wdProcessMessage(wd, message);	
}

void wdSetSpeed(OjCmpt wd)
{
	SetTravelSpeedMessage setSpeed = NULL;
	JausMessage message = NULL;
	
	setSpeed = setTravelSpeedMessageCreate();
	setSpeed->speedMps = 4.0;
	
	message = setTravelSpeedMessageToJausMessage(setSpeed);
	setTravelSpeedMessageDestroy(setSpeed);
	wdProcessMessage(wd, message);
}

// Function: wdProcessMessage
// Access:		Private
// Description:	This function is responsible for handling incoming JAUS messages from the Node Manager.
//				Incoming messages are processed according to message type.
void wdProcessMessage(OjCmpt wd, JausMessage message)
{
	JausMessage txMessage;
	JausAddress address;
	ConfirmComponentControlMessage confirmComponentControl;
	RejectComponentControlMessage rejectComponentControl;
	ReportComponentStatusMessage reportComponentStatus;
	QueryGlobalWaypointMessage queryGlobalWaypointMessage;
	ReportGlobalWaypointMessage reportGlobalWaypointMessage;
	QueryWaypointCountMessage queryWaypointCountMessage;
	ReportWaypointCountMessage reportWaypointCountMessage;
	SetTravelSpeedMessage setTravelSpeed;
	ReportGlobalPoseMessage reportGpos;
	ReportVelocityStateMessage reportVss;
	ReportWrenchEffortMessage reportWrench;	
	SetGlobalWaypointMessage tempGlobalWaypoint;
	int i;
	WdData *data;
	
	data = (WdData*)ojCmptGetUserData(wd);

	// This block of code is intended to reject commands from non-controlling components
	if(ojCmptHasController(wd) && jausMessageIsRejectableCommand(message) )
	{
		address = ojCmptGetControllerAddress(wd);
		if(!jausAddressEqual(message->source, address))
		{		
			//jausAddressToString(message->source, buf);
			//cError("pd: Received command message %s from non-controlling component (%s).\n", jausMessageCommandCodeString(message), buf);
			jausAddressDestroy(address);
			return;
		}
		jausAddressDestroy(address);
	}	

	switch(message->commandCode) // Switch the processing algorithm according to the JAUS message type
	{	
		case JAUS_REPORT_COMPONENT_STATUS:
			reportComponentStatus = reportComponentStatusMessageFromJausMessage(message);
			if(reportComponentStatus)
			{
				if(jausAddressEqual(reportComponentStatus->source, data->pdAddress))
				{
					data->pdState = reportComponentStatus->primaryStatusCode;
				}
				reportComponentStatusMessageDestroy(reportComponentStatus);
			}
			break;

		case JAUS_CONFIRM_COMPONENT_CONTROL:
			confirmComponentControl = confirmComponentControlMessageFromJausMessage(message);
			if(confirmComponentControl)
			{
				if(jausAddressEqual(confirmComponentControl->source, data->pdAddress))
				{
					//cDebug(4,"wd: Confirmed control of PD\n");
					data->inControl = JAUS_TRUE;
				}
				confirmComponentControlMessageDestroy(confirmComponentControl);
			}
			break;
			
		case JAUS_REJECT_COMPONENT_CONTROL:
			rejectComponentControl = rejectComponentControlMessageFromJausMessage(message);
			if(rejectComponentControl)
			{
				if(jausAddressEqual(rejectComponentControl->source, data->pdAddress))
				{			
					//cDebug(4,"wd: Lost control of PD\n");
					data->inControl = JAUS_FALSE;
				}
				rejectComponentControlMessageDestroy(rejectComponentControl);
			}
			break;
			
		case JAUS_SET_TRAVEL_SPEED:
			setTravelSpeed = setTravelSpeedMessageFromJausMessage(message);
			if(setTravelSpeed)
			{
				setTravelSpeedMessageDestroy(data->setSpeed);
				data->setSpeed = setTravelSpeed;
			}
			break;

		case JAUS_REPORT_GLOBAL_POSE:
			reportGpos = reportGlobalPoseMessageFromJausMessage(message);
			if(reportGpos)
			{
				reportGlobalPoseMessageDestroy(data->reportGpos);
				data->reportGpos = reportGpos;
			}

			if(data->currentWaypoint < data->waypoints->elementCount)
			{
				// update waypoint index
				tempGlobalWaypoint = (SetGlobalWaypointMessage) data->waypoints->elementData[data->currentWaypoint];
				data->waypointDistance = greatCircleDistance(	data->reportGpos->latitudeDegrees * RAD_PER_DEG,
																data->reportGpos->longitudeDegrees * RAD_PER_DEG,
																tempGlobalWaypoint->latitudeDegrees * RAD_PER_DEG,
																tempGlobalWaypoint->longitudeDegrees * RAD_PER_DEG);
								
				if(data->waypointDistance < WAYPOINT_POP_DISTANCE_M)
				{
					//cError("wd: popping waypoint: %d\n",currentWaypointIndex); 
					data->currentWaypoint++;
				}
			}
			break;

		case JAUS_REPORT_VELOCITY_STATE:
			reportVss = reportVelocityStateMessageFromJausMessage(message);
			if(reportVss)
			{
				reportVelocityStateMessageDestroy(data->reportVss);
				data->reportVss = reportVss;
			}
			break;

		case JAUS_REPORT_WRENCH_EFFORT:
			reportWrench = reportWrenchEffortMessageFromJausMessage(message);
			if(reportWrench)
			{
				reportWrenchEffortMessageDestroy(data->reportWrench);
				data->reportWrench = reportWrench;
			}
			break;
			
		case JAUS_SET_GLOBAL_WAYPOINT:
			tempGlobalWaypoint = setGlobalWaypointMessageFromJausMessage(message);
			if(tempGlobalWaypoint)
			{
				data->setWaypoint = tempGlobalWaypoint;
				jausArrayAdd(data->waypoints, tempGlobalWaypoint);
			}
			break;

		case JAUS_QUERY_GLOBAL_WAYPOINT:
			queryGlobalWaypointMessage = queryGlobalWaypointMessageFromJausMessage(message);
			// loop thru waypoints to find the one that matches the request
			// if there's a match, prepare/send the report, else whine
			for(i = 0; i < data->waypoints->elementCount; i++)
			{
				tempGlobalWaypoint = (SetGlobalWaypointMessage) data->waypoints->elementData[i];
				if(tempGlobalWaypoint->waypointNumber == queryGlobalWaypointMessage->waypointNumber)
				{
					reportGlobalWaypointMessage = reportGlobalWaypointMessageCreate();
					jausAddressCopy(reportGlobalWaypointMessage->destination, queryGlobalWaypointMessage->source);
					address = ojCmptGetAddress(wd);
					jausAddressCopy(reportGlobalWaypointMessage->source, address);
					jausAddressDestroy(address);
					reportGlobalWaypointMessage->presenceVector = NO_PRESENCE_VECTOR;
					reportGlobalWaypointMessage->waypointNumber = tempGlobalWaypoint->waypointNumber;
					reportGlobalWaypointMessage->latitudeDegrees = tempGlobalWaypoint->latitudeDegrees;
					reportGlobalWaypointMessage->longitudeDegrees = tempGlobalWaypoint->longitudeDegrees;
					txMessage = reportGlobalWaypointMessageToJausMessage(reportGlobalWaypointMessage);
					reportGlobalWaypointMessageDestroy(reportGlobalWaypointMessage);
					ojCmptSendMessage(wd, txMessage);
					jausMessageDestroy(txMessage);
				}
			}
			queryGlobalWaypointMessageDestroy(queryGlobalWaypointMessage);
			break;
			
		case JAUS_QUERY_WAYPOINT_COUNT:
			queryWaypointCountMessage = queryWaypointCountMessageFromJausMessage(message);
			if(!queryWaypointCountMessage)
			{
				//cError("wd: Error unpacking %s message.\n", jausMessageCommandCodeString(message));
			}
			reportWaypointCountMessage = reportWaypointCountMessageCreate();
			jausAddressCopy(reportWaypointCountMessage->destination, queryWaypointCountMessage->source);
			address = ojCmptGetAddress(wd);
			jausAddressCopy(reportWaypointCountMessage->source, address);
			jausAddressDestroy(address);
			reportWaypointCountMessage->waypointCount = data->waypoints->elementCount;
			txMessage = reportWaypointCountMessageToJausMessage(reportWaypointCountMessage);
			reportWaypointCountMessageDestroy(reportWaypointCountMessage);
			ojCmptSendMessage(wd, txMessage);
			queryWaypointCountMessageDestroy(queryWaypointCountMessage);
			break;
		
		default:
			ojCmptDefaultMessageProcessor(wd, message);
			break;
	}
}

void wdInitState(OjCmpt wd)
{
	WdData *data;
	
	data = (WdData*)ojCmptGetUserData(wd);
		
	if(data->pdAddress->node == 0)
	{
		if(ojCmptLookupAddress(wd, data->pdAddress))
		{
			jausAddressCopy(data->setWrench->destination, data->pdAddress);
		}
	}

	wdManagePdControl(wd);

	// Check for critcal service connections or conditions here
	if(	ojCmptIsIncomingScActive(wd, data->gposSc) &&
		ojCmptIsIncomingScActive(wd, data->vssSc) &&
		ojCmptIsIncomingScActive(wd, data->pdWrenchSc) &&
		ojCmptIsIncomingScActive(wd, data->pdStatusSc) &&
		data->inControl)
	{
		// Transition to Standby
		ojCmptSetState(wd, JAUS_STANDBY_STATE);
		//cDebug(4, "wd: Switching to STANDBY State\n");
	}
}

void wdStandbyState(OjCmpt wd)
{
	WdData *data;
	
	data = (WdData*)ojCmptGetUserData(wd);

	if(	!ojCmptIsIncomingScActive(wd, data->gposSc) ||
		!ojCmptIsIncomingScActive(wd, data->vssSc) ||
		!ojCmptIsIncomingScActive(wd, data->pdWrenchSc) ||
		!ojCmptIsIncomingScActive(wd, data->pdStatusSc) ||
		!data->inControl)
	{
		ojCmptSetState(wd, JAUS_INITIALIZE_STATE);
		return;
	}
		
	wdManagePdControl(wd);

	// Setup Vehicle State		
	data->vehicleState->desiredSpeedMps = 0;
	data->vehicleState->desiredPhiEffort = 0;
	wdExcecuteControl(wd, data->vehicleState);
	
	if(data->pdState == JAUS_READY_STATE)
	{
		ojCmptSetState(wd, JAUS_READY_STATE);
	}	
}

void wdReadyState(OjCmpt wd)
{
	SetGlobalWaypointMessage tempGlobalWaypoint;
	double waypointHeading = 0;
	double headingDelta = 0;
	WdData *data;
	
	data = (WdData*)ojCmptGetUserData(wd);

	if(	!ojCmptIsIncomingScActive(wd, data->gposSc) ||
		!ojCmptIsIncomingScActive(wd, data->vssSc) ||
		!ojCmptIsIncomingScActive(wd, data->pdWrenchSc) ||
		!ojCmptIsIncomingScActive(wd, data->pdStatusSc) ||
		!data->inControl)
	{
		ojCmptSetState(wd, JAUS_INITIALIZE_STATE);
		return;
	}
	
	if(data->pdState != JAUS_READY_STATE)
	{
		ojCmptSetState(wd, JAUS_STANDBY_STATE);
		return;
	}	
	
	wdManagePdControl(wd);

	data->vehicleState->speedMps = data->reportVss? (float)data->reportVss->velocityXMps : 0.0f;

	if(data->currentWaypoint < data->waypoints->elementCount)
	{
		tempGlobalWaypoint = (SetGlobalWaypointMessage) data->waypoints->elementData[data->currentWaypoint];

		data->waypointDistance = greatCircleDistance(	data->reportGpos->latitudeDegrees * RAD_PER_DEG,
														data->reportGpos->longitudeDegrees * RAD_PER_DEG,
														tempGlobalWaypoint->latitudeDegrees * RAD_PER_DEG,
														tempGlobalWaypoint->longitudeDegrees * RAD_PER_DEG);
		waypointHeading = greatCircleCourse(data->reportGpos->latitudeDegrees * RAD_PER_DEG,
											data->reportGpos->longitudeDegrees * RAD_PER_DEG,
											tempGlobalWaypoint->latitudeDegrees * RAD_PER_DEG,
											tempGlobalWaypoint->longitudeDegrees * RAD_PER_DEG);
		headingDelta = wdAngleSubtract(waypointHeading, data->reportGpos->yawRadians);
		
		data->vehicleState->desiredPhiEffort = (float)(WHEEL_ROTATION_EFFORT_PER_RAD * headingDelta);
		data->vehicleState->desiredSpeedMps = data->setSpeed? (float)data->setSpeed->speedMps : 0.0f;
		//cLog("HeadingDelta: %7.2f\n", headingDelta);
		//cLog("vehicleState->desiredPhiEffort: %7.2f\n", data->vehicleState->desiredPhiEffort);		
	}
	else
	{
		//hang out
		data->vehicleState->desiredSpeedMps = 0;
		data->vehicleState->desiredPhiEffort = 0;
	}
	
	wdExcecuteControl(wd, data->vehicleState);
}

void wdManagePdControl(OjCmpt wd)
{
	JausAddress address;
	JausMessage message = NULL;
	ReleaseComponentControlMessage releaseControl = NULL;
	RequestComponentControlMessage requestControl = NULL;
	WdData *data;
	
	data = (WdData*)ojCmptGetUserData(wd);
	
	// Check Control
	if(data->requestControl)
	{
		if(!data->inControl && data->pdAddress->node)
		{
			//jausAddressToString(pd->address, buf);
			//cDebug(4, "wd: Requesting control of PD %s\n", buf);
	
			requestControl = requestComponentControlMessageCreate();
			address = ojCmptGetAddress(wd);
			jausAddressCopy(requestControl->source, address);
			jausAddressDestroy(address);
			jausAddressCopy(requestControl->destination, data->pdAddress);
			requestControl->authorityCode = ojCmptGetAuthority(wd);
			
			message = requestComponentControlMessageToJausMessage(requestControl);
			ojCmptSendMessage(wd, message);
			jausMessageDestroy(message);
			
			requestComponentControlMessageDestroy(requestControl);
		}
	}
	else
	{
		if(data->inControl)
		{
			// Release Control
			releaseControl = releaseComponentControlMessageCreate();
			address = ojCmptGetAddress(wd);
			jausAddressCopy(releaseControl->source, address);
			jausAddressDestroy(address);
			jausAddressCopy(releaseControl->destination, data->pdAddress);
			
			message = releaseComponentControlMessageToJausMessage(releaseControl);
			ojCmptSendMessage(wd, message);
			jausMessageDestroy(message);
			
			releaseComponentControlMessageDestroy(releaseControl);
			
			data->inControl = JAUS_FALSE;
		}
	}
}

void wdExcecuteControl(OjCmpt wd, VehicleState vehicleState)
{
	static double speedCommand = 0;
	static double prevSpeedError = 0;
	static double dampedSpeedErrorDerivative = 0;
	static double acceleration = ACCELERATION_MPSPS; //Mpsps
	static double decceleration = DECCELERATION_MPSPS; //Mpsps
	static double lastExcecuteTime = -1; 
	static double dt;
	static double linearEffortInt = 0.0;
	double linearEffort;
	double speedError;
	JausMessage txMessage;
	WdData *data;
	
	data = (WdData*)ojCmptGetUserData(wd);
	
	if(lastExcecuteTime < 0)
	{
		lastExcecuteTime = ojGetTimeSec();
		dt = 1.0 / WD_THREAD_DESIRED_RATE_HZ;
	}
	else
	{
		dt = ojGetTimeSec() - lastExcecuteTime;
		lastExcecuteTime = ojGetTimeSec();
	}
	
	if(speedCommand < vehicleState->desiredSpeedMps)
	{
		speedCommand += acceleration * dt;
		if(speedCommand > vehicleState->desiredSpeedMps)
		{
			speedCommand = vehicleState->desiredSpeedMps;
		}
	}	
	else
	{
		speedCommand -= decceleration * dt;		
		if(speedCommand < vehicleState->desiredSpeedMps)
		{
			speedCommand = vehicleState->desiredSpeedMps;
		}
	}
	
	speedError = speedCommand - vehicleState->speedMps;	

	linearEffortInt += speedError * dt;
	if(linearEffortInt > LINEAR_EFFORT_I_LIM)
	{
		linearEffortInt = LINEAR_EFFORT_I_LIM;
	}
	if(linearEffortInt < -LINEAR_EFFORT_I_LIM)
	{
		linearEffortInt = -LINEAR_EFFORT_I_LIM;
	}

	if(dt > 0.001)
	{
		dampedSpeedErrorDerivative = 0.9 * dampedSpeedErrorDerivative + 0.1 * (speedError - prevSpeedError) / dt;
		prevSpeedError = speedError;
	}

	linearEffort = LINEAR_EFFORT_K_FF * speedCommand + LINEAR_EFFORT_BIAS_FF; 
	linearEffort += LINEAR_EFFORT_K_P * speedError;
	linearEffort += LINEAR_EFFORT_K_I * linearEffortInt;
	linearEffort += LINEAR_EFFORT_K_D * dampedSpeedErrorDerivative;
	
	// Pitch feed forward
	//linearEffort += PITCH_FF_EFFORT_PER_RAD * sin(wdDampedPitchRad); // Pitch feed forward effort
	
	// Steering feed forward
	if(data->reportWrench)
	{
		linearEffort += STEERING_FF_EFFORT * (1.0/cos(data->reportWrench->propulsiveRotationalEffortZPercent * WHEEL_ROTATION_RAD_PER_EFFORT) - 1.0);
	}
	
	// Sticktion feed forward
	if(speedCommand < WD_DEFAULT_MIN_SPEED_MPS)
	{
		linearEffort += STICKTION_EFFORT; 
	}

	if(linearEffort > 0)
	{
		data->setWrench->propulsiveLinearEffortXPercent = THROTTLE_K * linearEffort;
		data->setWrench->resistiveLinearEffortXPercent = 0;
	}
	else
	{
		data->setWrench->propulsiveLinearEffortXPercent = 0;
		data->setWrench->resistiveLinearEffortXPercent = BRAKE_K * linearEffort;
	}

	if(data->setWrench->propulsiveLinearEffortXPercent > 100.0)
	{
		data->setWrench->propulsiveLinearEffortXPercent = 100.0;
	}
	if(data->setWrench->propulsiveLinearEffortXPercent < 0.0)
	{
		data->setWrench->propulsiveLinearEffortXPercent = 0.0;
	}

	if(data->setWrench->resistiveLinearEffortXPercent > 100.0)
	{
		data->setWrench->resistiveLinearEffortXPercent = 100.0;
	}
	if(data->setWrench->resistiveLinearEffortXPercent < 0.0)
	{
		data->setWrench->resistiveLinearEffortXPercent = 0.0;
	}

	data->setWrench->propulsiveRotationalEffortZPercent = vehicleState->desiredPhiEffort;

	if(data->setWrench->propulsiveRotationalEffortZPercent > 100.0)
	{
		data->setWrench->propulsiveRotationalEffortZPercent = 100.0;
	}
	if(data->setWrench->propulsiveRotationalEffortZPercent < -100.0)
	{
		data->setWrench->propulsiveRotationalEffortZPercent = -100.0;
	}

	jausAddressCopy(data->setWrench->destination, data->pdAddress);

	txMessage = setWrenchEffortMessageToJausMessage(data->setWrench);
	ojCmptSendMessage(wd, txMessage);		
	jausMessageDestroy(txMessage);
}

// Function calculates angle a minus angle b
double wdAngleSubtract(double a, double b)
{
   return atan2(sin(a-b), cos(a-b));
} 
