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
// File Name: NodeManager.cpp
//
// Written By: Danny Kent (jaus AT dannykent DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: Wrapper class for the Node Manager as a whole. Provides user interface to create and run a Node Manager.

#include <cstdlib>
#include "nodeManager/NodeManager.h"


NodeManager::NodeManager(FileLoader *configData, EventHandler *handler)
{
	this->registerEventHandler(handler);
	
	// Read subsystem id config
	int	subsystemId = configData->GetConfigDataInt("JAUS", "SubsystemId");
	if(subsystemId < JAUS_MINIMUM_SUBSYSTEM_ID || subsystemId > JAUS_MAXIMUM_SUBSYSTEM_ID)
	{
		throw "NodeManager: Config file [JAUS] SubsystemId is invalid\n";
	}

	// Read node id config
	int nodeId = configData->GetConfigDataInt("JAUS", "NodeId");
	if(nodeId < JAUS_MINIMUM_NODE_ID || nodeId > JAUS_MAXIMUM_NODE_ID)
	{
		throw "NodeManager: Config file [JAUS] NodeId is invalid\n";
	}

	// Create this subsystem
	this->subsystem = jausSubsystemCreate();
	if(!subsystem)
	{
		throw "NodeManager: Could not create subsystem\n";
	}

	// Create this node
	this->node = jausNodeCreate();
	if(!node)
	{
		throw "NodeManager: Could not create node\n";
	}

	this->subsystem->id = subsystemId;
	size_t identificationLength = strlen(configData->GetConfigDataString("JAUS", "Subsystem_Identification").c_str()) + 1;
	this->subsystem->identification = (char *) malloc(identificationLength);
	sprintf(this->subsystem->identification, configData->GetConfigDataString("JAUS", "Subsystem_Identification").c_str());

	this->node->id = nodeId;
	identificationLength = strlen(configData->GetConfigDataString("JAUS", "Node_Identification").c_str()) + 1;
	this->node->identification = (char *) malloc(identificationLength);
	sprintf(this->node->identification, configData->GetConfigDataString("JAUS", "Node_Identification").c_str());
	jausArrayAdd(this->subsystem->nodes, this->node);

	// TODO: Check our config file parameters

	// Initialize our eventHandler list
	eventHandlers.empty();

	// Create our systemTable and add our subsystem
	this->systemTree = new SystemTree(configData, this);
	this->systemTree->addSubsystem(subsystem);

	// Create our MsgRouter
	try
	{
		this->msgRouter = new MessageRouter(configData, systemTree, this);
	}
	catch(...)
	{
		jausSubsystemDestroy(subsystem);
		delete systemTree;
		throw;
	}
}

NodeManager::~NodeManager(void)
{
	jausSubsystemDestroy(subsystem);
	
	delete msgRouter;
	delete systemTree;
}

std::string NodeManager::systemTreeToString()
{
	return systemTree->toString();
}

std::string NodeManager::systemTreeToDetailedString()
{
	return systemTree->toDetailedString();
}

bool NodeManager::registerEventHandler(EventHandler *handler)
{
	if(handler)
	{
		this->eventHandlers.push_back(handler);
		return true;
	}
	return false;
}

void NodeManager::handleEvent(NodeManagerEvent *e)
{
	// Send to all registered handlers
	std::list <EventHandler *>::iterator iter;
	for(iter = eventHandlers.begin(); iter != eventHandlers.end(); iter++)
	{
		(*iter)->handleEvent(e->cloneEvent());
	}
	delete e;
}

