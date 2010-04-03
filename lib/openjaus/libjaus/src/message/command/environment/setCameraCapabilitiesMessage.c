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
// File Name: setCameraCapabilitiesMessage.c
//
// Written By: Danny Kent (jaus AT dannykent DOT com), Tom Galluzzo (galluzzo AT gmail DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file defines the functionality of a SetCameraCapabilitiesMessage

#include <stdlib.h>
#include <string.h>
#include "jaus.h"

static const int commandCode = JAUS_SET_CAMERA_CAPABILITIES;
static const int maxDataSizeBytes = 22;

static JausBoolean headerFromBuffer(SetCameraCapabilitiesMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);
static JausBoolean headerToBuffer(SetCameraCapabilitiesMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);

static JausBoolean dataFromBuffer(SetCameraCapabilitiesMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);
static int dataToBuffer(SetCameraCapabilitiesMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);
static void dataInitialize(SetCameraCapabilitiesMessage message);
static void dataDestroy(SetCameraCapabilitiesMessage message);
static unsigned int dataSize(SetCameraCapabilitiesMessage message);

// ************************************************************************************************************** //
//                                    USER CONFIGURED FUNCTIONS
// ************************************************************************************************************** //

// Initializes the message-specific fields
static void dataInitialize(SetCameraCapabilitiesMessage message)
{
	// Set initial values of message fields
	message->presenceVector = newJausUnsignedShort(JAUS_SHORT_PRESENCE_VECTOR_ALL_ON);
	message->cameraID = newJausByte(0);
	message->horizontalFovRadians = newJausDouble(0);	// Scaled UShort (0, JAUS_PI) 
	message->verticalFovRadians = newJausDouble(0);		// Scaled UShort (0, JAUS_PI) 
	message->horizontalResolution = newJausUnsignedShort(0);
	message->verticalResolution = newJausUnsignedShort(0);
	message->focusPercent = newJausDouble(0);			// Scaled Byte (0, 100)
	message->irisPercent = newJausDouble(0);			// Scaled Byte (0, 100)
	message->gainPercent = newJausDouble(0);			// Scaled Byte (0, 100)
	message->frameRate = newJausUnsignedShort(0);
	message->shutterSpeed = newJausUnsignedShort(0);

	// Image Control
	message->autoFocus = JAUS_FALSE;
	message->autoIris = JAUS_FALSE;
	message->imageStabilization = JAUS_FALSE;
	message->whiteBalance = JAUS_FALSE;
	message->syncFlash = JAUS_FALSE;
	message->redEye = JAUS_FALSE;
	message->autoShutter = JAUS_FALSE;
	message->videoAutoGain = JAUS_FALSE;
	message->interlacedVideo = JAUS_FALSE;

	// Audio Control
	message->audioEnabled = JAUS_FALSE;
	message->audioAutoGain = JAUS_FALSE;
	message->stereoAudio = JAUS_FALSE;
	message->directionalAudio = JAUS_FALSE;
	message->frontMicrophone = JAUS_FALSE;
	message->rearMicrophone = JAUS_FALSE;
	message->leftMicrophone = JAUS_FALSE;
	message->rightMicrophone = JAUS_FALSE;
}

// Destructs the message-specific fields
static void dataDestroy(SetCameraCapabilitiesMessage message)
{
	// Free message fields
}

// Return boolean of success
static JausBoolean dataFromBuffer(SetCameraCapabilitiesMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	int index = 0;
	JausUnsignedShort tempUShort = 0;
	JausByte tempByte = 0;
	JausUnsignedShort imageControl = 0;
	JausUnsignedShort audioControl = 0;

	if(bufferSizeBytes == message->dataSize)
	{
		// Unpack Message Fields from Buffer
		
		// Presence Vector
		if(!jausUnsignedShortFromBuffer(&message->presenceVector, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
	
		// CameraID
		if(!jausByteFromBuffer(&message->cameraID, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_BYTE_SIZE_BYTES;

		// Horizontal FOV
		if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_HORIZONTAL_FOV_BIT))
		{
			//unpack
			if(!jausUnsignedShortFromBuffer(&tempUShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;

			// Scaled UShort (0, JAUS_PI)
			message->horizontalFovRadians = jausUnsignedShortToDouble(tempUShort, 0, JAUS_PI);
		}
	
		// Vertical FOV
		if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_VERTICAL_FOV_BIT))
		{
			//unpack
			if(!jausUnsignedShortFromBuffer(&tempUShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;

			// Scaled UShort (0, JAUS_PI)
			message->verticalFovRadians = jausUnsignedShortToDouble(tempUShort, 0, JAUS_PI);
		}

		// Horizontal Resolution
		if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_HORIZONTAL_RESOLUTION_BIT))
		{
			//unpack
			if(!jausUnsignedShortFromBuffer(&message->horizontalResolution, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
		}

		// Vertical Resolution
		if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_VERTICAL_RESOLUTION_BIT))
		{
			//unpack
			if(!jausUnsignedShortFromBuffer(&message->verticalResolution, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
		}

		// Focus 
		if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_FOCUS_BIT))
		{
			//unpack
			if(!jausByteFromBuffer(&tempByte, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_BYTE_SIZE_BYTES;

			// Scaled Byte (0, 100)
			message->focusPercent = jausByteToDouble(tempByte, 0, 100);
		}
		
		// Iris
		if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_IRIS_BIT))
		{
			//unpack
			if(!jausByteFromBuffer(&tempByte, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_BYTE_SIZE_BYTES;

			// Scaled Byte (0, 100)
			message->irisPercent = jausByteToDouble(tempByte, 0, 100);
		}

		// Gain
		if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_GAIN_BIT))
		{
			//unpack
			if(!jausByteFromBuffer(&tempByte, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_BYTE_SIZE_BYTES;

			// Scaled Byte (0, 100)
			message->gainPercent = jausByteToDouble(tempByte, 0, 100);
		}

		// Frame Rate
		if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_FRAME_RATE_BIT))
		{
			//unpack
			if(!jausUnsignedShortFromBuffer(&message->frameRate, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
		}

		// Shutter Speed
		if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_FRAME_RATE_BIT))
		{
			//unpack
			if(!jausUnsignedShortFromBuffer(&message->shutterSpeed, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
		}

		// Image Control
		if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_FRAME_RATE_BIT))
		{
			//unpack
			if(!jausUnsignedShortFromBuffer(&imageControl, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;

			// Setup the datastructure's boolean values
			message->autoFocus = jausUnsignedShortIsBitSet(imageControl, JAUS_CAMERA_IMAGE_CONTROL_BF_AUTO_FOCUS_BIT)? JAUS_TRUE : JAUS_FALSE;
			message->autoIris = jausUnsignedShortIsBitSet(imageControl, JAUS_CAMERA_IMAGE_CONTROL_BF_AUTO_IRIS_BIT)? JAUS_TRUE : JAUS_FALSE;
			message->imageStabilization = jausUnsignedShortIsBitSet(imageControl, JAUS_CAMERA_IMAGE_CONTROL_BF_IMAGE_STABILIZATION_BIT)? JAUS_TRUE : JAUS_FALSE;
			message->whiteBalance = jausUnsignedShortIsBitSet(imageControl, JAUS_CAMERA_IMAGE_CONTROL_BF_WHITE_BALANCE_BIT)? JAUS_TRUE : JAUS_FALSE;
			message->syncFlash = jausUnsignedShortIsBitSet(imageControl, JAUS_CAMERA_IMAGE_CONTROL_BF_SYNC_FLASH_BIT)? JAUS_TRUE : JAUS_FALSE;
			message->redEye = jausUnsignedShortIsBitSet(imageControl, JAUS_CAMERA_IMAGE_CONTROL_BF_RED_EYE_BIT)? JAUS_TRUE : JAUS_FALSE;
			message->autoShutter = jausUnsignedShortIsBitSet(imageControl, JAUS_CAMERA_IMAGE_CONTROL_BF_AUTO_SHUTTER_BIT)? JAUS_TRUE : JAUS_FALSE;
			message->videoAutoGain = jausUnsignedShortIsBitSet(imageControl, JAUS_CAMERA_IMAGE_CONTROL_BF_AUTO_GAIN_BIT)? JAUS_TRUE : JAUS_FALSE;
			message->interlacedVideo = jausUnsignedShortIsBitSet(imageControl, JAUS_CAMERA_IMAGE_CONTROL_BF_INTERLACED_BIT)? JAUS_TRUE : JAUS_FALSE;
		}

		// Audio Control
		if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_FRAME_RATE_BIT))
		{
			//unpack
			if(!jausUnsignedShortFromBuffer(&audioControl, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;

			message->audioEnabled = jausUnsignedShortIsBitSet(audioControl, JAUS_CAMERA_AUDIO_CONTROL_BF_AUDIO_BIT)? JAUS_TRUE : JAUS_FALSE;
			message->audioAutoGain = jausUnsignedShortIsBitSet(audioControl, JAUS_CAMERA_AUDIO_CONTROL_BF_AUTO_GAIN_BIT)? JAUS_TRUE : JAUS_FALSE;
			message->stereoAudio = jausUnsignedShortIsBitSet(audioControl, JAUS_CAMERA_AUDIO_CONTROL_BF_STEREO_BIT)? JAUS_TRUE : JAUS_FALSE;
			message->directionalAudio = jausUnsignedShortIsBitSet(audioControl, JAUS_CAMERA_AUDIO_CONTROL_BF_DIRECTIONAL_BIT)? JAUS_TRUE : JAUS_FALSE;
			message->frontMicrophone = jausUnsignedShortIsBitSet(audioControl, JAUS_CAMERA_AUDIO_CONTROL_BF_FRONT_MICROPHONE_BIT)? JAUS_TRUE : JAUS_FALSE;
			message->rearMicrophone = jausUnsignedShortIsBitSet(audioControl, JAUS_CAMERA_AUDIO_CONTROL_BF_REAR_MICROPHONE_BIT)? JAUS_TRUE : JAUS_FALSE;
			message->leftMicrophone = jausUnsignedShortIsBitSet(audioControl, JAUS_CAMERA_AUDIO_CONTROL_BF_LEFT_MICROPHONE_BIT)? JAUS_TRUE : JAUS_FALSE;
			message->rightMicrophone = jausUnsignedShortIsBitSet(audioControl, JAUS_CAMERA_AUDIO_CONTROL_BF_RIGHT_MICROPHONE_BIT)? JAUS_TRUE : JAUS_FALSE;
		}

		return JAUS_TRUE;
	}
	else
	{
		return JAUS_FALSE;
	}
}

// Returns number of bytes put into the buffer
static int dataToBuffer(SetCameraCapabilitiesMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	int index = 0;
	JausUnsignedShort tempUShort = 0;
	JausByte tempByte = 0;
	JausUnsignedShort imageControl = 0;
	JausUnsignedShort audioControl = 0;

	if(bufferSizeBytes >= dataSize(message))
	{
		// Pack Message Fields to Buffer
		
		// Presence Vector
		if(!jausUnsignedShortToBuffer(message->presenceVector, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
	
		// CameraID
		if(!jausByteToBuffer(message->cameraID, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_BYTE_SIZE_BYTES;

		// Horizontal FOV
		if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_HORIZONTAL_FOV_BIT))
		{
			// Scaled UShort (0, JAUS_PI)
			tempUShort = jausUnsignedShortFromDouble(message->horizontalFovRadians, 0, JAUS_PI);

			//pack
			if(!jausUnsignedShortToBuffer(tempUShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
		}
	
		// Vertical FOV
		if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_VERTICAL_FOV_BIT))
		{
			// Scaled UShort (0, JAUS_PI)
			tempUShort = jausUnsignedShortFromDouble(message->verticalFovRadians, 0, JAUS_PI);

			//pack
			if(!jausUnsignedShortToBuffer(tempUShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
		}

		// Horizontal Resolution
		if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_HORIZONTAL_RESOLUTION_BIT))
		{
			//pack
			if(!jausUnsignedShortToBuffer(message->horizontalResolution, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
		}

		// Vertical Resolution
		if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_VERTICAL_RESOLUTION_BIT))
		{
			//pack
			if(!jausUnsignedShortToBuffer(message->verticalResolution, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
		}

		// Focus 
		if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_FOCUS_BIT))
		{
			// Scaled Byte (0, 100)
			tempByte = jausByteFromDouble(message->focusPercent, 0, 100);

			//pack
			if(!jausByteToBuffer(tempByte, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_BYTE_SIZE_BYTES;
		}
		
		// Iris
		if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_IRIS_BIT))
		{
			// Scaled Byte (0, 100)
			tempByte = jausByteFromDouble(message->irisPercent, 0, 100);

			//pack
			if(!jausByteToBuffer(tempByte, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_BYTE_SIZE_BYTES;
		}

		// Gain
		if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_GAIN_BIT))
		{
			// Scaled Byte (0, 100)
			tempByte = jausByteFromDouble(message->gainPercent, 0, 100);

			//pack
			if(!jausByteToBuffer(tempByte, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_BYTE_SIZE_BYTES;
		}

		// Frame Rate
		if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_FRAME_RATE_BIT))
		{
			//pack
			if(!jausUnsignedShortToBuffer(message->frameRate, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
		}

		// Shutter Speed
		if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_FRAME_RATE_BIT))
		{
			//pack
			if(!jausUnsignedShortToBuffer(message->shutterSpeed, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
		}

		// Image Control
		if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_FRAME_RATE_BIT))
		{
			// Clear the imageControl field
			imageControl = 0;

			// Setup the imageControl field
			if(message->autoFocus) jausUnsignedShortSetBit(&imageControl, JAUS_CAMERA_IMAGE_CONTROL_BF_AUTO_FOCUS_BIT);
			if(message->autoIris) jausUnsignedShortSetBit(&imageControl, JAUS_CAMERA_IMAGE_CONTROL_BF_AUTO_IRIS_BIT);
			if(message->imageStabilization) jausUnsignedShortSetBit(&imageControl, JAUS_CAMERA_IMAGE_CONTROL_BF_IMAGE_STABILIZATION_BIT);
			if(message->whiteBalance) jausUnsignedShortSetBit(&imageControl, JAUS_CAMERA_IMAGE_CONTROL_BF_WHITE_BALANCE_BIT);
			if(message->syncFlash) jausUnsignedShortSetBit(&imageControl, JAUS_CAMERA_IMAGE_CONTROL_BF_SYNC_FLASH_BIT);
			if(message->redEye) jausUnsignedShortSetBit(&imageControl, JAUS_CAMERA_IMAGE_CONTROL_BF_RED_EYE_BIT);
			if(message->autoShutter) jausUnsignedShortSetBit(&imageControl, JAUS_CAMERA_IMAGE_CONTROL_BF_AUTO_SHUTTER_BIT);
			if(message->videoAutoGain) jausUnsignedShortSetBit(&imageControl, JAUS_CAMERA_IMAGE_CONTROL_BF_AUTO_GAIN_BIT);
			if(message->interlacedVideo) jausUnsignedShortSetBit(&imageControl, JAUS_CAMERA_IMAGE_CONTROL_BF_INTERLACED_BIT);

			//pack
			if(!jausUnsignedShortToBuffer(imageControl, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
		}

		// Audio Control
		if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_FRAME_RATE_BIT))
		{
			// Clear the imageControl field
			audioControl = 0;

			if(message->audioEnabled) jausUnsignedShortSetBit(&audioControl, JAUS_CAMERA_AUDIO_CONTROL_BF_AUDIO_BIT);
			if(message->audioAutoGain) jausUnsignedShortSetBit(&audioControl, JAUS_CAMERA_AUDIO_CONTROL_BF_AUTO_GAIN_BIT);
			if(message->stereoAudio) jausUnsignedShortSetBit(&audioControl, JAUS_CAMERA_AUDIO_CONTROL_BF_STEREO_BIT);
			if(message->directionalAudio) jausUnsignedShortSetBit(&audioControl, JAUS_CAMERA_AUDIO_CONTROL_BF_DIRECTIONAL_BIT);
			if(message->frontMicrophone) jausUnsignedShortSetBit(&audioControl, JAUS_CAMERA_AUDIO_CONTROL_BF_FRONT_MICROPHONE_BIT);
			if(message->rearMicrophone) jausUnsignedShortSetBit(&audioControl, JAUS_CAMERA_AUDIO_CONTROL_BF_REAR_MICROPHONE_BIT);
			if(message->leftMicrophone) jausUnsignedShortSetBit(&audioControl, JAUS_CAMERA_AUDIO_CONTROL_BF_LEFT_MICROPHONE_BIT);
			if(message->rightMicrophone) jausUnsignedShortSetBit(&audioControl, JAUS_CAMERA_AUDIO_CONTROL_BF_RIGHT_MICROPHONE_BIT);

			//pack
			if(!jausUnsignedShortToBuffer(audioControl, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
		}
	}

	return index;
}

// Returns number of bytes put into the buffer
static unsigned int dataSize(SetCameraCapabilitiesMessage message)
{
	int index = 0;

	// Presence Vector
	index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;

	// CameraID
	index += JAUS_BYTE_SIZE_BYTES;

	// Horizontal FOV
	if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_HORIZONTAL_FOV_BIT))
	{
		index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
	}

	// Vertical FOV
	if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_VERTICAL_FOV_BIT))
	{
		index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
	}

	// Horizontal Resolution
	if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_HORIZONTAL_RESOLUTION_BIT))
	{
		index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
	}

	// Vertical Resolution
	if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_VERTICAL_RESOLUTION_BIT))
	{
		index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
	}

	// Focus 
	if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_FOCUS_BIT))
	{
		index += JAUS_BYTE_SIZE_BYTES;
	}
	
	// Iris
	if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_IRIS_BIT))
	{
		index += JAUS_BYTE_SIZE_BYTES;
	}

	// Gain
	if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_GAIN_BIT))
	{
		index += JAUS_BYTE_SIZE_BYTES;
	}

	// Frame Rate
	if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_FRAME_RATE_BIT))
	{
		index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
	}

	// Shutter Speed
	if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_FRAME_RATE_BIT))
	{
		index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
	}

	// Image Control
	if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_FRAME_RATE_BIT))
	{
		index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
	}

	// Audio Control
	if(jausUnsignedShortIsBitSet(message->presenceVector, JAUS_SET_CAMERA_CAPABILITIES_PV_FRAME_RATE_BIT))
	{
		index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
	}
	
	return index;
}

// ************************************************************************************************************** //
//                                    NON-USER CONFIGURED FUNCTIONS
// ************************************************************************************************************** //

SetCameraCapabilitiesMessage setCameraCapabilitiesMessageCreate(void)
{
	SetCameraCapabilitiesMessage message;

	message = (SetCameraCapabilitiesMessage)malloc( sizeof(SetCameraCapabilitiesMessageStruct) );
	if(message == NULL)
	{
		return NULL;
	}
	
	// Initialize Values
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
	
	dataInitialize(message);
	message->dataSize = dataSize(message);
	
	return message;	
}

void setCameraCapabilitiesMessageDestroy(SetCameraCapabilitiesMessage message)
{
	dataDestroy(message);
	jausAddressDestroy(message->source);
	jausAddressDestroy(message->destination);
	free(message);
}

JausBoolean setCameraCapabilitiesMessageFromBuffer(SetCameraCapabilitiesMessage message, unsigned char* buffer, unsigned int bufferSizeBytes)
{
	int index = 0;
	
	if(headerFromBuffer(message, buffer+index, bufferSizeBytes-index))
	{
		index += JAUS_HEADER_SIZE_BYTES;
		if(dataFromBuffer(message, buffer+index, bufferSizeBytes-index))
		{
			return JAUS_TRUE;
		}
		else
		{
			return JAUS_FALSE;
		}
	}
	else
	{
		return JAUS_FALSE;
	}
}

JausBoolean setCameraCapabilitiesMessageToBuffer(SetCameraCapabilitiesMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	if(bufferSizeBytes < setCameraCapabilitiesMessageSize(message))
	{
		return JAUS_FALSE; //improper size	
	}
	else
	{	
		message->dataSize = dataToBuffer(message, buffer+JAUS_HEADER_SIZE_BYTES, bufferSizeBytes - JAUS_HEADER_SIZE_BYTES);
		if(headerToBuffer(message, buffer, bufferSizeBytes))
		{
			return JAUS_TRUE;
		}
		else
		{
			return JAUS_FALSE; // headerToSetCameraCapabilitiesBuffer failed
		}
	}
}

SetCameraCapabilitiesMessage setCameraCapabilitiesMessageFromJausMessage(JausMessage jausMessage)
{
	SetCameraCapabilitiesMessage message;
	
	if(jausMessage->commandCode != commandCode)
	{
		return NULL; // Wrong message type
	}
	else
	{
		message = (SetCameraCapabilitiesMessage)malloc( sizeof(SetCameraCapabilitiesMessageStruct) );
		if(message == NULL)
		{
			return NULL;
		}
		
		message->properties.priority = jausMessage->properties.priority;
		message->properties.ackNak = jausMessage->properties.ackNak;
		message->properties.scFlag = jausMessage->properties.scFlag;
		message->properties.expFlag = jausMessage->properties.expFlag;
		message->properties.version = jausMessage->properties.version;
		message->properties.reserved = jausMessage->properties.reserved;
		message->commandCode = jausMessage->commandCode;
		message->destination = jausAddressCreate();
		*message->destination = *jausMessage->destination;
		message->source = jausAddressCreate();
		*message->source = *jausMessage->source;
		message->dataSize = jausMessage->dataSize;
		message->dataFlag = jausMessage->dataFlag;
		message->sequenceNumber = jausMessage->sequenceNumber;
		
		// Unpack jausMessage->data
		if(dataFromBuffer(message, jausMessage->data, jausMessage->dataSize))
		{
			return message;
		}
		else
		{
			return NULL;
		}
	}
}

JausMessage setCameraCapabilitiesMessageToJausMessage(SetCameraCapabilitiesMessage message)
{
	JausMessage jausMessage;
	
	jausMessage = (JausMessage)malloc( sizeof(struct JausMessageStruct) );
	if(jausMessage == NULL)
	{
		return NULL;
	}	
	
	jausMessage->properties.priority = message->properties.priority;
	jausMessage->properties.ackNak = message->properties.ackNak;
	jausMessage->properties.scFlag = message->properties.scFlag;
	jausMessage->properties.expFlag = message->properties.expFlag;
	jausMessage->properties.version = message->properties.version;
	jausMessage->properties.reserved = message->properties.reserved;
	jausMessage->commandCode = message->commandCode;
	jausMessage->destination = jausAddressCreate();
	*jausMessage->destination = *message->destination;
	jausMessage->source = jausAddressCreate();
	*jausMessage->source = *message->source;
	jausMessage->dataSize = dataSize(message);
	jausMessage->dataFlag = message->dataFlag;
	jausMessage->sequenceNumber = message->sequenceNumber;
	
	jausMessage->data = (unsigned char *)malloc(jausMessage->dataSize);
	jausMessage->dataSize = dataToBuffer(message, jausMessage->data, jausMessage->dataSize);
	
	return jausMessage;
}

unsigned int setCameraCapabilitiesMessageSize(SetCameraCapabilitiesMessage message)
{
	return (unsigned int)(dataSize(message) + JAUS_HEADER_SIZE_BYTES);
}

//********************* PRIVATE HEADER FUNCTIONS **********************//

static JausBoolean headerFromBuffer(SetCameraCapabilitiesMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
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
		
		message->dataSize = buffer[12] + ((buffer[13] & 0x0F) << 8);

		message->dataFlag = ((buffer[13] >> 4) & 0x0F);

		message->sequenceNumber = buffer[14] + (buffer[15] << 8);
		
		return JAUS_TRUE;
	}
}

static JausBoolean headerToBuffer(SetCameraCapabilitiesMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
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

