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
// File Name: JausComponentCommunicationManager.cpp
//
// Written By: Danny Kent (jaus AT dannykent DOT com) 
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file defines a JausComponentCommunicationManager.cpp class.
// 				This is derived from a the JausCommunicationManager class and supports
//				different component interfaces.

#include "nodeManager/JausComponentCommunicationManager.h"
#include "nodeManager/NodeManagerComponent.h"
#include "nodeManager/CommunicatorComponent.h"
#include "nodeManager/JausOpcUdpInterface.h"
#include "nodeManager/JudpInterface.h"
#include "nodeManager/OjUdpComponentInterface.h"
#include "nodeManager/events/ErrorEvent.h"
#include "nodeManager/events/ConfigurationEvent.h"

JausComponentCommunicationManager::JausComponentCommunicationManager(FileLoader *configData, MessageRouter *msgRouter, SystemTree *systemTree, EventHandler *handler)
{
	this->enabled = true;
	this->msgRouter = msgRouter;
	this->systemTree = systemTree;
	this->configData = configData;
	this->eventHandler = handler;
	this->interfaces.empty();
	this->interfaceMap.empty();

	// NOTE: These two values should exist in the properties file and should be checked 
	// in the NodeManager class prior to constructing this object
	mySubsystemId = configData->GetConfigDataInt("JAUS", "SubsystemId");
	if(mySubsystemId < JAUS_MINIMUM_SUBSYSTEM_ID || mySubsystemId > JAUS_MAXIMUM_SUBSYSTEM_ID)
	{
		// Invalid ID
		throw "JausComponentCommunicationManager: Invalid Subsystem ID";
		mySubsystemId = JAUS_INVALID_SUBSYSTEM_ID;
		return;
	}

	myNodeId = configData->GetConfigDataInt("JAUS", "NodeId");
	if(myNodeId < JAUS_MINIMUM_NODE_ID || myNodeId > JAUS_MAXIMUM_NODE_ID)
	{
		// Invalid ID
		throw "JausComponentCommunicationManager: Invalid Node ID";
		myNodeId= JAUS_INVALID_NODE_ID;
		return;
	}

	ConfigurationEvent *e = new ConfigurationEvent(__FUNCTION__, __LINE__, "Starting Component Interfaces");
	this->eventHandler->handleEvent(e);

	// Start Local Components
	this->nodeManagerCmpt = new NodeManagerComponent(this->configData, this->eventHandler, this);
	this->interfaces.push_back(nodeManagerCmpt);
	
	// Only start the Communicator if the SubsystemCommunications is enabled
	if( configData->GetConfigDataBool("Subsystem_Communications", "Enabled"))
	{
		this->communicatorCmpt = new CommunicatorComponent(this->configData, this->eventHandler, this);
		this->interfaces.push_back(communicatorCmpt);
	}
	else
	{
		this->communicatorCmpt = NULL;
	}

	// Start component interface(s)
	if(configData->GetConfigDataBool("Component_Communications", "JAUS_OPC_UDP_Interface"))
	{
		JausOpcUdpInterface *etgUdpInterface = new JausOpcUdpInterface(configData, this->eventHandler, this);
		this->interfaces.push_back(etgUdpInterface);

		char buf[128] = {0};
		sprintf(buf, "Opened Component Interface:\t%s", etgUdpInterface->toString().c_str());
		ConfigurationEvent *e = new ConfigurationEvent(__FUNCTION__, __LINE__, buf);
		this->eventHandler->handleEvent(e);
	}

	if(configData->GetConfigDataBool("Component_Communications", "JUDP_Interface"))
	{
		JudpInterface *judpInterface = new JudpInterface(configData, this->eventHandler, this);
		this->interfaces.push_back(judpInterface);

		char buf[128] = {0};
		sprintf(buf, "Opened Component Interface:\t%s", judpInterface->toString().c_str());
		ConfigurationEvent *e = new ConfigurationEvent(__FUNCTION__, __LINE__, buf);
		this->eventHandler->handleEvent(e);
	}

	if(configData->GetConfigDataBool("Component_Communications", "OpenJAUS_UDP_Interface"))
	{
		this->udpCmptInf = new OjUdpComponentInterface(configData, this->eventHandler, this);

		char buf[128] = {0};
		sprintf(buf, "Opened Component Interface:\t%s", udpCmptInf->toString().c_str());
		ConfigurationEvent *e = new ConfigurationEvent(__FUNCTION__, __LINE__, buf);
		this->eventHandler->handleEvent(e);
	}
	else
	{
		this->udpCmptInf = NULL;
	}


	if(!this->udpCmptInf)
	{
		// Error, must have some CmptInterface
		throw "JausComponentCommunicationManager: No Component Interface defined";
	}
}

JausComponentCommunicationManager::~JausComponentCommunicationManager(void)
{
	std::vector<JausTransportInterface *>::iterator iterator;
	for(iterator = interfaces.begin(); iterator != interfaces.end(); iterator++) 
	{
		delete *iterator;
	}
	
	if(udpCmptInf) delete udpCmptInf;
}

bool JausComponentCommunicationManager::startInterfaces(void)
{
	bool retVal = true;
	std::vector <JausTransportInterface *>::iterator iter;
	for(iter = interfaces.begin(); iter != interfaces.end(); iter++)
	{
		retVal = retVal && (*iter)->startInterface();
	}

	if(udpCmptInf) this->udpCmptInf->startInterface();

	return retVal;
}

bool JausComponentCommunicationManager::stopInterfaces(void)
{
	bool retVal = true;
	std::vector <JausTransportInterface *>::iterator iter;
	for(iter = interfaces.begin(); iter != interfaces.end(); iter++)
	{
		retVal = retVal && (*iter)->stopInterface();
	}

	if(udpCmptInf) this->udpCmptInf->stopInterface();

	return retVal;
}

bool JausComponentCommunicationManager::sendJausMessage(JausMessage message)
{
	// This conforms to the CmptCommMngr MsgRouter Source Routing Table v2.0
	if(!message)
	{
		// Error: Invalid message
		ErrorEvent *e = new ErrorEvent(ErrorEvent::NullPointer, __FUNCTION__, __LINE__, "Invalid message pointer");
		this->eventHandler->handleEvent(e);
		return false;
	}

	// Check for Errors
	if( message->destination->subsystem != mySubsystemId &&
		message->destination->subsystem != JAUS_BROADCAST_SUBSYSTEM_ID )
	{
		// ERROR: Message not for this subsystem has been routed from the MsgRouter
		ErrorEvent *e = new ErrorEvent(ErrorEvent::Routing, __FUNCTION__, __LINE__, "Message not for this subsystem has been routed from the MsgRouter");
		this->eventHandler->handleEvent(e);
		jausMessageDestroy(message);
		return false;
	}

	if(message->source->subsystem == mySubsystemId && message->source->node == myNodeId)
	{
		// ERROR: Message from this node has been routed from the MsgRouter
		ErrorEvent *e = new ErrorEvent(ErrorEvent::Routing, __FUNCTION__, __LINE__, "Message from this node has been routed from the MsgRouter");
		this->eventHandler->handleEvent(e);
		jausMessageDestroy(message);
		return false;
	}

	if( message->destination->subsystem == mySubsystemId ||
		message->destination->subsystem == JAUS_BROADCAST_SUBSYSTEM_ID)
	{
		if( message->destination->component == JAUS_BROADCAST_COMPONENT_ID ||
			message->destination->instance == JAUS_BROADCAST_INSTANCE_ID)
		{
			return sendToAllInterfaces(message);
		}
		else
		{
			return sendToComponentX(message);
		}
	}
	else
	{
		// ERROR: Message not for components on this node received
		ErrorEvent *e = new ErrorEvent(ErrorEvent::Routing, __FUNCTION__, __LINE__, "Message not for components on this node received");
		this->eventHandler->handleEvent(e);
		jausMessageDestroy(message);
		return false;
	}
}

bool JausComponentCommunicationManager::receiveJausMessage(JausMessage message, JausTransportInterface *srcInf)
{
	// This conforms to the CmptCommMngr MsgRouter Source Routing Table v2.0
	if(!message)
	{
		// Error: Invalid message
		ErrorEvent *e = new ErrorEvent(ErrorEvent::NullPointer, __FUNCTION__, __LINE__, "Invalid message pointer");
		this->eventHandler->handleEvent(e);
		return false;
	}

	if(message->source->subsystem != mySubsystemId)
	{
		// ERROR: Received a message from a component that is not from my subsystem!
		ErrorEvent *e = new ErrorEvent(ErrorEvent::Routing, __FUNCTION__, __LINE__, "Received a message from a component that is not from my subsystem!");
		this->eventHandler->handleEvent(e);
		jausMessageDestroy(message);
		return false;
	}

	if(message->source->node != myNodeId)
	{
		// ERROR: Received a message from a component with a source node that is not my node ID!
		ErrorEvent *e = new ErrorEvent(ErrorEvent::Routing, __FUNCTION__, __LINE__, "Received a message from a component with a source node that is not my node ID!");
		this->eventHandler->handleEvent(e);
		jausMessageDestroy(message);
		return false;
	}

	// Add this source to our interface table
	interfaceMap[jausAddressHash(message->source)] = srcInf;

	if(message->destination->subsystem == mySubsystemId)
	{
		if(message->destination->node == myNodeId)
		{
			if(message->destination->component == JAUS_BROADCAST_COMPONENT_ID)
			{
				return sendToAllInterfaces(message);
			}
			else
			{
				return sendToComponentX(message);
			}
		}
		else
		{
			// Send to MsgRouter
			msgRouter->routeComponentSourceMessage(jausMessageClone(message));
			
			if(message->destination->component == JAUS_BROADCAST_COMPONENT_ID)
			{
				return sendToAllInterfaces(message);
			}
			else
			{
				return sendToComponentX(message);
			}
		}
	}
	else if(message->destination->subsystem == JAUS_BROADCAST_SUBSYSTEM_ID)
	{
		msgRouter->routeComponentSourceMessage(jausMessageClone(message));

		if(message->destination->component == JAUS_BROADCAST_COMPONENT_ID)
		{
			return sendToAllInterfaces(message);
		}
		else
		{
			return sendToComponentX(message);
		}
	}
	else
	{
		msgRouter->routeComponentSourceMessage(message);
	}

	return false;
}

NodeManagerComponent *JausComponentCommunicationManager::getNodeManagerComponent(void)
{
	return this->nodeManagerCmpt;
}

bool JausComponentCommunicationManager::sendToComponentX(JausMessage message)
{
	HASH_MAP<int, JausTransportInterface *>::iterator iter;

	if(!message)
	{
		// Error: Invalid message
		ErrorEvent *e = new ErrorEvent(ErrorEvent::NullPointer, __FUNCTION__, __LINE__, "Invalid message pointer");
		this->eventHandler->handleEvent(e);
		return false;
	}

	if(	message->destination->subsystem == JAUS_BROADCAST_SUBSYSTEM_ID ||
		message->destination->node == JAUS_BROADCAST_NODE_ID)
	{
		// This is ensure that the jausAddressHash works incase either the Subs or Node IDs are BROADCAST
		// Assume if we have gotten here this message is actually for our Subs & Node
		JausAddress lookupAddress = jausAddressClone(message->destination);
		lookupAddress->subsystem = mySubsystemId;
		lookupAddress->node = myNodeId;

		iter = interfaceMap.find(jausAddressHash(lookupAddress));
		jausAddressDestroy(lookupAddress);
	}
	else
	{
		iter = interfaceMap.find(jausAddressHash(message->destination));
	}
	
	if(iter != interfaceMap.end())
	{
		iter->second->queueJausMessage(message);
		return true;
	}
	else
	{
		// I don't know how to send something to that node
		jausMessageDestroy(message);
		return false;
	}
}

bool JausComponentCommunicationManager::sendToAllInterfaces(JausMessage message)
{
	std::vector <JausTransportInterface *>::iterator iter;

	if(!message)
	{
		// Error: Invalid message
		ErrorEvent *e = new ErrorEvent(ErrorEvent::NullPointer, __FUNCTION__, __LINE__, "Invalid message pointer");
		this->eventHandler->handleEvent(e);
		return false;
	}

	for(iter = interfaces.begin(); iter != interfaces.end(); iter++)
	{
		(*iter)->queueJausMessage(jausMessageClone(message));
	}
	jausMessageDestroy(message);
	return true;
}

