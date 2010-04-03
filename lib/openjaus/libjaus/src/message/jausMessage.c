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
// File Name: jausMessage.c
//
// Written By: Danny Kent (jaus AT dannykent DOT com), Tom Galluzzo (galluzzo AT gmail DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file defines all the attributes of a JausMessage and all pre-defined 
// command codes according to RA 3.2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jaus.h"

static const int commandCode = 0;
static const int maxDataSizeBytes = 0;

//Private Functions
static JausBoolean headerToBuffer(JausMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);
static JausBoolean headerFromBuffer(JausMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);

JausMessage jausMessageCreate(void)
{
	JausMessage message;
	
	message = (JausMessage)malloc( sizeof(struct JausMessageStruct) );
	if(message == NULL)
	{
		return NULL;
	}

	//Initialize Values
	message->properties.priority = JAUS_DEFAULT_PRIORITY;
	message->properties.ackNak = JAUS_ACK_NAK_NOT_REQUIRED;
	message->properties.scFlag = JAUS_NOT_SERVICE_CONNECTION_MESSAGE;
	message->properties.expFlag = JAUS_NOT_EXPERIMENTAL_MESSAGE;
	message->properties.version = JAUS_VERSION_3_3;
	message->properties.reserved = 0;
	message->commandCode = commandCode;
	message->destination = jausAddressCreate();
	message->source = jausAddressCreate();
	message->dataFlag = JAUS_SINGLE_DATA_PACKET;
	message->dataSize = maxDataSizeBytes;
	message->sequenceNumber = 0;
	
	message->data = NULL;

	return message;
}

void jausMessageDestroy(JausMessage message)
{
	if(message)
	{
		if(message->data)
		{
			free(message->data);
		}
		jausAddressDestroy(message->source);
		jausAddressDestroy(message->destination);
		free(message);	
	}
}


unsigned int jausMessageSize(JausMessage message)
{
	return (unsigned int)(message->dataSize + JAUS_HEADER_SIZE_BYTES);
}

JausBoolean jausMessageToBuffer(JausMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	if(bufferSizeBytes < jausMessageSize(message))
	{
		return JAUS_FALSE; //improper size	
	}
	else
	{	
		if(headerToBuffer(message, buffer, bufferSizeBytes))
		{
			memcpy(buffer + JAUS_HEADER_SIZE_BYTES, message->data, message->dataSize);
			return JAUS_TRUE;
		}
		else
		{
			return JAUS_FALSE; //headerToJausBuffer failed
		}
	}
}

JausBoolean jausMessageFromBuffer(JausMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	int index = 0;
	
	if(headerFromBuffer(message, buffer + index, bufferSizeBytes - index))
	{
		index += JAUS_HEADER_SIZE_BYTES;

		if(bufferSizeBytes - index < message->dataSize)
		{
			return JAUS_FALSE; // not sufficient size
		}
		else
		{
			message->data = (unsigned char*) malloc(message->dataSize);
			memcpy(message->data, buffer+index, message->dataSize);
			return JAUS_TRUE;
		}
		return JAUS_TRUE;
	}
	else
	{
		return JAUS_FALSE;
	}
}

JausBoolean jausMessageIsRejectableCommand(JausMessage message)
{
	if(!message)
	{
		return JAUS_FALSE;
	}
	
	switch(message->commandCode)
	{
		case JAUS_SET_COMPONENT_AUTHORITY:
		case JAUS_SHUTDOWN:
		case JAUS_STANDBY:
		case JAUS_RESUME:
		case JAUS_RESET:
		case JAUS_SET_EMERGENCY:
		case JAUS_CLEAR_EMERGENCY:
		case JAUS_SET_TIME:		
		case JAUS_RELEASE_COMPONENT_CONTROL:
		case JAUS_SET_WRENCH_EFFORT:
		case JAUS_SET_DISCRETE_DEVICES:
		case JAUS_SET_GLOBAL_VECTOR:
		case JAUS_SET_LOCAL_VECTOR:
		case JAUS_SET_TRAVEL_SPEED:
		case JAUS_SET_GLOBAL_WAYPOINT:
		case JAUS_SET_LOCAL_WAYPOINT:
		case JAUS_SET_GLOBAL_PATH_SEGMENT:
		case JAUS_SET_LOCAL_PATH_SEGMENT:
		case JAUS_SET_JOINT_EFFORTS:
		case JAUS_SET_JOINT_POSITIONS:
		case JAUS_SET_JOINT_VELOCITIES:
		case JAUS_SET_TOOL_POINT:
		case JAUS_SET_END_EFFECTOR_POSE:
		case JAUS_SET_END_EFFECTOR_VELOCITY_STATE:
		case JAUS_SET_JOINT_MOTION:
		case JAUS_SET_END_EFFECTOR_PATH_MOTION:
		case JAUS_SET_CAMERA_POSE:
		case JAUS_SELECT_CAMERA:
		case JAUS_SET_CAMERA_CAPABILITIES:
		case JAUS_SET_CAMERA_FORMAT_OPTIONS:
			return JAUS_TRUE;
			
		default:
			return JAUS_FALSE;
	}
	return JAUS_FALSE;	
}

char *jausMessageCommandCodeString(JausMessage message)
{
	return jausCommandCodeString(message->commandCode);
}

char *jausCommandCodeString(unsigned short commandCode)
{
	static char string[128] = {0};
	
	switch(commandCode)
	{
		case JAUS_SET_COMPONENT_AUTHORITY:
			return "JAUS_SET_COMPONENT_AUTHORITY";
		case JAUS_SHUTDOWN:
			return "JAUS_SHUTDOWN";
		case JAUS_STANDBY:
			return "JAUS_STANDBY";
		case JAUS_RESUME:
			return "JAUS_RESUME";
		case JAUS_RESET:
			return "JAUS_RESET";
		case JAUS_SET_EMERGENCY:
			return "JAUS_SET_EMERGENCY";
		case JAUS_CLEAR_EMERGENCY:
			return "JAUS_CLEAR_EMERGENCY";
		case JAUS_CREATE_SERVICE_CONNECTION:
			return "JAUS_CREATE_SERVICE_CONNECTION";
		case JAUS_CONFIRM_SERVICE_CONNECTION:
			return "JAUS_CONFIRM_SERVICE_CONNECTION";
		case JAUS_ACTIVATE_SERVICE_CONNECTION:
			return "JAUS_ACTIVATE_SERVICE_CONNECTION";
		case JAUS_SUSPEND_SERVICE_CONNECTION:
			return "JAUS_SUSPEND_SERVICE_CONNECTION";
		case JAUS_TERMINATE_SERVICE_CONNECTION:
			return "JAUS_TERMINATE_SERVICE_CONNECTION";
		case JAUS_REQUEST_COMPONENT_CONTROL:
			return "JAUS_REQUEST_COMPONENT_CONTROL";
		case JAUS_RELEASE_COMPONENT_CONTROL:
			return "JAUS_RELEASE_COMPONENT_CONTROL";
		case JAUS_CONFIRM_COMPONENT_CONTROL:
			return "JAUS_CONFIRM_COMPONENT_CONTROL";
		case JAUS_REJECT_COMPONENT_CONTROL:
			return "JAUS_REJECT_COMPONENT_CONTROL";
		case JAUS_SET_TIME:
			return "JAUS_SET_TIME";
		case JAUS_CREATE_EVENT:
			return "JAUS_CREATE_EVENT";
		case JAUS_UPDATE_EVENT:
			return "JAUS_UPDATE_EVENT";
		case JAUS_CANCEL_EVENT:
			return "JAUS_CANCEL_EVENT";
		case JAUS_CONFIRM_EVENT_REQUEST:
			return "JAUS_CONFIRM_EVENT_REQUEST";
		case JAUS_REJECT_EVENT_REQUEST:
			return "JAUS_REJECT_EVENT_REQUEST";
		case JAUS_SET_DATA_LINK_STATUS:
			return "JAUS_SET_DATA_LINK_STATUS";
		case JAUS_SET_DATA_LINK_SELECT:
			return "JAUS_SET_DATA_LINK_SELECT";
		case JAUS_SET_WRENCH_EFFORT:
			return "JAUS_SET_WRENCH_EFFORT";
		case JAUS_SET_DISCRETE_DEVICES:
			return "JAUS_SET_DISCRETE_DEVICES";
		case JAUS_SET_GLOBAL_VECTOR:
			return "JAUS_SET_GLOBAL_VECTOR";
		case JAUS_SET_LOCAL_VECTOR:
			return "JAUS_SET_LOCAL_VECTOR";
		case JAUS_SET_TRAVEL_SPEED:
			return "JAUS_SET_TRAVEL_SPEED";
		case JAUS_SET_GLOBAL_WAYPOINT:
			return "JAUS_SET_GLOBAL_WAYPOINT";
		case JAUS_SET_LOCAL_WAYPOINT:
			return "JAUS_SET_LOCAL_WAYPOINT";
		case JAUS_SET_GLOBAL_PATH_SEGMENT:
			return "JAUS_SET_GLOBAL_PATH_SEGMENT";
		case JAUS_SET_LOCAL_PATH_SEGMENT:
			return "JAUS_SET_LOCAL_PATH_SEGMENT";
		case JAUS_SET_JOINT_EFFORTS:
			return "JAUS_SET_JOINT_EFFORTS";
		case JAUS_SET_JOINT_POSITIONS:
			return "JAUS_SET_JOINT_POSITIONS";
		case JAUS_SET_JOINT_VELOCITIES:
			return "JAUS_SET_JOINT_VELOCITIES";
		case JAUS_SET_TOOL_POINT:
			return "JAUS_SET_TOOL_POINT";
		case JAUS_SET_END_EFFECTOR_POSE:
			return "JAUS_SET_END_EFFECTOR_POSE";
		case JAUS_SET_END_EFFECTOR_VELOCITY_STATE:
			return "JAUS_SET_END_EFFECTOR_VELOCITY_STATE";
		case JAUS_SET_JOINT_MOTION:
			return "JAUS_SET_JOINT_MOTION";
		case JAUS_SET_END_EFFECTOR_PATH_MOTION:
			return "JAUS_SET_END_EFFECTOR_PATH_MOTION";
		case JAUS_SET_CAMERA_POSE:
			return "JAUS_SET_CAMERA_POSE";
		case JAUS_SELECT_CAMERA:
			return "JAUS_SELECT_CAMERA";
		case JAUS_SET_CAMERA_CAPABILITIES:
			return "JAUS_SET_CAMERA_CAPABILITIES";
		case JAUS_SET_CAMERA_FORMAT_OPTIONS:
			return "JAUS_SET_CAMERA_FORMAT_OPTIONS";
		case JAUS_QUERY_COMPONENT_AUTHORITY:
			return "JAUS_QUERY_COMPONENT_AUTHORITY";
		case JAUS_QUERY_COMPONENT_STATUS:
			return "JAUS_QUERY_COMPONENT_STATUS";
		case JAUS_QUERY_TIME:
			return "JAUS_QUERY_TIME";
		case JAUS_QUERY_EVENTS:
			return "JAUS_QUERY_EVENTS";
		case JAUS_QUERY_DATA_LINK_STATUS:
			return "JAUS_QUERY_DATA_LINK_STATUS";
		case JAUS_QUERY_SELECTED_DATA_LINK_STATUS:
			return "JAUS_QUERY_SELECTED_DATA_LINK_STATUS";
		case JAUS_QUERY_HEARTBEAT_PULSE:
			return "JAUS_QUERY_HEARTBEAT_PULSE";
		case JAUS_QUERY_PLATFORM_SPECIFICATIONS:
			return "JAUS_QUERY_PLATFORM_SPECIFICATIONS";
		case JAUS_QUERY_PLATFORM_OPERATIONAL_DATA:
			return "JAUS_QUERY_PLATFORM_OPERATIONAL_DATA";
		case JAUS_QUERY_GLOBAL_POSE:
			return "JAUS_QUERY_GLOBAL_POSE";
		case JAUS_QUERY_LOCAL_POSE:
			return "JAUS_QUERY_LOCAL_POSE";
		case JAUS_QUERY_VELOCITY_STATE:
			return "JAUS_QUERY_VELOCITY_STATE";
		case JAUS_QUERY_WRENCH_EFFORT:
			return "JAUS_QUERY_WRENCH_EFFORT";
		case JAUS_QUERY_DISCRETE_DEVICES:
			return "JAUS_QUERY_DISCRETE_DEVICES";
		case JAUS_QUERY_GLOBAL_VECTOR:
			return "JAUS_QUERY_GLOBAL_VECTOR";
		case JAUS_QUERY_LOCAL_VECTOR:
			return "JAUS_QUERY_LOCAL_VECTOR";
		case JAUS_QUERY_TRAVEL_SPEED:
			return "JAUS_QUERY_TRAVEL_SPEED";
		case JAUS_QUERY_WAYPOINT_COUNT:
			return "JAUS_QUERY_WAYPOINT_COUNT";
		case JAUS_QUERY_GLOBAL_WAYPOINT:
			return "JAUS_QUERY_GLOBAL_WAYPOINT";
		case JAUS_QUERY_LOCAL_WAYPOINT:
			return "JAUS_QUERY_LOCAL_WAYPOINT";
		case JAUS_QUERY_PATH_SEGMENT_COUNT:
			return "JAUS_QUERY_PATH_SEGMENT_COUNT";
		case JAUS_QUERY_GLOBAL_PATH_SEGMENT:
			return "JAUS_QUERY_GLOBAL_PATH_SEGMENT";
		case JAUS_QUERY_LOCAL_PATH_SEGMENT:
			return "JAUS_QUERY_LOCAL_PATH_SEGMENT";
		case JAUS_QUERY_MANIPULATOR_SPECIFICATIONS:
			return "JAUS_QUERY_MANIPULATOR_SPECIFICATIONS";
		case JAUS_QUERY_JOINT_EFFORTS:
			return "JAUS_QUERY_JOINT_EFFORTS";
		case JAUS_QUERY_JOINT_POSITIONS:
			return "JAUS_QUERY_JOINT_POSITIONS";
		case JAUS_QUERY_JOINT_VELOCITIES:
			return "JAUS_QUERY_JOINT_VELOCITIES";
		case JAUS_QUERY_TOOL_POINT:
			return "JAUS_QUERY_TOOL_POINT";
		case JAUS_QUERY_JOINT_FORCE_TORQUES:
			return "JAUS_QUERY_JOINT_FORCE_TORQUES";
		case JAUS_QUERY_CAMERA_POSE:
			return "JAUS_QUERY_CAMERA_POSE";
		case JAUS_QUERY_CAMERA_COUNT:
			return "JAUS_QUERY_CAMERA_COUNT";
		case JAUS_QUERY_RELATIVE_OBJECT_POSITION:
			return "JAUS_QUERY_RELATIVE_OBJECT_POSITION";
		case JAUS_QUERY_SELECTED_CAMERA:
			return "JAUS_QUERY_SELECTED_CAMERA";
		case JAUS_QUERY_CAMERA_CAPABILITIES:
			return "JAUS_QUERY_CAMERA_CAPABILITIES";
		case JAUS_QUERY_CAMERA_FORMAT_OPTIONS:
			return "JAUS_QUERY_CAMERA_FORMAT_OPTIONS";
		case JAUS_QUERY_IMAGE:
			return "JAUS_QUERY_IMAGE";
		case JAUS_QUERY_COMPONENT_CONTROL:
			return "JAUS_QUERY_COMPONENT_CONTROL";
		case JAUS_REPORT_COMPONENT_AUTHORITY:
			return "JAUS_REPORT_COMPONENT_AUTHORITY";
		case JAUS_REPORT_COMPONENT_STATUS:
			return "JAUS_REPORT_COMPONENT_STATUS";
		case JAUS_REPORT_TIME:
			return "JAUS_REPORT_TIME";
		case JAUS_REPORT_EVENTS:
			return "JAUS_REPORT_EVENTS";
		case JAUS_EVENT:
			return "JAUS_EVENT";
		case JAUS_REPORT_DATA_LINK_STATUS:
			return "JAUS_REPORT_DATA_LINK_STATUS";
		case JAUS_REPORT_SELECTED_DATA_LINK_STATUS:
			return "JAUS_REPORT_SELECTED_DATA_LINK_STATUS";
		case JAUS_REPORT_HEARTBEAT_PULSE:
			return "JAUS_REPORT_HEARTBEAT_PULSE";
		case JAUS_REPORT_PLATFORM_SPECIFICATIONS:
			return "JAUS_REPORT_PLATFORM_SPECIFICATIONS";
		case JAUS_REPORT_PLATFORM_OPERATIONAL_DATA:
			return "JAUS_REPORT_PLATFORM_OPERATIONAL_DATA";
		case JAUS_REPORT_GLOBAL_POSE:
			return "JAUS_REPORT_GLOBAL_POSE";
		case JAUS_REPORT_LOCAL_POSE:
			return "JAUS_REPORT_LOCAL_POSE";
		case JAUS_REPORT_VELOCITY_STATE:
			return "JAUS_REPORT_VELOCITY_STATE";
		case JAUS_REPORT_WRENCH_EFFORT:
			return "JAUS_REPORT_WRENCH_EFFORT";
		case JAUS_REPORT_DISCRETE_DEVICES:
			return "JAUS_REPORT_DISCRETE_DEVICES";
		case JAUS_REPORT_GLOBAL_VECTOR:
			return "JAUS_REPORT_GLOBAL_VECTOR";
		case JAUS_REPORT_LOCAL_VECTOR:
			return "JAUS_REPORT_LOCAL_VECTOR";
		case JAUS_REPORT_TRAVEL_SPEED:
			return "JAUS_REPORT_TRAVEL_SPEED";
		case JAUS_REPORT_WAYPOINT_COUNT:
			return "JAUS_REPORT_WAYPOINT_COUNT";
		case JAUS_REPORT_GLOBAL_WAYPOINT:
			return "JAUS_REPORT_GLOBAL_WAYPOINT";
		case JAUS_REPORT_LOCAL_WAYPOINT:
			return "JAUS_REPORT_LOCAL_WAYPOINT";
		case JAUS_REPORT_PATH_SEGMENT_COUNT:
			return "JAUS_REPORT_PATH_SEGMENT_COUNT";
		case JAUS_REPORT_GLOBAL_PATH_SEGMENT:
			return "JAUS_REPORT_GLOBAL_PATH_SEGMENT";
		case JAUS_REPORT_LOCAL_PATH_SEGMENT:
			return "JAUS_REPORT_LOCAL_PATH_SEGMENT";
		case JAUS_REPORT_MANIPULATOR_SPECIFICATIONS:
			return "JAUS_REPORT_MANIPULATOR_SPECIFICATIONS";
		case JAUS_REPORT_JOINT_EFFORTS:
			return "JAUS_REPORT_JOINT_EFFORTS";
		case JAUS_REPORT_JOINT_POSITIONS:
			return "JAUS_REPORT_JOINT_POSITIONS";
		case JAUS_REPORT_JOINT_VELOCITIES:
			return "JAUS_REPORT_JOINT_VELOCITIES";
		case JAUS_REPORT_TOOL_POINT:
			return "JAUS_REPORT_TOOL_POINT";
		case JAUS_REPORT_JOINT_FORCE_TORQUES:
			return "JAUS_REPORT_JOINT_FORCE_TORQUES";
		case JAUS_REPORT_CAMERA_POSE:
			return "JAUS_REPORT_CAMERA_POSE";
		case JAUS_REPORT_CAMERA_COUNT:
			return "JAUS_REPORT_CAMERA_COUNT";
		case JAUS_REPORT_RELATIVE_OBJECT_POSITION:
			return "JAUS_REPORT_RELATIVE_OBJECT_POSITION";
		case JAUS_REPORT_SELECTED_CAMERA:
			return "JAUS_REPORT_SELECTED_CAMERA";
		case JAUS_REPORT_CAMERA_CAPABILITIES:
			return "JAUS_REPORT_CAMERA_CAPABILITIES";
		case JAUS_REPORT_CAMERA_FORMAT_OPTIONS:
			return "JAUS_REPORT_CAMERA_FORMAT_OPTIONS";
		case JAUS_REPORT_IMAGE:
			return "JAUS_REPORT_IMAGE";
		case JAUS_QUERY_IDENTIFICATION:
			return "JAUS_QUERY_IDENTIFICATION";
		case JAUS_REPORT_IDENTIFICATION:
			return "JAUS_REPORT_IDENTIFICATION";
		case JAUS_QUERY_CONFIGURATION:
			return "JAUS_QUERY_CONFIGURATION";
		case JAUS_REPORT_CONFIGURATION:
			return "JAUS_REPORT_CONFIGURATION";
		case JAUS_QUERY_PAYLOAD_DATA_ELEMENT:
			return "JAUS_QUERY_PAYLOAD_DATA_ELEMENT";
		case JAUS_QUERY_PAYLOAD_INTERFACE:
			return "JAUS_QUERY_PAYLOAD_INTERFACE";
		case JAUS_QUERY_SERVICES:
			return "JAUS_QUERY_SERVICES";
		case JAUS_REPORT_PAYLOAD_DATA_ELEMENT:
			return "JAUS_REPORT_PAYLOAD_DATA_ELEMENT";
		case JAUS_REPORT_PAYLOAD_INTERFACE:
			return "JAUS_REPORT_PAYLOAD_INTERFACE";
		case JAUS_REPORT_SERVICES:
			return "JAUS_REPORT_SERVICES";
		case JAUS_REPORT_COMPONENT_CONTROL:
			return "JAUS_REPORT_COMPONENT_CONTROL";
		case JAUS_SET_PAYLOAD_DATA_ELEMENT:
			return "JAUS_SET_PAYLOAD_DATA_ELEMENT";
		case JAUS_CREATE_VKS_OBJECTS:
			return "JAUS_CREATE_VKS_OBJECTS";
		case JAUS_DELETE_VKS_OBJECTS:
			return "JAUS_DELETE_VKS_OBJECTS";
		case JAUS_QUERY_VKS_BOUNDS:
			return "JAUS_QUERY_VKS_BOUNDS";
		case JAUS_QUERY_VKS_FEATURE_CLASS_METADATA:
			return "JAUS_QUERY_VKS_FEATURE_CLASS_METADATA";
		case JAUS_QUERY_VKS_OBJECTS:
			return "JAUS_QUERY_VKS_OBJECTS";
		case JAUS_REPORT_VKS_BOUNDS:
			return "JAUS_REPORT_VKS_BOUNDS";
		case JAUS_REPORT_VKS_FEATURE_CLASS_METADATA:
			return "JAUS_REPORT_VKS_FEATURE_CLASS_METADATA";
		case JAUS_REPORT_VKS_OBJECTS_CREATION:
			return "JAUS_REPORT_VKS_OBJECTS_CREATION";
		case JAUS_REPORT_VKS_OBJECTS:
			return "JAUS_REPORT_VKS_OBJECTS";
		default:
			sprintf(string, "UNDEFINED MESSAGE: 0x%04X", commandCode); 
			return string;
	}	
}

unsigned short jausMessageGetComplimentaryCommandCode(unsigned short commandCode)
{
	switch(commandCode)
	{
		case JAUS_QUERY_COMPONENT_AUTHORITY:
			return JAUS_REPORT_COMPONENT_AUTHORITY;
		case JAUS_QUERY_COMPONENT_STATUS:
			return JAUS_REPORT_COMPONENT_STATUS;
		case JAUS_QUERY_TIME:
			return JAUS_REPORT_TIME;
		case JAUS_QUERY_EVENTS:
			return JAUS_REPORT_EVENTS;
		case JAUS_QUERY_DATA_LINK_STATUS:
			return JAUS_REPORT_DATA_LINK_STATUS;
		case JAUS_QUERY_SELECTED_DATA_LINK_STATUS:
			return JAUS_REPORT_SELECTED_DATA_LINK_STATUS;
		case JAUS_QUERY_HEARTBEAT_PULSE:
			return JAUS_REPORT_HEARTBEAT_PULSE;
		case JAUS_QUERY_PLATFORM_SPECIFICATIONS:
			return JAUS_REPORT_PLATFORM_SPECIFICATIONS;
		case JAUS_QUERY_PLATFORM_OPERATIONAL_DATA:
			return JAUS_REPORT_PLATFORM_OPERATIONAL_DATA;
		case JAUS_QUERY_GLOBAL_POSE:
			return JAUS_REPORT_GLOBAL_POSE;
		case JAUS_QUERY_LOCAL_POSE:
			return JAUS_REPORT_LOCAL_POSE;
		case JAUS_QUERY_VELOCITY_STATE:
			return JAUS_REPORT_VELOCITY_STATE;
		case JAUS_QUERY_WRENCH_EFFORT:
			return JAUS_REPORT_WRENCH_EFFORT;
		case JAUS_QUERY_DISCRETE_DEVICES:
			return JAUS_REPORT_DISCRETE_DEVICES;
		case JAUS_QUERY_GLOBAL_VECTOR:
			return JAUS_REPORT_GLOBAL_VECTOR;
		case JAUS_QUERY_LOCAL_VECTOR:
			return JAUS_REPORT_LOCAL_VECTOR;
		case JAUS_QUERY_TRAVEL_SPEED:
			return JAUS_REPORT_TRAVEL_SPEED;
		case JAUS_QUERY_WAYPOINT_COUNT:
			return JAUS_REPORT_WAYPOINT_COUNT;
		case JAUS_QUERY_GLOBAL_WAYPOINT:
			return JAUS_REPORT_GLOBAL_WAYPOINT;
		case JAUS_QUERY_LOCAL_WAYPOINT:
			return JAUS_REPORT_LOCAL_WAYPOINT;
		case JAUS_QUERY_PATH_SEGMENT_COUNT:
			return JAUS_REPORT_PATH_SEGMENT_COUNT;
		case JAUS_QUERY_GLOBAL_PATH_SEGMENT:
			return JAUS_REPORT_GLOBAL_PATH_SEGMENT;
		case JAUS_QUERY_LOCAL_PATH_SEGMENT:
			return JAUS_REPORT_LOCAL_PATH_SEGMENT;
		case JAUS_QUERY_MANIPULATOR_SPECIFICATIONS:
			return JAUS_REPORT_MANIPULATOR_SPECIFICATIONS;
		case JAUS_QUERY_JOINT_EFFORTS:
			return JAUS_REPORT_JOINT_EFFORTS;
		case JAUS_QUERY_JOINT_POSITIONS:
			return JAUS_REPORT_JOINT_POSITIONS;
		case JAUS_QUERY_JOINT_VELOCITIES:
			return JAUS_REPORT_JOINT_VELOCITIES;
		case JAUS_QUERY_TOOL_POINT:
			return JAUS_REPORT_TOOL_POINT;
		case JAUS_QUERY_JOINT_FORCE_TORQUES:
			return JAUS_REPORT_JOINT_FORCE_TORQUES;
		case JAUS_QUERY_CAMERA_POSE:
			return JAUS_REPORT_CAMERA_POSE;
		case JAUS_QUERY_CAMERA_COUNT:
			return JAUS_REPORT_CAMERA_COUNT;
		case JAUS_QUERY_RELATIVE_OBJECT_POSITION:
			return JAUS_REPORT_RELATIVE_OBJECT_POSITION;
		case JAUS_QUERY_SELECTED_CAMERA:
			return JAUS_REPORT_SELECTED_CAMERA;
		case JAUS_QUERY_CAMERA_CAPABILITIES:
			return JAUS_REPORT_CAMERA_CAPABILITIES;
		case JAUS_QUERY_CAMERA_FORMAT_OPTIONS:
			return JAUS_REPORT_CAMERA_FORMAT_OPTIONS;
		case JAUS_QUERY_IMAGE:
			return JAUS_REPORT_IMAGE;
		case JAUS_QUERY_PAYLOAD_DATA_ELEMENT:
			return JAUS_REPORT_PAYLOAD_DATA_ELEMENT;
		case JAUS_QUERY_PAYLOAD_INTERFACE:
			return JAUS_REPORT_PAYLOAD_INTERFACE;
		case JAUS_QUERY_SERVICES:
			return JAUS_REPORT_SERVICES;
		case JAUS_QUERY_IDENTIFICATION:
			return JAUS_REPORT_IDENTIFICATION;
		case JAUS_QUERY_CONFIGURATION:
			return JAUS_REPORT_CONFIGURATION;
		case JAUS_QUERY_VKS_BOUNDS:
			return JAUS_REPORT_VKS_BOUNDS;
		case JAUS_QUERY_VKS_FEATURE_CLASS_METADATA:
			return JAUS_REPORT_VKS_FEATURE_CLASS_METADATA;
		case JAUS_QUERY_VKS_OBJECTS:
			return JAUS_REPORT_VKS_OBJECTS;
		case JAUS_REPORT_COMPONENT_AUTHORITY:
			return JAUS_QUERY_COMPONENT_AUTHORITY;
		case JAUS_REPORT_COMPONENT_STATUS:
			return JAUS_QUERY_COMPONENT_STATUS;
		case JAUS_REPORT_TIME:
			return JAUS_QUERY_TIME;
		case JAUS_REPORT_EVENTS:
			return JAUS_QUERY_EVENTS;
		case JAUS_REPORT_DATA_LINK_STATUS:
			return JAUS_QUERY_DATA_LINK_STATUS;
		case JAUS_REPORT_SELECTED_DATA_LINK_STATUS:
			return JAUS_QUERY_SELECTED_DATA_LINK_STATUS;
		case JAUS_REPORT_HEARTBEAT_PULSE:
			return JAUS_QUERY_HEARTBEAT_PULSE;
		case JAUS_REPORT_PLATFORM_SPECIFICATIONS:
			return JAUS_QUERY_PLATFORM_SPECIFICATIONS;
		case JAUS_REPORT_PLATFORM_OPERATIONAL_DATA:
			return JAUS_QUERY_PLATFORM_OPERATIONAL_DATA;
		case JAUS_REPORT_GLOBAL_POSE:
			return JAUS_QUERY_GLOBAL_POSE;
		case JAUS_REPORT_LOCAL_POSE:
			return JAUS_QUERY_LOCAL_POSE;
		case JAUS_REPORT_VELOCITY_STATE:
			return JAUS_QUERY_VELOCITY_STATE;
		case JAUS_REPORT_WRENCH_EFFORT:
			return JAUS_QUERY_WRENCH_EFFORT;
		case JAUS_REPORT_DISCRETE_DEVICES:
			return JAUS_QUERY_DISCRETE_DEVICES;
		case JAUS_REPORT_GLOBAL_VECTOR:
			return JAUS_QUERY_GLOBAL_VECTOR;
		case JAUS_REPORT_LOCAL_VECTOR:
			return JAUS_QUERY_LOCAL_VECTOR;
		case JAUS_REPORT_TRAVEL_SPEED:
			return JAUS_QUERY_TRAVEL_SPEED;
		case JAUS_REPORT_WAYPOINT_COUNT:
			return JAUS_QUERY_WAYPOINT_COUNT;
		case JAUS_REPORT_GLOBAL_WAYPOINT:
			return JAUS_QUERY_GLOBAL_WAYPOINT;
		case JAUS_REPORT_LOCAL_WAYPOINT:
			return JAUS_QUERY_LOCAL_WAYPOINT;
		case JAUS_REPORT_PATH_SEGMENT_COUNT:
			return JAUS_QUERY_PATH_SEGMENT_COUNT;
		case JAUS_REPORT_GLOBAL_PATH_SEGMENT:
			return JAUS_QUERY_GLOBAL_PATH_SEGMENT;
		case JAUS_REPORT_LOCAL_PATH_SEGMENT:
			return JAUS_QUERY_LOCAL_PATH_SEGMENT;
		case JAUS_REPORT_MANIPULATOR_SPECIFICATIONS:
			return JAUS_QUERY_MANIPULATOR_SPECIFICATIONS;
		case JAUS_REPORT_JOINT_EFFORTS:
			return JAUS_QUERY_JOINT_EFFORTS;
		case JAUS_REPORT_JOINT_POSITIONS:
			return JAUS_QUERY_JOINT_POSITIONS;
		case JAUS_REPORT_JOINT_VELOCITIES:
			return JAUS_QUERY_JOINT_VELOCITIES;
		case JAUS_REPORT_TOOL_POINT:
			return JAUS_QUERY_TOOL_POINT;
		case JAUS_REPORT_JOINT_FORCE_TORQUES:
			return JAUS_QUERY_JOINT_FORCE_TORQUES;
		case JAUS_REPORT_CAMERA_POSE:
			return JAUS_QUERY_CAMERA_POSE;
		case JAUS_REPORT_CAMERA_COUNT:
			return JAUS_QUERY_CAMERA_COUNT;
		case JAUS_REPORT_RELATIVE_OBJECT_POSITION:
			return JAUS_QUERY_RELATIVE_OBJECT_POSITION;
		case JAUS_REPORT_SELECTED_CAMERA:
			return JAUS_QUERY_SELECTED_CAMERA;
		case JAUS_REPORT_CAMERA_CAPABILITIES:
			return JAUS_QUERY_CAMERA_CAPABILITIES;
		case JAUS_REPORT_CAMERA_FORMAT_OPTIONS:
			return JAUS_QUERY_CAMERA_FORMAT_OPTIONS;
		case JAUS_REPORT_IMAGE:
			return JAUS_QUERY_IMAGE;
		case JAUS_REPORT_IDENTIFICATION:
			return JAUS_QUERY_IDENTIFICATION;
		case JAUS_REPORT_CONFIGURATION:
			return JAUS_QUERY_CONFIGURATION;
		case JAUS_REPORT_PAYLOAD_DATA_ELEMENT:
			return JAUS_QUERY_PAYLOAD_DATA_ELEMENT;
		case JAUS_REPORT_PAYLOAD_INTERFACE:
			return JAUS_QUERY_PAYLOAD_INTERFACE;
		case JAUS_REPORT_SERVICES:
			return JAUS_QUERY_SERVICES;
		case JAUS_REPORT_VKS_BOUNDS:
			return JAUS_QUERY_VKS_BOUNDS;
		case JAUS_REPORT_VKS_FEATURE_CLASS_METADATA:
			return JAUS_QUERY_VKS_FEATURE_CLASS_METADATA;
		case JAUS_REPORT_VKS_OBJECTS:
			return JAUS_QUERY_VKS_OBJECTS;
		default:
			// Unknown Command Code
			return 0;
	}	
}

JausMessage jausMessageClone(JausMessage inputMessage)
{
	JausMessage outputMessage;

	outputMessage = (JausMessage)malloc( sizeof(struct JausMessageStruct) );
	if(outputMessage == NULL)
	{
		return NULL;
	}
	
	outputMessage->properties = inputMessage->properties;
	outputMessage->commandCode = inputMessage->commandCode;

	outputMessage->destination = jausAddressCreate();
	*outputMessage->destination = *inputMessage->destination;

	outputMessage->source = jausAddressCreate();
	*outputMessage->source = *inputMessage->source;

	outputMessage->dataSize = inputMessage->dataSize;
	outputMessage->dataFlag = inputMessage->dataFlag;
	outputMessage->sequenceNumber = inputMessage->sequenceNumber;

	outputMessage->data = NULL;
	if(inputMessage->dataSize)
	{
		outputMessage->data =  (unsigned char *)malloc(inputMessage->dataSize);
		memcpy(outputMessage->data, inputMessage->data, inputMessage->dataSize);
	}
		
	return outputMessage;
}


//********************* PRIVATE HEADER FUNCTIONS **********************//

static JausBoolean headerToBuffer(JausMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	JausUnsignedShort *propertiesPtr = (JausUnsignedShort*)&message->properties;
	
	if(bufferSizeBytes < JAUS_HEADER_SIZE_BYTES)
	{
		return JAUS_FALSE;
	}
	else
	{	
		buffer[0] = (unsigned char)(*propertiesPtr & 0xFF);
		buffer[1] = (unsigned char)((*propertiesPtr & 0xFF00) >> 8);

		buffer[2] = (unsigned char)(message->commandCode & 0xFF);
		buffer[3] = (unsigned char)((message->commandCode & 0xFF00) >> 8);

		buffer[4] = (unsigned char)(message->destination->instance & 0xFF);
		buffer[5] = (unsigned char)(message->destination->component & 0xFF);
		buffer[6] = (unsigned char)(message->destination->node & 0xFF);
		buffer[7] = (unsigned char)(message->destination->subsystem & 0xFF);

		buffer[8] = (unsigned char)(message->source->instance & 0xFF);
		buffer[9] = (unsigned char)(message->source->component & 0xFF);
		buffer[10] = (unsigned char)(message->source->node & 0xFF);
		buffer[11] = (unsigned char)(message->source->subsystem & 0xFF);
		
		buffer[12] = (unsigned char)(message->dataSize & 0xFF);
		buffer[13] = (unsigned char)((message->dataFlag & 0xFF) << 4) | (unsigned char)((message->dataSize & 0x0F00) >> 8);

		buffer[14] = (unsigned char)(message->sequenceNumber & 0xFF);
		buffer[15] = (unsigned char)((message->sequenceNumber & 0xFF00) >> 8);
		
		return JAUS_TRUE;
	}
}

static JausBoolean headerFromBuffer(JausMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	if(bufferSizeBytes < JAUS_HEADER_SIZE_BYTES)
	{
		return JAUS_FALSE;
	}
	else
	{
		// unpack header
		message->properties.priority = (buffer[0] & 0x0F);
		message->properties.ackNak	 = ((buffer[0] >> 4) & 0x03);
		message->properties.scFlag	 = ((buffer[0] >> 6) & 0x01);
		message->properties.expFlag	 = ((buffer[0] >> 7) & 0x01);
		message->properties.version	 = (buffer[1] & 0x3F);
		message->properties.reserved = ((buffer[1] >> 6) & 0x03);
		
		message->commandCode = buffer[2] + (buffer[3] << 8);

		message->destination->instance = buffer[4];
		message->destination->component = buffer[5];
		message->destination->node = buffer[6];
		message->destination->subsystem = buffer[7];

		message->source->instance = buffer[8];
		message->source->component = buffer[9];
		message->source->node = buffer[10];
		message->source->subsystem = buffer[11];
		
		message->dataSize = buffer[12] | ((buffer[13] & 0x0F) << 8);
		message->dataFlag = (buffer[13] & 0xF0) >> 4;

		message->sequenceNumber = buffer[14] + (buffer[15] << 8);
		
		return JAUS_TRUE;
	}
}
