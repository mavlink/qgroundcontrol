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
// File Name: jausMessage.h
//
// Written By: Danny Kent (jaus AT dannykent DOT com), Tom Galluzzo (galluzzo AT gmail DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file defines all the attributes of a JausMessage and all pre-defined command codes according to RA 3.2

#ifndef JAUS_MESSAGE_H
#define JAUS_MESSAGE_H

#include "jaus.h"

#define JAUS_HEADER_SIZE_BYTES		16

#define JAUS_LOW_PRIORITY			0
#define JAUS_DEFAULT_PRIORITY		6
#define JAUS_HIGH_PRIORITY			11

#define JAUS_ACK_NAK_NOT_REQUIRED	0
#define JAUS_ACK_NAK_REQUIRED		1
#define JAUS_NEGATIVE_ACKNOWLEDGE	2
#define JAUS_ACKNOWLEDGE			3

#define JAUS_ACK	JAUS_ACKNOWLEDGE
#define JAUS_NAK	JAUS_NEGATIVE_ACKNOWLEDGE

#define JAUS_SERVICE_CONNECTION_MESSAGE		1
#define	JAUS_NOT_SERVICE_CONNECTION_MESSAGE	0

#define JAUS_EXPERIMENTAL_MESSAGE			1
#define JAUS_NOT_EXPERIMENTAL_MESSAGE		0

#define JAUS_VERSION_2_0	0
#define JAUS_VERSION_2_1	0
#define JAUS_VERSION_3_0	1
#define JAUS_VERSION_3_1	1
#define JAUS_VERSION_3_2	2
#define JAUS_VERSION_3_3	2		// Not a bug, 2 is defined as 3.2 and 3.3

#define JAUS_MAX_DATA_SIZE_BYTES		4080

#define JAUS_SINGLE_DATA_PACKET			0
#define JAUS_FIRST_DATA_PACKET			1
#define JAUS_NORMAL_DATA_PACKET			2
#define JAUS_RETRANSMITTED_DATA_PACKET	4
#define JAUS_LAST_DATA_PACKET			8
 
#define JAUS_MAX_SEQUENCE_NUMBER		65535

// Jaus Command Class Messages
#define JAUS_SET_COMPONENT_AUTHORITY			0x0001
#define JAUS_SHUTDOWN							0x0002
#define JAUS_STANDBY							0x0003
#define JAUS_RESUME								0x0004
#define JAUS_RESET								0x0005
#define JAUS_SET_EMERGENCY						0x0006
#define JAUS_CLEAR_EMERGENCY					0x0007
#define JAUS_CREATE_SERVICE_CONNECTION			0x0008
#define JAUS_CONFIRM_SERVICE_CONNECTION			0x0009
#define JAUS_ACTIVATE_SERVICE_CONNECTION		0x000A
#define JAUS_SUSPEND_SERVICE_CONNECTION			0x000B
#define JAUS_TERMINATE_SERVICE_CONNECTION		0x000C
#define JAUS_REQUEST_COMPONENT_CONTROL			0x000D
#define JAUS_RELEASE_COMPONENT_CONTROL			0x000E
#define JAUS_CONFIRM_COMPONENT_CONTROL			0x000F
#define JAUS_REJECT_COMPONENT_CONTROL			0x0010
#define JAUS_SET_TIME							0x0011
#define JAUS_CREATE_EVENT						0x01F0
#define JAUS_UPDATE_EVENT						0x01F1
#define JAUS_CANCEL_EVENT						0x01F2
#define JAUS_CONFIRM_EVENT_REQUEST				0x01F3
#define JAUS_REJECT_EVENT_REQUEST				0x01F4
#define JAUS_SET_DATA_LINK_STATUS				0x0200
#define JAUS_SET_DATA_LINK_SELECT				0x0201
#define JAUS_SET_SELECTED_DATA_LINK_STATE		0x0202
#define JAUS_SET_WRENCH_EFFORT					0x0405
#define JAUS_SET_DISCRETE_DEVICES				0x0406
#define JAUS_SET_GLOBAL_VECTOR					0x0407
#define JAUS_SET_LOCAL_VECTOR					0x0408
#define JAUS_SET_TRAVEL_SPEED					0x040A
#define JAUS_SET_GLOBAL_WAYPOINT				0x040C
#define JAUS_SET_LOCAL_WAYPOINT					0x040D
#define JAUS_SET_GLOBAL_PATH_SEGMENT			0x040F
#define JAUS_SET_LOCAL_PATH_SEGMENT				0x0410
#define JAUS_SET_JOINT_EFFORTS					0x0601
#define JAUS_SET_JOINT_POSITIONS				0x0602
#define JAUS_SET_JOINT_VELOCITIES				0x0603
#define JAUS_SET_TOOL_POINT						0x0604
#define JAUS_SET_END_EFFECTOR_POSE				0x0605
#define JAUS_SET_END_EFFECTOR_VELOCITY_STATE	0x0606
#define JAUS_SET_JOINT_MOTION					0x0607
#define JAUS_SET_END_EFFECTOR_PATH_MOTION		0x0608
#define JAUS_SET_CAMERA_POSE					0x0801
#define JAUS_SELECT_CAMERA						0x0802
#define JAUS_SET_CAMERA_CAPABILITIES			0x0805
#define JAUS_SET_CAMERA_FORMAT_OPTIONS			0x0806
#define JAUS_CREATE_VKS_OBJECTS					0x0A20
#define JAUS_DELETE_VKS_OBJECTS					0x0A25
#define JAUS_SET_VKS_FEATURE_CLASS_METADATA		0x0A21
#define JAUS_TERMINATE_VKS_DATA_TRANSFER		0x0A24
#define JAUS_SET_PAYLOAD_DATA_ELEMENT 			0x0D01
#define JAUS_SPOOL_MISSION						0x0E00
#define JAUS_RUN_MISSION						0x0E01
#define JAUS_ABORT_MISSION						0x0E02
#define JAUS_PAUSE_MISSION						0x0E03
#define JAUS_RESUME_MISSION						0x0E04
#define JAUS_REMOVE_MESSAGES					0x0E05
#define JAUS_REPLACE_MESSAGES					0x0E06

// Jaus Query Class Messages
#define JAUS_QUERY_COMPONENT_AUTHORITY			0x2001
#define JAUS_QUERY_COMPONENT_STATUS				0x2002
#define JAUS_QUERY_TIME							0x2011
#define JAUS_QUERY_COMPONENT_CONTROL			0x200D
#define JAUS_QUERY_EVENTS						0x21F0
#define JAUS_QUERY_DATA_LINK_STATUS				0x2200
#define JAUS_QUERY_SELECTED_DATA_LINK_STATUS	0x2201
#define JAUS_QUERY_HEARTBEAT_PULSE				0x2202
#define JAUS_QUERY_PLATFORM_SPECIFICATIONS		0x2400
#define JAUS_QUERY_PLATFORM_OPERATIONAL_DATA	0x2401
#define JAUS_QUERY_GLOBAL_POSE					0x2402
#define JAUS_QUERY_LOCAL_POSE					0x2403
#define JAUS_QUERY_VELOCITY_STATE				0x2404
#define JAUS_QUERY_WRENCH_EFFORT				0x2405
#define JAUS_QUERY_DISCRETE_DEVICES				0x2406
#define JAUS_QUERY_GLOBAL_VECTOR				0x2407
#define JAUS_QUERY_LOCAL_VECTOR					0x2408
#define JAUS_QUERY_TRAVEL_SPEED					0x240A
#define JAUS_QUERY_WAYPOINT_COUNT				0x240B
#define JAUS_QUERY_GLOBAL_WAYPOINT				0x240C
#define JAUS_QUERY_LOCAL_WAYPOINT				0x240D
#define JAUS_QUERY_PATH_SEGMENT_COUNT			0x240E
#define JAUS_QUERY_GLOBAL_PATH_SEGMENT			0x240F
#define JAUS_QUERY_LOCAL_PATH_SEGMENT			0x2410
#define JAUS_QUERY_MANIPULATOR_SPECIFICATIONS	0x2600
#define JAUS_QUERY_JOINT_EFFORTS				0x2601
#define JAUS_QUERY_JOINT_POSITIONS				0x2602
#define JAUS_QUERY_JOINT_VELOCITIES				0x2603
#define JAUS_QUERY_TOOL_POINT					0x2604
#define JAUS_QUERY_JOINT_FORCE_TORQUES			0x2605
#define JAUS_QUERY_CAMERA_POSE					0x2800
#define JAUS_QUERY_CAMERA_COUNT					0x2801
#define JAUS_QUERY_RELATIVE_OBJECT_POSITION		0x2802
#define JAUS_QUERY_SELECTED_CAMERA				0x2804
#define JAUS_QUERY_CAMERA_CAPABILITIES			0x2805
#define JAUS_QUERY_CAMERA_FORMAT_OPTIONS		0x2806
#define JAUS_QUERY_IMAGE						0x2807
#define JAUS_QUERY_VKS_FEATURE_CLASS_METADATA	0x2A21
#define JAUS_QUERY_VKS_BOUNDS					0x2A22
#define JAUS_QUERY_VKS_OBJECTS					0x2A23
#define JAUS_QUERY_IDENTIFICATION				0x2B00
#define JAUS_QUERY_CONFIGURATION 				0x2B01
#define JAUS_QUERY_SUBSYSTEM_LIST 				0x2B02
#define JAUS_QUERY_SERVICES 					0x2B03
#define JAUS_QUERY_PAYLOAD_INTERFACE 			0x2D00
#define JAUS_QUERY_PAYLOAD_DATA_ELEMENT 		0x2D01
#define JAUS_QUERY_SPOOLING_PREFERENCE			0x2E00
#define JAUS_QUERY_MISSION_STATUS				0x2E01

// Jaus Inform Class Messages
#define JAUS_REPORT_COMPONENT_AUTHORITY				0x4001
#define JAUS_REPORT_COMPONENT_STATUS				0x4002
#define JAUS_REPORT_TIME							0x4011
#define JAUS_REPORT_COMPONENT_CONTROL				0x400D
#define JAUS_REPORT_EVENTS							0x41F0
#define JAUS_EVENT									0x41F1
#define JAUS_REPORT_DATA_LINK_STATUS				0x4200
#define JAUS_REPORT_SELECTED_DATA_LINK_STATUS		0x4201
#define JAUS_REPORT_HEARTBEAT_PULSE					0x4202
#define JAUS_REPORT_PLATFORM_SPECIFICATIONS			0x4400
#define JAUS_REPORT_PLATFORM_OPERATIONAL_DATA		0x4401
#define JAUS_REPORT_GLOBAL_POSE						0x4402
#define JAUS_REPORT_LOCAL_POSE						0x4403
#define JAUS_REPORT_VELOCITY_STATE					0x4404
#define JAUS_REPORT_WRENCH_EFFORT					0x4405
#define JAUS_REPORT_DISCRETE_DEVICES				0x4406
#define JAUS_REPORT_GLOBAL_VECTOR					0x4407
#define JAUS_REPORT_LOCAL_VECTOR					0x4408
#define JAUS_REPORT_TRAVEL_SPEED					0x440A
#define JAUS_REPORT_WAYPOINT_COUNT					0x440B
#define JAUS_REPORT_GLOBAL_WAYPOINT					0x440C
#define JAUS_REPORT_LOCAL_WAYPOINT					0x440D
#define JAUS_REPORT_PATH_SEGMENT_COUNT				0x440E
#define JAUS_REPORT_GLOBAL_PATH_SEGMENT				0x440F
#define JAUS_REPORT_LOCAL_PATH_SEGMENT				0x4410
#define JAUS_REPORT_MANIPULATOR_SPECIFICATIONS		0x4600
#define JAUS_REPORT_JOINT_EFFORTS					0x4601
#define JAUS_REPORT_JOINT_POSITIONS					0x4602
#define JAUS_REPORT_JOINT_VELOCITIES				0x4603
#define JAUS_REPORT_TOOL_POINT						0x4604
#define JAUS_REPORT_JOINT_FORCE_TORQUES				0x4605
#define JAUS_REPORT_CAMERA_POSE						0x4800
#define JAUS_REPORT_CAMERA_COUNT					0x4801
#define JAUS_REPORT_RELATIVE_OBJECT_POSITION		0x4802
#define JAUS_REPORT_SELECTED_CAMERA					0x4804
#define JAUS_REPORT_CAMERA_CAPABILITIES				0x4805
#define JAUS_REPORT_CAMERA_FORMAT_OPTIONS			0x4806
#define JAUS_REPORT_IMAGE							0x4807
#define JAUS_REPORT_VKS_OBJECTS_CREATION			0x4A20
#define JAUS_REPORT_VKS_FEATURE_CLASS_METADATA		0x4A21
#define JAUS_REPORT_VKS_BOUNDS						0x4A22
#define JAUS_REPORT_VKS_OBJECTS						0x4A23
#define JAUS_REPORT_VKS_DATA_TRANSFER_TERMINATION	0x4A24
#define JAUS_REPORT_IDENTIFICATION 					0x4B00
#define JAUS_REPORT_CONFIGURATION 					0x4B01
#define JAUS_REPORT_SUBSYSTEM_LIST 					0x4B02
#define JAUS_REPORT_SERVICES 						0x4B03
#define JAUS_REPORT_PAYLOAD_INTERFACE 				0x4D00
#define JAUS_REPORT_PAYLOAD_DATA_ELEMENT 			0x4D01
#define JAUS_REPORT_SPOOLING_PREFERENCE				0x4E00
#define JAUS_REPORT_MISSION_STATUS					0x4E01

// Experimental Messages
#define JAUS_SET_VELOCITY_STATE						0x0404

// Define JausMessage data structure
struct JausMessageStruct
{
	// Header Properties
	
	struct
	{
		// Properties by bit fields
		#ifdef JAUS_BIG_ENDIAN
			JausUnsignedShort reserved:2;
			JausUnsignedShort version:6;
			JausUnsignedShort expFlag:1;
			JausUnsignedShort scFlag:1;
			JausUnsignedShort ackNak:2;
			JausUnsignedShort priority:4; 
		#elif JAUS_LITTLE_ENDIAN
			JausUnsignedShort priority:4; 
			JausUnsignedShort ackNak:2;
			JausUnsignedShort scFlag:1; 
			JausUnsignedShort expFlag:1;
			JausUnsignedShort version:6; 
			JausUnsignedShort reserved:2;
		#else
			#error "Please define system endianess (see jaus.h)"
		#endif
	}properties;
	
	JausUnsignedShort commandCode; 

	JausAddress destination;

	JausAddress source;

	JausUnsignedInteger dataSize;
	
	JausUnsignedInteger dataFlag;
	
	JausUnsignedShort sequenceNumber;

	// Message Data Buffer
	JausByte *data;
	
	struct JausMessageStruct *next;
};

typedef struct JausMessageStruct *JausMessage;

JAUS_EXPORT JausMessage jausMessageCreate(void);
JAUS_EXPORT void jausMessageDestroy(JausMessage);

JAUS_EXPORT unsigned int jausMessageSize(JausMessage);

JAUS_EXPORT JausBoolean jausMessageToBuffer(JausMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);
JAUS_EXPORT JausBoolean jausMessageFromBuffer(JausMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);
JAUS_EXPORT char *jausMessageCommandCodeString(JausMessage);
JAUS_EXPORT char *jausCommandCodeString(unsigned short commandCode);
JAUS_EXPORT JausMessage jausMessageClone(JausMessage);
JAUS_EXPORT JausBoolean jausMessageIsRejectableCommand(JausMessage message);
JAUS_EXPORT unsigned short jausMessageGetComplimentaryCommandCode(unsigned short commandCode);


#endif // JAUS_MESSAGE_H
