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
// File Name: SystemTree.h
//
// Written By: Danny Kent (jaus AT dannykent DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: 

#ifndef SYSTEM_TREE_H
#define SYSTEM_TREE_H

#include <list>
#include "utils/FileLoader.h"
#include "EventHandler.h"
#include "jaus.h"

class SystemTree
{
public:
	SystemTree(FileLoader *configData, EventHandler *handler);
	~SystemTree(void);

	bool updateComponentTimestamp(JausAddress address);
	bool updateNodeTimestamp(JausAddress address);
	bool updateSubsystemTimestamp(JausAddress address);

	bool setComponentServices(JausAddress address, JausArray inputServices);

	bool setSubsystemIdentification(JausAddress address, char *identification);
	bool setNodeIdentification(JausAddress address, char *identification);
	bool setComponentIdentification(JausAddress address, char *identification);

	char *getSubsystemIdentification(JausSubsystem subsystem);
	char *getSubsystemIdentification(JausAddress address);
	char *getSubsystemIdentification(int subsId);

	char *getNodeIdentification(JausNode node);
	char *getNodeIdentification(JausAddress address);
	char *getNodeIdentification(int subsId, int nodeId);

	bool hasComponent(JausComponent cmpt);
	bool hasComponent(JausAddress address);
	bool hasComponent(int subsystemId, int nodeId, int componentId, int instanceId);

	bool hasNode(JausNode node);
	bool hasNode(JausAddress address);
	bool hasNode(int subsId, int nodeId);

	bool hasSubsystem(JausSubsystem subsystem);
	bool hasSubsystem(JausAddress address);
	bool hasSubsystem(int subsId);

	bool hasSubsystemIdentification(JausSubsystem subsystem);
	bool hasSubsystemIdentification(JausAddress address);
	bool hasSubsystemIdentification(int subsId);

	bool hasSubsystemConfiguration(JausSubsystem subsystem);
	bool hasSubsystemConfiguration(JausAddress address);
	bool hasSubsystemConfiguration(int subsId);

	bool hasNodeIdentification(JausNode node);
	bool hasNodeIdentification(JausAddress address);
	bool hasNodeIdentification(int subsId, int nodeId);

	bool hasNodeConfiguration(JausNode node);
	bool hasNodeConfiguration(JausAddress address);
	bool hasNodeConfiguration(int subsId, int nodeId);

	bool hasComponentIdentification(JausComponent cmpt);
	bool hasComponentIdentification(JausAddress address);
	bool hasComponentIdentification(int subsystemId, int nodeId, int componentId, int instanceId);

	bool hasComponentServices(JausComponent cmpt);
	bool hasComponentServices(JausAddress address);
	bool hasComponentServices(int subsystemId, int nodeId, int componentId, int instanceId);

	JausSubsystem *getSystem(void);

	JausSubsystem getSubsystem(JausSubsystem subsystem);
	JausSubsystem getSubsystem(JausAddress address);
	JausSubsystem getSubsystem(int subsId);

	JausNode getNode(JausNode node);
	JausNode getNode(JausAddress address);
	JausNode getNode(int subsId, int nodeId);

	JausComponent getComponent(JausComponent cmpt);
	JausComponent getComponent(JausAddress address);
	JausComponent getComponent(int subsystemId, int nodeId, int componentId, int instanceId);

	bool addSubsystem(JausSubsystem subs);
	bool addSubsystem(JausAddress address, JausSubsystem subs);
	bool addSubsystem(int subsId, JausSubsystem subs);

	bool addNode(JausNode node);
	bool addNode(JausAddress address, JausNode node);
	bool addNode(int subsId, int nodeId, JausNode node);

	bool addComponent(JausComponent cmpt);
	bool addComponent(JausAddress address, JausComponent cmpt);
	bool addComponent(int subsystemId, int nodeId, int componentId, int instanceId, JausComponent cmpt);

	bool removeSubsystem(JausSubsystem subsystem);
	bool removeSubsystem(JausAddress address);
	bool removeSubsystem(int subsId);

	bool removeNode(JausNode node);
	bool removeNode(JausAddress address);
	bool removeNode(int subsId, int nodeId);
	
	bool removeComponent(JausComponent cmpt);
	bool removeComponent(JausAddress address);
	bool removeComponent(int subsystemId, int nodeId, int componentId, int instanceId);

	bool replaceSubsystem(JausAddress address, JausSubsystem newSubs);
	bool replaceSubsystem(int subsystemId, JausSubsystem newSubs);

	bool replaceNode(JausAddress address, JausNode newNode);
	bool replaceNode(int subsystemId, int nodeId, JausNode newNode);
	bool replaceComponent(JausAddress address, JausComponent newCmpt);

	JausAddress lookUpAddress(JausAddress address);
	JausAddress lookUpAddress(int lookupSubs, int lookupNode, int lookupCmpt, int lookupInst);
	JausAddress lookUpAddress2(int lookupSubs, int lookupNode, int lookupCmpt, int lookupInst);
	JausAddress lookUpAddressInNode(JausNode node, int lookupCmpt, int lookupInst);
	JausAddress lookUpAddressInSubsystem(JausSubsystem subs, int lookupNode, int lookupCmpt, int lookupInst);
	
	JausAddress lookUpServiceInNode(int nodeId, int commandCode, int serviceType);
	JausAddress lookUpServiceInNode(JausNode node, int commandCode, int serviceType);
	JausAddress lookUpServiceInSubsystem(int subsId, int nodeId, int commandCode, int serviceType);
	JausAddress lookUpServiceInSubsystem(JausSubsystem subs, int commandCode, int serviceType);
	JausAddress lookUpServiceInSystem(int commandCode, int serviceType);
	JausAddress lookUpService(JausAddress address, int commandCode, int serviceType);

	unsigned char getNextInstanceId(JausAddress address);
	bool registerEventHandler(EventHandler *handler);

	std::string toString();
	std::string toDetailedString();
	void refresh();

private:
	FileLoader *configData;
	std::list <EventHandler *> eventHandlers;
	JausSubsystem system[255];
	int subsystemCount;
	int mySubsystemId;
	int myNodeId;

	JausNode findNode(JausNode node);
	JausNode findNode(JausAddress address);
	JausNode findNode(int subsId, int nodeId);

	JausComponent findComponent(JausComponent cmpt);
	JausComponent findComponent(JausAddress address);
	JausComponent findComponent(int subsId, int nodeId, int cmptId, int instId);
	void handleEvent(NodeManagerEvent *e);
};

#endif
