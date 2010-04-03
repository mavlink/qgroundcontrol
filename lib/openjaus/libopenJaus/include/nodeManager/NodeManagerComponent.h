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
// File Name: NodeManagerComponent.h
//
// Written By: Danny Kent (jaus AT dannykent DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: 

#ifndef NODE_MANAGER_COMPONENT_H
#define NODE_MANAGER_COMPONENT_H

#include "LocalComponent.h"

#if defined(WIN32)
	#include <hash_map>
	#define HASH_MAP stdext::hash_map
#elif defined(__GNUC__)
	#include <ext/hash_map>
	#define HASH_MAP __gnu_cxx::hash_map
#else
	#error "Hash Map undefined in SystemTable.h."
#endif

class JausComponentCommunicationManager;
class SystemTree;

#define NM_RATE_HZ			5
#define MAXIMUM_EVENT_ID	255
#define REFRESH_TIME_SEC	1

class NodeManagerComponent : public LocalComponent, EventHandler
{
public:
	NodeManagerComponent(FileLoader *configData, EventHandler *handler, JausComponentCommunicationManager *cmptComms);
	~NodeManagerComponent(void);

	bool processMessage(JausMessage message);
	std::string toString();
	void handleEvent(NodeManagerEvent *e);
	bool startInterface(void);
	bool stopInterface(void);

	JausAddress checkInLocalComponent(int cmptId);
	void checkOutLocalComponent(int subsId, int nodeId, int cmptId, int instId);
	void checkOutLocalComponent(JausAddress address);

private:
	bool processReportIdentification(JausMessage message);
	bool processReportConfiguration(JausMessage message);
	bool processReportServices(JausMessage message);
	bool processReportHeartbeatPulse(JausMessage message);
	bool processCreateEvent(JausMessage message);
	bool processCancelEvent(JausMessage message);
	bool processCreateServiceConnection(JausMessage message);
	bool processActivateServiceConnection(JausMessage message);
	bool processSuspendServiceConnection(JausMessage message);
	bool processTerminateServiceConnection(JausMessage message);
	bool processRequestComponentControl(JausMessage message);
	bool processQueryComponentAuthority(JausMessage message);
	bool processQueryComponentStatus(JausMessage message);
	bool processQueryHeartbeatPulse(JausMessage message);
	bool processQueryConfiguration(JausMessage message);
	bool processQueryIdentification(JausMessage message);
	bool processQueryServices(JausMessage message);
	bool processConfirmEvent(JausMessage message);
	bool processEvent(JausMessage message);

	void sendNodeChangedEvents();
	void sendSubsystemChangedEvents();
	void generateHeartbeats();
	bool setupJausServices();

	void sendNodeShutdownEvents();
	void sendSubsystemShutdownEvents();

	bool sendQueryNodeIdentification(JausAddress address);
	bool sendQuerySubsystemIdentification(JausAddress address);
	bool sendQueryComponentIdentification(JausAddress address);
	
	bool sendQueryNodeConfiguration(JausAddress address, bool createEvent);
	bool sendQuerySubsystemConfiguration(JausAddress address, bool createEvent);
	bool sendQueryComponentServices(JausAddress address);

	void startupState();
	void intializeState();
	void standbyState();
	void readyState();
	void emergencyState();
	void failureState();
	void shutdownState();
	void allState();

	int getNextEventId();

	HASH_MAP <int, JausAddress> nodeChangeList;
	HASH_MAP <int, JausAddress> subsystemChangeList;
	bool eventId[255];
	SystemTree *systemTree;
};

#endif
