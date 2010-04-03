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
// File Name: JausSubsystemCommunicationManager.cpp
//
// Written By: Danny Kent (jaus AT dannykent DOT com) 
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file defines a JausSubsystemCommunicationManager.cpp class.
// 				This is derived from a the JausCommunicationManager class and supports
//				different subsystem interfaces.

#include "nodeManager/JausSubsystemCommunicationManager.h"
#include "nodeManager/JausOpcUdpInterface.h"
#include "nodeManager/JudpInterface.h"
#include "nodeManager/events/ErrorEvent.h"
#include "nodeManager/events/ConfigurationEvent.h"

JausSubsystemCommunicationManager::JausSubsystemCommunicationManager(FileLoader *configData, MessageRouter *msgRouter, SystemTree *systemTree, EventHandler *handler)
{
	this->systemTree = systemTree;
	this->msgRouter = msgRouter;
	this->configData = configData;
	this->eventHandler = handler;

	// NOTE: These two values should exist in the properties file and should be checked 
	// in the NodeManager class prior to constructing this object
	mySubsystemId = configData->GetConfigDataInt("JAUS", "SubsystemId");
	if(mySubsystemId < JAUS_MINIMUM_SUBSYSTEM_ID || mySubsystemId > JAUS_MAXIMUM_SUBSYSTEM_ID)
	{
		// Invalid ID
		throw "JausSubsystemCommunicationManager: Config file [JAUS] SubsystemId is invalid\n";
		mySubsystemId = JAUS_INVALID_SUBSYSTEM_ID;
		return;
	}

	myNodeId = configData->GetConfigDataInt("JAUS", "NodeId");
	if(myNodeId < JAUS_MINIMUM_NODE_ID || myNodeId > JAUS_MAXIMUM_NODE_ID)
	{
		// Invalid ID
		throw "JausSubsystemCommunicationManager: Config file [JAUS] NodeId is invalid\n";
		myNodeId= JAUS_INVALID_NODE_ID;
		return;
	}

	if( configData->GetConfigDataBool("Subsystem_Communications", "Enabled"))
	{
		ConfigurationEvent *e = new ConfigurationEvent(__FUNCTION__, __LINE__, "Starting Subsystem Interfaces");
		this->eventHandler->handleEvent(e);

		// Start subsystem interface(s)
		if(configData->GetConfigDataBool("Subsystem_Communications", "JAUS_OPC_UDP_Interface"))
		{
			JausOpcUdpInterface *etgUdpInterface = new JausOpcUdpInterface(configData, handler, this);
			this->interfaces.push_back(etgUdpInterface);

			char buf[128] = {0};
			sprintf(buf, "Opened Subsystem Interface:\t%s", etgUdpInterface->toString().c_str());
			ConfigurationEvent *e = new ConfigurationEvent(__FUNCTION__, __LINE__, buf);
			this->eventHandler->handleEvent(e);
		}

		if(configData->GetConfigDataBool("Subsystem_Communications", "JUDP_Interface"))
		{
			JudpInterface *judpInterface = new JudpInterface(configData, this->eventHandler, this);
			this->interfaces.push_back(judpInterface);

			char buf[128] = {0};
			sprintf(buf, "Opened Subsystem Interface:\t%s", judpInterface->toString().c_str());
			ConfigurationEvent *e = new ConfigurationEvent(__FUNCTION__, __LINE__, buf);
			this->eventHandler->handleEvent(e);
		}

		if( this->interfaces.size() > 0)
		{
			this->enabled = true;
		}
		else
		{
			this->enabled = false;
		}
	}
	else
	{
		this->enabled = false;
	}

}

JausSubsystemCommunicationManager::~JausSubsystemCommunicationManager(void)
{
	std::vector<JausTransportInterface *>::iterator iterator;
	for(iterator = interfaces.begin(); iterator != interfaces.end(); iterator++) 
	{
		delete *iterator;
	}
}

bool JausSubsystemCommunicationManager::startInterfaces(void)
{
	bool retVal = true;
	std::vector <JausTransportInterface *>::iterator iter;
	for(iter = interfaces.begin(); iter != interfaces.end(); iter++)
	{
		retVal = retVal && (*iter)->startInterface();
	}
	return retVal;
}

bool JausSubsystemCommunicationManager::stopInterfaces(void)
{
	bool retVal = true;
	std::vector <JausTransportInterface *>::iterator iter;
	for(iter = interfaces.begin(); iter != interfaces.end(); iter++)
	{
		retVal = retVal && (*iter)->stopInterface();
	}

	return retVal;
}

bool JausSubsystemCommunicationManager::sendJausMessage(JausMessage message)
{
	if(!this->enabled)
	{
		// This Communication Manager is turned off
		// Destroy this message
		jausMessageDestroy(message);
		return false;
	}

	// This conforms to the SubsCommMngr MsgRouter Source Routing Table v2.0
	if(!message)
	{
		// Error: Invalid message
		ErrorEvent *e = new ErrorEvent(ErrorEvent::NullPointer, __FUNCTION__, __LINE__, "Invalid JausMessage.");
		this->eventHandler->handleEvent(e);
		return false;
	}
	
	// Check for Errors
	if(message->source->subsystem != mySubsystemId)
	{
		// ERROR: Message from another Subs coming in from MsgRouter
		ErrorEvent *e = new ErrorEvent(ErrorEvent::Routing, __FUNCTION__, __LINE__, "Message from another Subs coming in from MsgRouter");
		this->eventHandler->handleEvent(e);
		jausMessageDestroy(message);
		return false;
	}

	if(message->destination->subsystem == mySubsystemId)
	{
		// ERROR: Message for this Subs coming in from MsgRouter
		ErrorEvent *e = new ErrorEvent(ErrorEvent::Routing, __FUNCTION__, __LINE__, "Message for this Subs coming in from MsgRouter");
		this->eventHandler->handleEvent(e);
		jausMessageDestroy(message);
		return false;
	}

	if(message->destination->subsystem == JAUS_BROADCAST_SUBSYSTEM_ID)
	{
		return sendToAllInterfaces(message);
	}
	else
	{
		return sendToSubsystemX(message);
	}
}

bool JausSubsystemCommunicationManager::receiveJausMessage(JausMessage message, JausTransportInterface *srcInf)
{
	if(!this->enabled)
	{
		// This Communication Manager is turned off
		// Destroy this message
		jausMessageDestroy(message);
		return false;
	}

	// This conforms to the SubsCommMngr MsgRouter Source Routing Table v2.0
	if(!message)
	{
		// Error: Invalid message
		ErrorEvent *e = new ErrorEvent(ErrorEvent::NullPointer, __FUNCTION__, __LINE__, "Invalid JausMessage.");
		this->eventHandler->handleEvent(e);
		return false;
	}

	if(!srcInf)
	{
		// Error: Invalid interface
		ErrorEvent *e = new ErrorEvent(ErrorEvent::NullPointer, __FUNCTION__, __LINE__, "srcInf is invalid");
		this->eventHandler->handleEvent(e);
		jausMessageDestroy(message);
		return false;
	}

	if(message->source->subsystem == mySubsystemId)
	{
		// Error: Cannot receive messages for myself through a subsInterface!
		ErrorEvent *e = new ErrorEvent(ErrorEvent::Routing, __FUNCTION__, __LINE__, "Message received from MsgRouter that is not from this subs is for this node");
		this->eventHandler->handleEvent(e);
		jausMessageDestroy(message);
		return false;
	}

	// Ok, this is a valid source. Add/Update its interface on the map
	interfaceMap[message->source->subsystem] = srcInf;

	if(	message->destination->subsystem == mySubsystemId ||
		message->destination->subsystem == JAUS_BROADCAST_SUBSYSTEM_ID)
	{
		msgRouter->routeSubsystemSourceMessage(message);
		return true;
	}
	else
	{
		// Error: Somehow I received a message intended for another subsystem
		ErrorEvent *e = new ErrorEvent(ErrorEvent::Routing, __FUNCTION__, __LINE__, "Message received on my Subsystem Interface for another subsystem ID!");
		this->eventHandler->handleEvent(e);
		jausMessageDestroy(message);
		return false;
	}
}

bool JausSubsystemCommunicationManager::sendToSubsystemX(JausMessage message)
{
	if(!message)
	{
		// Error: Invalid message
		ErrorEvent *e = new ErrorEvent(ErrorEvent::NullPointer, __FUNCTION__, __LINE__, "Invalid JausMessage.");
		this->eventHandler->handleEvent(e);
		return false;
	}

	// Route to subsystem X
	JausTransportInterface *jtInf = interfaceMap[message->destination->subsystem];
	if(jtInf)
	{
		jtInf->queueJausMessage(message);
		return true;
	}
	else
	{
		// I don't know how to send to Subsystem X
		jausMessageDestroy(message);
		return false;
	}
}

bool JausSubsystemCommunicationManager::sendToAllInterfaces(JausMessage message)
{
	std::vector <JausTransportInterface *>::iterator iter;

	if(!message)
	{
		// Error: Invalid message
		ErrorEvent *e = new ErrorEvent(ErrorEvent::NullPointer, __FUNCTION__, __LINE__, "Invalid JausMessage.");
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
