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
// File Name: setCameraFormatOptionsMessage.h
//
// Written By: Danny Kent (jaus AT dannykent DOT com)
//
// Version: 3.3.0a
//
// Date: 03/20/08
//
// Description: This file defines the attributes of a SetCameraFormatOptionsMessage

#ifndef SET_CAMERA_FORMAT_OPTIONS_MESSAGE_H
#define SET_CAMERA_FORMAT_OPTIONS_MESSAGE_H

#include "jaus.h"

#ifndef JAUS_CAMERA_FORMAT_OPTIONS_PV
#define JAUS_CAMERA_FORMAT_OPTIONS_PV
#define JAUS_CAMERA_FORMAT_OPTIONS_PV_AUDIO_FORMAT_BIT	0
#define JAUS_CAMERA_FORMAT_OPTIONS_PV_IMAGE_FORMAT_BIT	1
#define JAUS_CAMERA_FORMAT_OPTIONS_PV_FORMAT_OPTION_BIT 2
#endif

#ifndef JAUS_AUDIO_FORMAT_ENUM
#define JAUS_AUDIO_FORMAT_ENUM
typedef enum 
{	
	UnusedAudioFormat=0,
	RAW_AUDIO=1, 
	PCM=2,
	AU=3,
	WAV=4,
	MID=5,
	MP3=6,
	MP2=7,
	ASF=8 
}JausAudioFormatEnum;
#endif

#ifndef JAUS_IMAGE_FORMAT_ENUM
#define JAUS_IMAGE_FORMAT_ENUM
typedef enum  
{
	UnusedImageFormat=0,
	MPEG2=1,
	MPEG4=2,
	MJPEG=3,
	NTSC=4,
	PAL=5,
	TIFF=6,
	JPEG=7,
	GIF=8,
	H263=9,
	H264=10,
	PNG=11,
	BMP=12,
	RAW_IMAGE=13,
	PPM=14,
	PGM=15,
	PNM=16 
} JausImageFormatEnum;
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
	JausByte presenceVector;
	JausByte cameraID;
	JausUnsignedInteger formatOption;
	JausAudioFormatEnum audioFormat;
	JausImageFormatEnum imageFormat;
	
}SetCameraFormatOptionsMessageStruct;

typedef SetCameraFormatOptionsMessageStruct* SetCameraFormatOptionsMessage;

JAUS_EXPORT SetCameraFormatOptionsMessage setCameraFormatOptionsMessageCreate(void);
JAUS_EXPORT void setCameraFormatOptionsMessageDestroy(SetCameraFormatOptionsMessage);

JAUS_EXPORT JausBoolean setCameraFormatOptionsMessageFromBuffer(SetCameraFormatOptionsMessage message, unsigned char* buffer, unsigned int bufferSizeBytes);
JAUS_EXPORT JausBoolean setCameraFormatOptionsMessageToBuffer(SetCameraFormatOptionsMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);

JAUS_EXPORT SetCameraFormatOptionsMessage setCameraFormatOptionsMessageFromJausMessage(JausMessage jausMessage);
JAUS_EXPORT JausMessage setCameraFormatOptionsMessageToJausMessage(SetCameraFormatOptionsMessage message);

JAUS_EXPORT unsigned int setCameraFormatOptionsMessageSize(SetCameraFormatOptionsMessage message);

#endif // SET_CAMERA_FORMAT_OPTIONS_MESSAGE_H
