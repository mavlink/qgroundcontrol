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
// File Name: JausTransportInterface.cpp
//
// Written By: Danny Kent (jaus AT dannykent DOT com) 
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file defines a JausTransportInterface.cpp class.

#include "nodeManager/JausTransportInterface.h"
#include "nodeManager/JausCommunicationManager.h"

JausTransportInterface::JausTransportInterface(void)
{
	this->running = false;
}

JausTransportInterface::~JausTransportInterface(void) {}

std::string JausTransportInterface::getName()
{
	return this->name;
}

JausCommunicationManager *JausTransportInterface::getCommunicationManager()
{
	return this->commMngr;
}

void JausTransportInterface::setCommunicationManager(JausCommunicationManager *commMngr)
{
	this->commMngr = commMngr;
}

bool JausTransportInterface::isRunning()
{
	return this->running;
}

void JausTransportInterface::stopThread()
{
	wakeThread();
	pthread_join(this->pThread, NULL);
	this->queue.emptyQueue();
	pthread_mutex_destroy(&this->threadMutex);
	pthread_cond_destroy(&this->threadConditional);
}

void JausTransportInterface::startThread()
{
	int retVal;
	char errorString[128] = {0};
	
	retVal = pthread_cond_init(&this->threadConditional, NULL); 
	if(retVal != 0)
	{
		sprintf(errorString, "JausTransportInterface: pthread_cond_init returned error code: %d", retVal);
		throw errorString;
	}
	
	retVal = pthread_mutex_init(&this->threadMutex, NULL);
	if(retVal != 0)
	{
		sprintf(errorString, "JausTransportInterface: pthread_mutex_init returned error code: %d", retVal);
		throw errorString;
	}
	
	retVal = pthread_attr_init(&this->threadAttributes);
	if(retVal != 0)
	{
		sprintf(errorString, "JausTransportInterface: pthread_attr_init returned error code: %d", retVal);
		throw errorString;
	}

	retVal = pthread_attr_setdetachstate(&this->threadAttributes, PTHREAD_CREATE_JOINABLE);
	if(retVal != 0)
	{
		sprintf(errorString, "JausTransportInterface: pthread_attr_setdetachstate returned error code: %d", retVal);
		throw errorString;
	}

	retVal = pthread_create(&this->pThread, &this->threadAttributes, ThreadRun, this);
	if(retVal != 0)
	{
		sprintf(errorString, "JausTransportInterface: pthread_attr_destroy returned error code: %d", retVal);
		throw errorString;
	}

	retVal = pthread_attr_destroy(&this->threadAttributes);
	if(retVal != 0)
	{
		sprintf(errorString, "JausTransportInterface: pthread_attr_destroy returned error code: %d", retVal);
		throw errorString;
	}
}

JausTransportType JausTransportInterface::getType(void)
{
	return this->type;
}

unsigned long JausTransportInterface::queueSize()
{
	return this->queue.size();
}

void JausTransportInterface::queueJausMessage(JausMessage message)
{
	if(this->running)
	{
		this->queue.push(message);
		wakeThread();
	}
	else
	{
		jausMessageDestroy(message);
	}
}

void JausTransportInterface::wakeThread()
{
	pthread_cond_signal(&this->threadConditional);
}

void *ThreadRun(void *obj)
{
	JausTransportInterface *jtInterface = (JausTransportInterface *)obj;
	jtInterface->run();
	return NULL;
}

