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
// File Name: JausTransportInterface.h
//
// Written By: Danny Kent (jaus AT dannykent DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: 

#ifndef JAUS_TRANSPORT_INTERFACE
#define JAUS_TRANSPORT_INTERFACE

#ifdef WIN32
	#include "pthread.h"	
#elif defined(__GNUC__)
	#include <pthread.h>
	#include <errno.h>
#endif

#include <string>
#include "JausTransportQueue.h"
#include "EventHandler.h"
#include "SystemTree.h"
#include "utils/FileLoader.h"
#include "jaus.h"

enum JausTransportType {UNKNOWN_INTERFACE, SUBSYSTEM_INTERFACE, NODE_INTERFACE, COMPONENT_INTERFACE};

class JausCommunicationManager;

extern "C" void *ThreadRun(void *);

class JausTransportInterface
{
public:
	JausTransportInterface(void);
	virtual ~JausTransportInterface(void);

	std::string getName();
	JausCommunicationManager *getCommunicationManager();
	void setCommunicationManager(JausCommunicationManager *commMngr);

	bool isRunning();
	void stopThread();

	JausTransportType getType(void);
	unsigned long queueSize();
	void queueJausMessage(JausMessage message);

	virtual bool startInterface(void) = 0;
	virtual bool stopInterface(void) = 0;
	virtual bool processMessage(JausMessage message) = 0;
	virtual std::string toString() = 0;
	virtual void run() = 0;

protected:
	void startThread();
	void wakeThread();

	EventHandler *eventHandler;
	std::string name;
	JausTransportType type;
	JausTransportQueue queue;
	bool running;
	JausCommunicationManager *commMngr;
	FileLoader *configData;
	int pThreadId;
	pthread_t pThread;
	pthread_attr_t threadAttributes;
	pthread_cond_t threadConditional;
	pthread_mutex_t threadMutex;

	JausByte mySubsystemId;
};

#endif
