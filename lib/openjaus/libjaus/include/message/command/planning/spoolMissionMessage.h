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
// File Name: spoolMissionMessage.h
//
// Written By: Danny Kent (jaus AT dannykent DOT com), Tom Galluzzo 
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file defines the attributes of a SpoolMissionMessage

#ifndef SPOOL_MISSION_MESSAGE_H
#define SPOOL_MISSION_MESSAGE_H

#include "jaus.h"
#include "type/jausMissionTask.h"


// Mission Proc Types: non-redundant declaration
#ifndef JAUS_MISSION_BLOCKING_TYPES
#define JAUS_MISSION_BLOCKING_TYPES
#define JAUS_NON_BLOCKING         0
#define JAUS_BLOCKING             1
#endif
// Mission Proc Types: non-redundant declaration
#ifndef JAUS_MISSION_SPOOLING_AND_MANIP_TYPES
#define JAUS_MISSION_SPOOLING_AND_MANIP_TYPES
#define JAUS_REPLACE_CURRENT_MISSION_WITH_NEW_MISSION  0 
#define JAUS_APPEND_NEW_MISSION_TO_CURRENT_MISSION     1
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


  //MESSAGE DATA MEMBERS
  JausUnsignedShort missionId; //Field 1: identifies the unique 
                               //Mission to be spooled or manipulated.                       
  JausByte appendFlag;         //Field 2: 0 = replace current mission with
                               //new mission. 1= Append new mission to
                               //current mission.
  JausMissionTask naryTree;    //Fields [3 to 8+n+3n] are dynamcially allocated
                               //as required to form the tree levels where level
                               //zero consists of only one task.  Tree levels
                               //grow down from a task as children. 
                               //Each task ends up having a unique taskId and:
                               //  * 1 to N messages each with a uid and
                               //    blocking flag.
                               //  * 1 to M Children (tasks) each with unique
                               //    taskId and messages of their own.
                               //NOTE: Field 4 (number of Children) is stored
                               //in the JausArray for JausMissionTaskStruct 
                               //element children.
                               //NOTE: Field 4+n (child indexs) are not stored
                               //in a structure.  For offsets to task
                               //children data in the spool mission message
                               //see the jausMissionTask.cpp functions.
                               //NOTE: Field 5+n (number of messages in task)
                               //is stored in the JausArray for
                               //JausMissionTaskStruct element commands.  Also
                               //see JausMissionCommand.

}SpoolMissionMessageStruct;

typedef SpoolMissionMessageStruct* SpoolMissionMessage;

JAUS_EXPORT SpoolMissionMessage spoolMissionMessageCreate(void);
JAUS_EXPORT void spoolMissionMessageDestroy(SpoolMissionMessage);

JAUS_EXPORT JausBoolean spoolMissionMessageFromBuffer(SpoolMissionMessage message, unsigned char* buffer, unsigned int bufferSizeBytes);
JAUS_EXPORT JausBoolean spoolMissionMessageToBuffer(SpoolMissionMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);

JAUS_EXPORT SpoolMissionMessage spoolMissionMessageFromJausMessage(JausMessage jausMessage);
JAUS_EXPORT JausMessage spoolMissionMessageToJausMessage(SpoolMissionMessage message);

JAUS_EXPORT unsigned int spoolMissionMessageSize(SpoolMissionMessage message);

#endif // SPOOL_MISSION_MESSAGE_H
