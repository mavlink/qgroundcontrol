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
// File Name: JudpInterface.h
//
// Written By: Danny Kent (jaus AT dannykent DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file lists the functions associated with a UDP Jaus 
//              Transport interface. This interface confirms to the JUDP 
//              interface (with header compression) described in SAE 
//				document AS5669.

#ifndef JUDP_INTERFACE_H
#define JUDP_INTERFACE_H

#ifdef WIN32
	#include <errno.h>
	#include <hash_map>
	#define HASH_MAP stdext::hash_map
#elif defined(__GNUC__)
	#include <ext/hash_map>
	#define HASH_MAP __gnu_cxx::hash_map
#endif

#include "JausTransportInterface.h"
#include "utils/multicastSocket.h"
#include "utils/inetAddress.h"
#include "utils/datagramPacket.h"
#include "utils/FileLoader.h"

#define JUDP_NAME								"JUDP Interface"
#define JUDP_DATA_PORT							3794 // per AS5669 v1.0 and IANA assignment
#define JUDP_PER_PACKET_HEADER_SIZE_BYTES		1
#define JUDP_PER_MESSAGE_HEADER_SIZE_BYTES		4
#define JUDP_VERSION_NUMBER						1 // per AS5669 v1.0
#define JUDP_MAX_PACKET_SIZE					4101 // per AS5669 v1.0

// Header Compression Flag Values
#define JUDP_HC_NO_COMPRESSION			0
#define JUDP_HC_ENGAGE_COMPRESSION		1
#define JUDP_HC_COMPRESSION_ACKNOWLEDGE	2 // Note: This value has multiple interpretations, depending on the message length field
#define JUDP_HC_COMPRESSED_MESSAGE		3

// Default Configuration Values
// Component UDP Interface Default Values
#define JUDP_DEFAULT_COMPONENT_UDP_PORT				24629 // Per OpenJAUS Node Manager Interface document
#define JUDP_DEFAULT_COMPONENT_TTL					1
#define JUDP_DEFAULT_COMPONENT_UDP_TIMEOUT_SEC		1.0f
#define JUDP_DEFAULT_COMPONENT_MULTICAST			false
#define JUDP_DEFAULT_COMPONENT_HEADER_COMPRESSION	false

// Node UDP Interface Default Values
#define JUDP_DEFAULT_NODE_TTL						16 // per AS5669
#define JUDP_DEFAULT_NODE_UDP_TIMEOUT_SEC			1.0f
#define JUDP_DEFAULT_NODE_MULTICAST					true
#define JUDP_DEFAULT_NODE_HEADER_COMPRESSION		false

// Subsystem UDP Interface Default Values
#define JUDP_DEFAULT_SUBSYSTEM_TTL					16 // per AS5669
#define JUDP_DEFAULT_SUBSYSTEM_UDP_TIMEOUT_SEC		1.0f
#define JUDP_DEFAULT_SUBSYSTEM_MULTICAST			true
#define JUDP_DEFAULT_SUBSYSTEM_HEADER_COMPRESSION	false

static const std::string JUDP_DEFAULT_COMPONENT_IP = "127.0.0.1"; // Per OpenJAUS Node Manager Interface document
static const std::string JUDP_DEFAULT_SUBSYSTEM_MULTICAST_GROUP = "224.1.0.1"; // per AS5669
static const std::string JUDP_DEFAULT_NODE_MULTICAST_GROUP = "225.1.0.1"; // per AS5669 with slight modification

extern "C" void *JudpRecvThread(void *);

// Transport Data Structure
typedef struct
{
	unsigned int addressValue;
	unsigned short port;
}JudpTransportData;

typedef struct
{
	unsigned char headerNumber;
	unsigned char length;
	unsigned char flags;
	unsigned short messageLength;
}JudpHeaderCompressionData;

class JudpInterface : public JausTransportInterface
{
public:
	JudpInterface(FileLoader *configData, EventHandler *handler, JausCommunicationManager *commMngr);
	~JudpInterface(void);

	InetAddress getInetAddress(void);

	bool processMessage(JausMessage message);

	bool startInterface();
	bool stopInterface();
	std::string toString();
	void run();
	void recvThreadRun();

private:
	MulticastSocket socket;
	bool openSocket(void);
	void closeSocket(void);

	bool multicast;
	InetAddress ipAddress;
	InetAddress multicastGroup;
	unsigned short portNumber;
	bool supportHeaderCompression;

	int recvThreadId;
	pthread_t recvThread;
	pthread_attr_t recvThreadAttr;

	void sendJausMessage(JudpTransportData data, JausMessage message);
	void startRecvThread();
	void stopRecvThread();

	unsigned int headerCompressionDataToBuffer(JudpHeaderCompressionData *hcData, unsigned char *buffer, unsigned int bufferSizeBytes);
	unsigned int headerCompressionDataFromBuffer(JudpHeaderCompressionData *hcData, unsigned char *buffer, unsigned int bufferSizeBytes);
	void sendUncompressedMessage(JudpTransportData data, JausMessage message);
	void sendCompressedMessage(JudpTransportData data, JausMessage message);
	bool receiveUncompressedMessage(JausMessage rxMessage, unsigned char *buffer, unsigned int bufferSizeBytes);
	bool receiveCompressedMessage(JausMessage rxMessage, JudpHeaderCompressionData *hcData, unsigned char *buffer, unsigned int bufferSizeBytes);


	HASH_MAP <int, JudpTransportData> addressMap;
	bool subsystemGatewayDiscovered;
	JudpTransportData subsystemGatewayData;
	JudpTransportData multicastData;
};

#endif
