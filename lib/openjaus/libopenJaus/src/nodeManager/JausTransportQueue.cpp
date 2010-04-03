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
// File Name: JausTransportQueue.cpp
//
// Written By: Danny Kent (jaus AT dannykent DOT com) 
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: Provides a FIFO queue for JausTransportPackets. Supports monitoring.

#include "nodeManager/JausTransportQueue.h"
#include "nodeManager/JausTransportInterface.h"
#include "nodeManager/JausCommunicationManager.h"

JausTransportQueue::JausTransportQueue(void)
{
	pthread_mutex_init(&mutex, NULL);
}

JausTransportQueue::~JausTransportQueue(void)
{
	//list.~queue();
	pthread_mutex_destroy(&mutex);
}

void JausTransportQueue::emptyQueue(void)
{
	pthread_mutex_lock(&mutex);
	while(!list.empty())
	{
		JausMessage out = list.front();
		list.pop();
		jausMessageDestroy(out);
	}

	pthread_mutex_unlock(&mutex);
}

void JausTransportQueue::push(JausMessage inc)
{
	pthread_mutex_lock(&mutex);
	list.push(inc);
	pthread_mutex_unlock(&mutex);
}

JausMessage JausTransportQueue::pop(void)
{
	pthread_mutex_lock(&mutex);
	if(!list.empty())
	{
		JausMessage out = list.front();
		list.pop();

		pthread_mutex_unlock(&mutex);
		return out;
	}

	pthread_mutex_unlock(&mutex);
	return NULL;

}

bool JausTransportQueue::isEmpty(void)
{
	return list.empty();
}

unsigned long JausTransportQueue::size()
{
	return (unsigned long)list.size();
}

