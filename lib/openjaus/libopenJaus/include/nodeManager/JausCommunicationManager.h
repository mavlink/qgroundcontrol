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
// File Name: JausCommunicationManager.h
//
// Written By: Danny Kent (jaus AT dannykent DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: 

#ifndef JAUS_COMMUNICATION_MANAGER
#define JAUS_COMMUNICATION_MANAGER

#ifdef WIN32
	#include <errno.h>
	#include <hash_map>
	#define HASH_MAP stdext::hash_map
#elif defined(__GNUC__)
	#include <ext/hash_map>
	#define HASH_MAP __gnu_cxx::hash_map
#endif

#include <vector>
#include "JausTransportInterface.h"
#include "utils/FileLoader.h"
#include "MessageRouter.h"
#include "SystemTree.h"
#include "EventHandler.h"

class JausCommunicationManager
{
public:
	JausCommunicationManager(void);
	virtual ~JausCommunicationManager(void);
	
	unsigned long getInterfaceCount(void);
	JausTransportInterface *getJausInterface(unsigned long index);
	JausTransportInterface *getJausInterface(std::string interfaceName);
	bool interfaceExists(std::string interfaceName);
	void enable(void);
	void disable(void);
	bool isEnabled(void);
	SystemTree *getSystemTree();
	virtual bool sendJausMessage(JausMessage message) = 0;
	virtual bool receiveJausMessage(JausMessage message, JausTransportInterface *jtInterface) = 0;
	MessageRouter *getMessageRouter();
	virtual bool startInterfaces(void) = 0;

protected:
	MessageRouter *msgRouter;
	std::vector <JausTransportInterface *> interfaces;
	HASH_MAP<int, JausTransportInterface *> interfaceMap;
	FileLoader *configData;
	SystemTree *systemTree;
	EventHandler *eventHandler;
	JausByte mySubsystemId;
	JausByte myNodeId;
	bool enabled;
};

#endif
