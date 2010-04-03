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
// File Name: SystemTreeEvent.cpp
//
// Written By: Danny Kent (jaus AT dannykent DOT com) 
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file defines the interface of a SystemTreeEvent, 
// 				which is derived from a the NodeManagerEvent class

#include "nodeManager/events/SystemTreeEvent.h"

SystemTreeEvent::SystemTreeEvent(unsigned int type)
{
	this->type = NodeManagerEvent::SystemTreeEvent;
	this->subType = type;
	this->subs = NULL;
	this->node = NULL;
	this->cmpt = NULL;
}

SystemTreeEvent::SystemTreeEvent(unsigned int type, JausSubsystem subs)
{
	this->type = NodeManagerEvent::SystemTreeEvent;
	this->subType = type;
	this->subs = jausSubsystemClone(subs);
	this->node = NULL;
	this->cmpt = NULL;
}

SystemTreeEvent::SystemTreeEvent(unsigned int type, JausNode node)
{
	this->type = NodeManagerEvent::SystemTreeEvent;
	this->subType = type;
	this->subs = NULL;
	this->node = jausNodeClone(node);
	this->cmpt = NULL;
}

SystemTreeEvent::SystemTreeEvent(unsigned int type, JausComponent cmpt)
{
	this->type = NodeManagerEvent::SystemTreeEvent;
	this->subType = type;
	this->subs = NULL;
	this->node = NULL;
	this->cmpt = jausComponentClone(cmpt);
}

SystemTreeEvent::~SystemTreeEvent()
{
	if(subs)
	{ 
		jausSubsystemDestroy(subs);
	}

	if(node)
	{
		jausNodeDestroy(node);
	}

	if(cmpt)
	{
		jausComponentDestroy(cmpt);
	}
}

SystemTreeEvent *SystemTreeEvent::cloneEvent()
{
	if(this->subs)
	{
		return new SystemTreeEvent(this->subType, this->subs);
	}
	else if(this->node)
	{
		return new SystemTreeEvent(this->subType, this->node);
	}
	else if(this->cmpt)
	{
		return new SystemTreeEvent(this->subType, this->cmpt);
	}
	else
	{
		return new SystemTreeEvent(this->subType);
	}
}

std::string SystemTreeEvent::toString()
{
	std::string output = "";
	char buf[128] = {0};

	switch(this->subType)
	{
		case SystemTreeEvent::SubsystemAdded:
			jausSubsystemToString(this->subs, buf);
			output += "Subsystem ADDED: ";
			output += buf;
			return output;

		case SystemTreeEvent::SubsystemRemoved:
			jausSubsystemToString(this->subs, buf);
			output += "Subsystem REMOVED: ";
			output += buf;
			return output;

		case SystemTreeEvent::SubsystemTimeout:
			jausSubsystemToString(this->subs, buf);
			output += "Subsystem TIMEOUT: ";
			output += buf;
			return output;

		case SystemTreeEvent::NodeAdded:
			jausNodeToString(this->node, buf);
			output += "Node ADDED: ";
			output += buf;
			return output;

		case SystemTreeEvent::NodeRemoved:
			jausNodeToString(this->node, buf);
			output += "Node REMOVED: ";
			output += buf;
			return output;
		
		case SystemTreeEvent::NodeTimeout:
			jausNodeToString(this->node, buf);
			output += "Node TIMEOUT: ";
			output += buf;
			return output;

		case SystemTreeEvent::ComponentAdded:
			jausComponentToString(this->cmpt, buf);
			output += "Component ADDED: ";
			output += buf;
			return output;

		case SystemTreeEvent::ComponentRemoved:
			jausComponentToString(this->cmpt, buf);
			output += "Component REMOVED: ";
			output += buf;
			return output;

		case SystemTreeEvent::ComponentTimeout:
			jausComponentToString(this->cmpt, buf);
			output += "Component TIMEOUT: ";
			output += buf;
			return output;
	
		default:
			output = "Unknown Subsystem Event";
			return output;
	}
}

unsigned int SystemTreeEvent::getSubType()
{
	return this->subType;
}

JausComponent SystemTreeEvent::getComponent()
{
	return this->cmpt;
}

JausNode SystemTreeEvent::getNode()
{
	return this->node;
}

JausSubsystem SystemTreeEvent::getSubsystem()
{
	return this->subs;
}

