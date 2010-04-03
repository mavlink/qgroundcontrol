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
// File Name: jausMissionCommand.c
//
// Written By: Danny Kent (jaus AT dannykent DOT com), Tom Galluzzo (galluzzo AT gmail DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: Stores a single jaus message in a task in a mission with the
//                additional data fields associated with that message
// Modified by: Luke Roseberry (MountainTop Technology, Inc) to add Planner
//              messages to OpenJAUS.

#include <stdlib.h>
#include "jaus.h"

// JausMissionCommand Constructor
JausMissionCommand missionCommandCreate(void)
{
	JausMissionCommand object;
	
	object = (JausMissionCommand) malloc(sizeof(JausMissionCommandStruct));
	if(object)
	{
		object->uid = newJausUnsignedShort(0); //Message unique ID
		object->message = NULL; //The Jaus Message
		object->blocking = newJausByte(0); //Indicates whether the message is blocking
		//object->next = NULL; // The next JausMissionCommand for linked lists
		return object;
	}
	else
	{
		return NULL;
	}
}

// JausMissionCommand Constructor (from Buffer)
JausBoolean missionCommandFromBuffer(JausMissionCommand *messagePointer, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	unsigned int index = 0;
  JausMissionCommand message;
	message = (JausMissionCommand) malloc(sizeof(JausMissionCommandStruct));
  *messagePointer = NULL;
	if(message)
	{	
	
		// Read Unigue Id		
		if(!jausUnsignedShortFromBuffer(&message->uid, buffer+index, bufferSizeBytes-index))
		{
			free(message);
			return JAUS_FALSE;
		}
		index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;

    // Read Jaus Message
    message->message = jausMessageCreate();
    if(!jausMessageFromBuffer(message->message, buffer+index, bufferSizeBytes-index))
    {
      free(message);
      return JAUS_FALSE;
    }
    index += jausMessageSize(message->message);
    
    // Read blocking status
    if(!jausByteFromBuffer(&message->blocking, buffer+index, bufferSizeBytes-index))
    {
      free(message);
      return JAUS_FALSE;
    }
    index += JAUS_BYTE_SIZE_BYTES;
    
    //message->next = NULL;
    
    *messagePointer = message;
		return JAUS_TRUE;
	}
	else
	{
		return JAUS_FALSE;
	}
}

// JausMissionCommand To Buffer
JausBoolean missionCommandToBuffer(JausMissionCommand message, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	unsigned int index = 0;

	
	if(message)
	{
    
		// uid
		if(!jausUnsignedShortToBuffer(message->uid, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
		
    // Jaus Message
    if(!jausMessageToBuffer(message->message, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;		
    index += jausMessageSize(message->message);

		// Blocking status
		if(!jausByteToBuffer(message->blocking, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_BYTE_SIZE_BYTES;

		return JAUS_TRUE;
	}
	
	return JAUS_FALSE;		
}

// JausMissionCommand Destructor
void missionCommandDestroy(JausMissionCommand object)
{
	if(object)
	{
    if(!object->message)
		  jausMessageDestroy(object->message);
		

		free(object);
		object = NULL;
	}
}

// JausMissionCommand Buffer Size
unsigned int missionCommandSize(JausMissionCommand object)
{
	unsigned int size = 0;
	
	if(object)
	{
    size += JAUS_UNSIGNED_SHORT_SIZE_BYTES; //uid
    
    size += jausMessageSize(object->message); //Jaus Message
    
		size += JAUS_BYTE_SIZE_BYTES;		// Blocking status
  }	
	return size;
}

