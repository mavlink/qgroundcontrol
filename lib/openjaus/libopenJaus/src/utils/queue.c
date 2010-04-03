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
// File Name: queue.c
//
// Written By: Tom Galluzzo (galluzzo AT gmail DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description:	This file describes the functionality associated with a generic void pointer Queue object.

#include <stdlib.h>
#include <pthread.h>
#include "utils/queue.h"

Queue queueCreate(void)
{
	Queue queue = NULL;
	
	queue = (Queue)malloc( sizeof(QueueStruct) );
	if(queue == NULL)
	{
		return NULL;
	}
	
	queue->firstObject = NULL;
	queue->lastObject = NULL;
	queue->size = 0;
	pthread_mutex_init(&queue->mutex, NULL);

	return queue;
}

void queueDestroy(Queue queue, void (*objectDestroy)(void *))
{
	if(queue == NULL)
	{
		return;
	}

	queueEmpty(queue, objectDestroy);

	pthread_mutex_destroy(&queue->mutex);
	
	free(queue);
}

void *queuePopNoLock(Queue queue)
{
	QueueObject *queueObject;
	void *object;

	queueObject = queue->firstObject;

	if(queueObject)
	{
		queue->firstObject = queueObject->nextObject;
		if(queue->firstObject == NULL)
		{
			queue->lastObject = NULL;
		}
		queue->size--;
	
		object = queueObject->object;
		
		free(queueObject);
	}
	else
	{
		object = NULL;
	}

	return object;	
}

void queueEmpty(Queue queue, void (*objectDestroy)(void *))
{
	void *object;

	if(queue)
	{
		pthread_mutex_lock(&queue->mutex);

		while(queue->size)
		{
			object = queuePopNoLock(queue);

			if(objectDestroy)
			{
				objectDestroy(object);
			}
		}

		pthread_mutex_unlock(&queue->mutex);
	}
}

void *queuePop(Queue queue)
{
	void *object;

	pthread_mutex_lock(&queue->mutex);

	object = queuePopNoLock(queue);

	pthread_mutex_unlock(&queue->mutex);

	return object;	
}

void queuePush(Queue queue, void *object)
{
	QueueObject *queueObject;

	queueObject = (QueueObject*)malloc( sizeof(QueueObject) );
	queueObject->object = object;
	queueObject->nextObject = NULL;

	pthread_mutex_lock(&queue->mutex);
	
	if(queue->firstObject == NULL)
	{
		queue->firstObject = queueObject;
	}

	if(queue->lastObject)
	{
		queue->lastObject->nextObject = queueObject;
	}

	queue->lastObject = queueObject;

	queue->size++;

	pthread_mutex_unlock(&queue->mutex);
}
