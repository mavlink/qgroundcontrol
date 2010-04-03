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
// File Name: jausMissionTask.c
//
// Written By: Danny Kent (jaus AT dannykent DOT com), Tom Galluzzo (galluzzo AT gmail DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: Stores a jaus task in a mission with the
//                additional data fields associated with that task
// Modified by: Luke Roseberry (MountainTop Technology, Inc) to add Planner
//              messages to OpenJAUS.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "jaus.h"
#include "type/jausMissionTask.h"

// JausMissionTask Constructor
JausMissionTask missionTaskCreate(void)
{
	JausMissionTask object;
	
	object = (JausMissionTask) malloc(sizeof(JausMissionTaskStruct));
	if(object)
	{
		object->taskId = newJausUnsignedShort(0); //Message unique ID
		object->commands = jausArrayCreate(); //List of JausMissionCommand structures for messages
    object->children = jausArrayCreate(); //List of JausMissionTask structures signifying the children tasks of this task
		object->bufferOffset = newJausInteger(0); //Internal variable used for ToBuffer processing
    return object;
	}
	else
	{
		return NULL;
	}
}

// JausMissionTask Constructor (from Buffer)
JausBoolean missionTaskFromBuffer(JausMissionTask *taskPointer, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	unsigned int index = 0;
	JausUnsignedShort numChildren = 0;
	JausUnsignedShort numMessages = 0;
	JausUnsignedInteger *childIndex = NULL;
	JausMissionTask task;
    JausMissionTask tempTask;
    JausMissionCommand tempCmd = NULL;
	int i = 0;

	task = missionTaskCreate();//(JausMissionTask) malloc(sizeof(JausMissionTaskStruct));
	*taskPointer = NULL;
	if(task)
	{	
		// Read task Unique Id		
		if(!jausUnsignedShortFromBuffer(&task->taskId, buffer+index, bufferSizeBytes-index))
		{
			free(task);
			return JAUS_FALSE;
		}
		index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;

    // Read the number of child tasks  
    if(!jausUnsignedShortFromBuffer(&numChildren, buffer+index, bufferSizeBytes-index))
    {
      free(task);
      return JAUS_FALSE;
    }
    index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
    
    childIndex = (JausUnsignedInteger*) malloc(JAUS_UNSIGNED_INTEGER_SIZE_BYTES * numChildren);
    
    for( i = 0; i<numChildren; i++)
    {
      // Read a child index 
      if(!jausUnsignedIntegerFromBuffer(&childIndex[i], buffer+index, bufferSizeBytes-index))
      {
        free(task);
        free(childIndex);
        return JAUS_FALSE;
      }
      index += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
    }
    
    // Read the number of messages 
    if(!jausUnsignedShortFromBuffer(&numMessages, buffer+index, bufferSizeBytes-index))
    {
      free(task);
      free(childIndex);
      return JAUS_FALSE;
    }
    index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
    
    // Read the messages
    for( i = 0; i<numMessages; i++)
    {
      // Read a command structure 
      if(!missionCommandFromBuffer(&tempCmd, buffer+index, bufferSizeBytes-index))
      {
        free(task);
        free(childIndex);
        return JAUS_FALSE;
      }
      index += missionCommandSize(tempCmd);
      
      //Add command to array
      jausArrayAdd(task->commands, tempCmd);
      
      tempCmd = NULL;
    }
    
    // Read the child tasks
    for( i = 0; i<numChildren; i++)
    {
      // Read a task structure 
      if(!missionTaskFromBuffer(&tempTask, buffer+index, bufferSizeBytes-index))
      {
        free(task);
        free(childIndex);
        return JAUS_FALSE;
      }
      index += missionTaskSize(tempTask);
      
      //Add command to array
      jausArrayAdd(task->children, tempTask);
      
      tempTask = NULL;
    }
    
    *taskPointer = task;
		return JAUS_TRUE;
	}
	else
	{
		return JAUS_FALSE;
	}
}

// JausMissionTask To Buffer
JausBoolean missionTaskToBuffer(JausMissionTask task, unsigned char *buffer, unsigned int bufferSizeBytes)
{
  int i = 0;
  unsigned int index = 0;
  JausUnsignedShort tempCount = 0;
  JausUnsignedInteger childIndex;

  if(task)
  { 
    // Write task Unique Id    
    if(!jausUnsignedShortToBuffer(task->taskId, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
    index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;

    if(task->children->elementCount > JAUS_UNSIGNED_SHORT_MAX_VALUE)
    {
      return JAUS_FALSE;
    }
    else
    {
      tempCount = task->children->elementCount;
    }
    // Write the number of child tasks  
    if(!jausUnsignedShortToBuffer(tempCount, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
    index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
    
    childIndex = task->bufferOffset; //Offset into message where this task started
    childIndex += JAUS_UNSIGNED_SHORT_SIZE_BYTES; //task Id
    childIndex += JAUS_UNSIGNED_SHORT_SIZE_BYTES; //Number of children
    childIndex += JAUS_UNSIGNED_INTEGER_SIZE_BYTES * task->children->elementCount; //Children Indexes
    childIndex += JAUS_UNSIGNED_SHORT_SIZE_BYTES; //Number of messages
    //Size of all messages
    for( i = 0; i<task->commands->elementCount; i++)
    {
      childIndex += missionCommandSize(task->commands->elementData[i]);
    }
    
    // Write all the child indexes
    for( i = 0; i<task->children->elementCount; i++)
    {
      // Write a child index 
      if(!jausUnsignedIntegerToBuffer(childIndex, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
      if(i < task->children->elementCount-1)
      {
        childIndex += missionTaskSize(task->children->elementData[i]);
      }
      index += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
    }
    
    if( task->commands->elementCount > JAUS_UNSIGNED_SHORT_MAX_VALUE)
    {
      return JAUS_FALSE;
    }
    else
    {
      tempCount = task->commands->elementCount;
    }
    // Write the number of messages 
    if(!jausUnsignedShortToBuffer(tempCount, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
    index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
    
    // Write the messages
    for( i = 0; i<task->commands->elementCount; i++)
    {
      // Write a command structure 
      if(!missionCommandToBuffer(task->commands->elementData[i], buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
      index += missionCommandSize(task->commands->elementData[i]);
      
    }
    
    // Write the child tasks
    for( i = 0; i<task->children->elementCount; i++)
    {
      // Write a task structure 
      ((JausMissionTask)task->children->elementData[i])->bufferOffset = index;
      if(!missionTaskToBuffer(task->children->elementData[i], buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
      index += missionTaskSize(task->children->elementData[i]);
    }
		return JAUS_TRUE;
	}
	
	return JAUS_FALSE;		
}

// JausMissionTask Destructor
void missionTaskDestroy(JausMissionTask object)
{
	if(object)
	{
		jausArrayDestroy(object->commands, (void*)missionCommandDestroy);
		jausArrayDestroy(object->children, (void*)missionTaskDestroy);
    
		free(object);
		object = NULL;
	}
}

// JausMissionTask Buffer Size
unsigned int missionTaskSize(JausMissionTask object)
{
	unsigned int size = 0;
	int i = 0;
	if(object)
	{
    size += JAUS_UNSIGNED_SHORT_SIZE_BYTES; //task Id
    size += JAUS_UNSIGNED_SHORT_SIZE_BYTES; //Number of children
    size += JAUS_UNSIGNED_INTEGER_SIZE_BYTES * object->children->elementCount; //Children Indexes
    size += JAUS_UNSIGNED_SHORT_SIZE_BYTES; //Number of messages
    //Size of all messages
    for( i = 0; i<object->commands->elementCount; i++)
    {
      size += missionCommandSize(object->commands->elementData[i]);
    }
    //Size of all the child tasks
    for( i = 0; i<object->children->elementCount; i++)
    {
      size += missionTaskSize(object->children->elementData[i]);
    }
  }	
	return size;
}

