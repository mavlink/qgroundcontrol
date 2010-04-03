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
// File Name: JausCommunicationManager.cpp
//
// Written By: Danny Kent (jaus AT dannykent DOT com) 
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file defines a JausCommunicationManager interface.
// 				This class is a virtual class which supports the common fuctionality of 
//				comminication managers.

#include "nodeManager/JausCommunicationManager.h"
#include "nodeManager/JausTransportInterface.h"

JausCommunicationManager::JausCommunicationManager(void) {}

JausCommunicationManager::~JausCommunicationManager(void) {}

unsigned long JausCommunicationManager::getInterfaceCount(void)
{
	return (unsigned long) this->interfaces.size();
}

JausTransportInterface *JausCommunicationManager::getJausInterface(unsigned long index)
{
	if(index < this->interfaces.size())
	{
		return this->interfaces.at(index);
	}
	else
	{
		return NULL;
	}
}

JausTransportInterface *JausCommunicationManager::getJausInterface(std::string interfaceName)
{
	for(unsigned long i = 0; i < this->interfaces.size(); i++)
	{
		if(this->interfaces.at(i)->getName() == interfaceName)
		{
			return this->interfaces.at(i);
		}
	}
	return NULL;
}

bool JausCommunicationManager::interfaceExists(std::string interfaceName)
{
	for(unsigned long i = 0; i < this->interfaces.size(); i++)
	{
		if(this->interfaces.at(i)->getName() == interfaceName)
		{
			return true;
		}
	}
	return false;
}

void JausCommunicationManager::enable(void)
{
	enabled = true;
}

void JausCommunicationManager::disable(void)
{
	enabled = false;
}

bool JausCommunicationManager::isEnabled(void)
{
	return enabled;
}

SystemTree *JausCommunicationManager::getSystemTree()
{
	return this->systemTree;
}

MessageRouter *JausCommunicationManager::getMessageRouter()
{
	return this->msgRouter;
}


