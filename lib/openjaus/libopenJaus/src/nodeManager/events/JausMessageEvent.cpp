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
// File Name: JausMessageEvent.cpp
//
// Written By: Danny Kent (jaus AT dannykent DOT com) 
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file defines the interface of a JausMessageEvent, 
// 				which is derived from a the NodeManagerEvent class

#include "nodeManager/events/JausMessageEvent.h"

JausMessageEvent::JausMessageEvent(JausMessage message, JausTransportInterface *transport, unsigned char direction)
{
	this->type = NodeManagerEvent::JausMessageEvent;
	this->message = message;
	this->transport = transport;
	this->direction = direction;
}

JausMessageEvent::~JausMessageEvent()
{
	if(message != NULL)
	{
		jausMessageDestroy(message);
	}
}

JausMessageEvent *JausMessageEvent::cloneEvent()
{
	return new JausMessageEvent(jausMessageClone(this->message), this->transport, this->direction);
}

JausMessage JausMessageEvent::getJausMessage()
{
	return this->message;
}

JausTransportInterface *JausMessageEvent::getJausTransportInterface()
{
	return this->transport;
}

unsigned char JausMessageEvent::getMessageDirection()
{
	return this->direction;
}

std::string JausMessageEvent::toString()
{
	char buf[1024] = {0};
	char sourceString[80] = {0};
	char destinationString[80] = {0};
	
	jausAddressToString(this->message->source, sourceString);
	jausAddressToString(this->message->destination, destinationString);
	
	if(direction == JausMessageEvent::Inbound)
	{
		sprintf(buf, "RECEIVED: %s from %s to %s on interface: %s", jausMessageCommandCodeString(this->message), sourceString, destinationString, this->transport->toString().c_str());
		return buf;	
	}
	else
	{
		sprintf(buf, "SENDING: %s from %s to %s on interface: %s", jausMessageCommandCodeString(this->message), sourceString, destinationString, this->transport->toString().c_str());		
		return buf;
	}
}


