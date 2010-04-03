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
// File Name: JausMissionCommand.h
//
// Written By: Danny Kent (jaus AT dannykent DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file describes all the functionality associated with a JausMissionCommand. 
//                JausMissionCommands are used to support the storage and transfer of mission through the planning message set.
// Modified by: Luke Roseberry (MountainTop Technology, Inc) to add Planner
//              messages to OpenJAUS.
#ifndef JAUS_MISSION_COMMAND_H
#define JAUS_MISSION_COMMAND_H

#include "jaus.h"


#ifndef MISSION_BLOCKING_TYPES
#define MISSION_BLOCKING_TYPES
#define NON_BLOCKING 0
#define BLOCKING 1
#endif

// ************************************************************************************************************************************
//			JausMissionCommand
// ************************************************************************************************************************************
typedef struct
{
	JausUnsignedShort uid;		// Unique Jaus Message Id
	JausMessage message; //The jaus message
	JausByte blocking; //Indecates if this message is blocking 0 - NonBlocking, 1 - Blocking
  //struct JausMissionCommandStruct* next;
}JausMissionCommandStruct;
typedef JausMissionCommandStruct *JausMissionCommand;
;
// JausMissionCommand Constructor
JAUS_EXPORT JausMissionCommand missionCommandCreate(void);

// JausMissionCommand Constructor (from Buffer)
JAUS_EXPORT JausBoolean missionCommandFromBuffer(JausMissionCommand* messagePointer, unsigned char *buffer, unsigned int bufferSizeBytes);

// JausMissionCommand To Buffer
JAUS_EXPORT JausBoolean missionCommandToBuffer(JausMissionCommand message, unsigned char *buffer, unsigned int bufferSizeBytes);

// JausMissionCommand Destructor
JAUS_EXPORT void missionCommandDestroy(JausMissionCommand object);

// JausMissionCommand Buffer Size
JAUS_EXPORT unsigned int missionCommandSize(JausMissionCommand object);

#endif // JAUSMISSIONCOMMAND_H
