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
// File Name: setDiscreteDevicesMessage.h
//
// Written By: Danny Kent (jaus AT dannykent DOT com), Tom Galluzzo 
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file defines the attributes of a SetDiscreteDevicesMessage

#ifndef SET_DISCRETE_DEVICES_MESSAGE_H
#define SET_DISCRETE_DEVICES_MESSAGE_H

#include "jaus.h"

#ifndef JAUS_DEVICES_PV
#define JAUS_DEVICES_PV
#define JAUS_DEVICES_PV_PROPULSION_BIT	0
#define JAUS_DEVICES_PV_PARKING_BIT		1
#define JAUS_DEVICES_PV_GEAR_BIT		2
#define JAUS_DEVICES_PV_TRANSFER_BIT	3
#endif

#ifndef JAUS_DEVICES_PROPULSION_BF
#define JAUS_DEVICES_PROPULSION_BF
#define JAUS_DEVICES_PROPULSION_BF_MAIN_POWER_BIT		0
#define JAUS_DEVICES_PROPULSION_BF_MAIN_FUEL_BIT		1
#define JAUS_DEVICES_PROPULSION_BF_AUXILARY_FUEL_BIT	2
#define JAUS_DEVICES_PROPULSION_BF_AUXILARY_POWER_BIT	3
#define JAUS_DEVICES_PROPULSION_BF_STARTING_DEVICE_BIT	4
#define JAUS_DEVICES_PROPULSION_BF_COLD_START_BIT		5
#define JAUS_DEVICES_PROPULSION_BF_AUTO_START_BIT		6
#define JAUS_DEVICES_PROPULSION_BF_AUTO_SHUTDOWN_BIT	7
#endif

#ifndef JAUS_DEVICES_OTHER_BF
#define JAUS_DEVICES_OTHER_BF
#define JAUS_DEVICES_OTHER_BF_PARKING_BRAKE_BIT	0
#define JAUS_DEVICES_OTHER_BF_HORN_BIT			1
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

	JausByte presenceVector;
	JausByte gear;
	JausByte transferCase;
	
	// Main Propulsion
	JausBoolean mainPropulsion;
	JausBoolean mainFuelSupply;
	JausBoolean auxFuelSupply;
	JausBoolean powerAuxDevices;
	JausBoolean startingDevice;
	JausBoolean coldStart;
	JausBoolean automaticStart;
	JausBoolean automaticStop;

	// Parking, Brake and Horn
	JausBoolean parkingBrake;
	JausBoolean horn;

}SetDiscreteDevicesMessageStruct;

typedef SetDiscreteDevicesMessageStruct* SetDiscreteDevicesMessage;

JAUS_EXPORT SetDiscreteDevicesMessage setDiscreteDevicesMessageCreate(void);
JAUS_EXPORT void setDiscreteDevicesMessageDestroy(SetDiscreteDevicesMessage);

JAUS_EXPORT JausBoolean setDiscreteDevicesMessageFromBuffer(SetDiscreteDevicesMessage message, unsigned char* buffer, unsigned int bufferSizeBytes);
JAUS_EXPORT JausBoolean setDiscreteDevicesMessageToBuffer(SetDiscreteDevicesMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);

JAUS_EXPORT SetDiscreteDevicesMessage setDiscreteDevicesMessageFromJausMessage(JausMessage jausMessage);
JAUS_EXPORT JausMessage setDiscreteDevicesMessageToJausMessage(SetDiscreteDevicesMessage message);

JAUS_EXPORT unsigned int setDiscreteDevicesMessageSize(SetDiscreteDevicesMessage message);

#endif // SET_DISCRETE_DEVICES_MESSAGE_H
