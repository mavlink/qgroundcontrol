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
// File Name: setCameraCapabilitiesMessage.h
//
// Written By: Danny Kent (jaus AT dannykent DOT com), Tom Galluzzo 
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file defines the attributes of a SetCameraCapabilitiesMessage

#ifndef SET_CAMERA_CAPABILITIES_MESSAGE_H
#define SET_CAMERA_CAPABILITIES_MESSAGE_H

#include "jaus.h"

#ifndef JAUS_SET_CAMERA_CAPABILITIES_PV
#define JAUS_SET_CAMERA_CAPABILITIES_PV
#define JAUS_SET_CAMERA_CAPABILITIES_PV_HORIZONTAL_FOV_BIT			0
#define JAUS_SET_CAMERA_CAPABILITIES_PV_VERTICAL_FOV_BIT			1
#define JAUS_SET_CAMERA_CAPABILITIES_PV_HORIZONTAL_RESOLUTION_BIT	2
#define JAUS_SET_CAMERA_CAPABILITIES_PV_VERTICAL_RESOLUTION_BIT		3
#define JAUS_SET_CAMERA_CAPABILITIES_PV_FOCUS_BIT					4
#define JAUS_SET_CAMERA_CAPABILITIES_PV_IRIS_BIT					5
#define JAUS_SET_CAMERA_CAPABILITIES_PV_GAIN_BIT					6
#define JAUS_SET_CAMERA_CAPABILITIES_PV_FRAME_RATE_BIT				7
#define JAUS_SET_CAMERA_CAPABILITIES_PV_SHUTTER_SPEED_BIT			8
#define JAUS_SET_CAMERA_CAPABILITIES_PV_IMAGE_CONTROL_BIT			9
#define JAUS_SET_CAMERA_CAPABILITIES_PV_AUDIO_CONTROL_BIT			10
#endif

#ifndef JAUS_CAMERA_IMAGE_CONTROL_BF
#define JAUS_CAMERA_IMAGE_CONTROL_BF
#define JAUS_CAMERA_IMAGE_CONTROL_BF_AUTO_FOCUS_BIT				0
#define JAUS_CAMERA_IMAGE_CONTROL_BF_AUTO_IRIS_BIT				1
#define JAUS_CAMERA_IMAGE_CONTROL_BF_IMAGE_STABILIZATION_BIT	2
#define JAUS_CAMERA_IMAGE_CONTROL_BF_WHITE_BALANCE_BIT			3
#define JAUS_CAMERA_IMAGE_CONTROL_BF_SYNC_FLASH_BIT				4
#define JAUS_CAMERA_IMAGE_CONTROL_BF_RED_EYE_BIT				5	
#define JAUS_CAMERA_IMAGE_CONTROL_BF_AUTO_SHUTTER_BIT			6	
#define JAUS_CAMERA_IMAGE_CONTROL_BF_AUTO_GAIN_BIT				7	
#define JAUS_CAMERA_IMAGE_CONTROL_BF_INTERLACED_BIT				8	
#endif

#ifndef JAUS_CAMERA_AUDIO_CONTROL_BF 
#define JAUS_CAMERA_AUDIO_CONTROL_BF
#define JAUS_CAMERA_AUDIO_CONTROL_BF_AUDIO_BIT				0
#define JAUS_CAMERA_AUDIO_CONTROL_BF_AUTO_GAIN_BIT			1
#define JAUS_CAMERA_AUDIO_CONTROL_BF_STEREO_BIT				2
#define JAUS_CAMERA_AUDIO_CONTROL_BF_DIRECTIONAL_BIT		3
#define JAUS_CAMERA_AUDIO_CONTROL_BF_FRONT_MICROPHONE_BIT	4
#define JAUS_CAMERA_AUDIO_CONTROL_BF_REAR_MICROPHONE_BIT	5
#define JAUS_CAMERA_AUDIO_CONTROL_BF_LEFT_MICROPHONE_BIT	6
#define JAUS_CAMERA_AUDIO_CONTROL_BF_RIGHT_MICROPHONE_BIT	7
#endif

typedef struct
{
	// Include all parameters from a JausMessage structure:
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

	// MESSAGE DATA MEMBERS GO HERE
	JausUnsignedShort presenceVector;
	JausByte cameraID;
	JausDouble horizontalFovRadians;		// Scaled UShort (0, JAUS_PI) 
	JausDouble verticalFovRadians;			// Scaled UShort (0, JAUS_PI) 
	JausUnsignedShort horizontalResolution;
	JausUnsignedShort verticalResolution;
	JausDouble focusPercent;				// Scaled Byte (0, 100)
	JausDouble irisPercent;					// Scaled Byte (0, 100)
	JausDouble gainPercent;					// Scaled Byte (0, 100)
	JausUnsignedShort frameRate;
	JausUnsignedShort shutterSpeed;
	
	// Image Control
	JausBoolean autoFocus;
	JausBoolean autoIris;
	JausBoolean imageStabilization;
	JausBoolean whiteBalance;
	JausBoolean syncFlash;
	JausBoolean redEye;
	JausBoolean autoShutter;
	JausBoolean videoAutoGain;
	JausBoolean interlacedVideo;

	// Audio Control
	JausBoolean audioEnabled;
	JausBoolean audioAutoGain;
	JausBoolean stereoAudio;
	JausBoolean directionalAudio;
	JausBoolean frontMicrophone;
	JausBoolean rearMicrophone;
	JausBoolean leftMicrophone;
	JausBoolean rightMicrophone;

}SetCameraCapabilitiesMessageStruct;

typedef SetCameraCapabilitiesMessageStruct* SetCameraCapabilitiesMessage;

JAUS_EXPORT SetCameraCapabilitiesMessage setCameraCapabilitiesMessageCreate(void);
JAUS_EXPORT void setCameraCapabilitiesMessageDestroy(SetCameraCapabilitiesMessage);

JAUS_EXPORT JausBoolean setCameraCapabilitiesMessageFromBuffer(SetCameraCapabilitiesMessage message, unsigned char* buffer, unsigned int bufferSizeBytes);
JAUS_EXPORT JausBoolean setCameraCapabilitiesMessageToBuffer(SetCameraCapabilitiesMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);

JAUS_EXPORT SetCameraCapabilitiesMessage setCameraCapabilitiesMessageFromJausMessage(JausMessage jausMessage);
JAUS_EXPORT JausMessage setCameraCapabilitiesMessageToJausMessage(SetCameraCapabilitiesMessage message);

JAUS_EXPORT unsigned int setCameraCapabilitiesMessageSize(SetCameraCapabilitiesMessage message);

#endif // SET_CAMERA_CAPABILITIES_MESSAGE_H
