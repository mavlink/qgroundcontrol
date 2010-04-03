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
// File Name: OjUdpComponentInterface.h
//
// Written By: Danny Kent (jaus AT dannykent DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: 

#ifndef OJ_UDP_COMPONENT_INTERFACE_H
#define OJ_UDP_COMPONENT_INTERFACE_H

#if defined(WIN32)
	#include <hash_map>
	#define HASH_MAP stdext::hash_map
#elif defined(__GNUC__)
	#include <ext/hash_map>
	#define HASH_MAP __gnu_cxx::hash_map
#else
	#error "Hash Map undefined in SystemTable.h."
#endif

#include "JausTransportInterface.h"
#include "JausComponentCommunicationManager.h"
#include "SystemTree.h"
#include "NodeManagerComponent.h"
#include "utils/datagramSocket.h"

#define OJ_UDP_INTERFACE_MESSAGE_SIZE_BYTES	8
#define OJ_UDP_DEFAULT_PORT					24627 // Per OJ Nodemanager Interface Document
#define OJ_UDP_DEFAULT_TIMEOUT				1.0f

static const std::string OJ_UDP_DEFAULT_COMPONENT_IP = "127.0.0.1"; // Per OJ Nodemanager Interface Document

class OjUdpComponentInterface : public JausTransportInterface
{
public:
	OjUdpComponentInterface(FileLoader *configData, EventHandler *handler, JausComponentCommunicationManager *commMngr);
	~OjUdpComponentInterface(void);

	unsigned int getPort(void);
	bool processMessage(JausMessage message);
	std::string toString();
	bool startInterface(void);
	bool stopInterface(void);

private:
	NodeManagerComponent *nodeManager;
	SystemTree *systemTree;
	DatagramSocket socket;
	InetAddress ipAddress;
	unsigned short portNumber;
	HASH_MAP<int, unsigned short> portMap;

	bool sendDatagramPacket(DatagramPacket dgPacket);
	bool openSocket(void);
	void closeSocket(void);
	void run();

	bool processOjNodemanagerInterfaceMessage(char *buffer);

	// Message type codes
	enum
	{
		CHECK_IN								= 0x01,
		REPORT_ADDRESS							= 0x02,
		CHECK_OUT								= 0x03,
		VERIFY_ADDRESS							= 0x04,
		ADDRESS_VERIFIED						= 0x05,
		GET_COMPONENT_ADDRESS_LIST				= 0x06,
		COMPONENT_ADDRESS_LIST_RESPONSE			= 0x07,
		LOOKUP_ADDRESS							= 0x08,
		LOOKUP_ADDRESS_RESPONSE					= 0x09,
		LOOKUP_SERVICE_ADDRESS					= 0x0A,
		LOOKUP_SERVICE_ADDRESS_RESPONSE			= 0x0B,
		LOOKUP_SERVICE_ADDRESS_LIST				= 0x0C,
		LOOKUP_SERVICE_ADDRESS_LIST_RESPONSE	= 0x0D 
	};
};

#endif
