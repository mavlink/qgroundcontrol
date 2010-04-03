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
// File Name: JudpInterface.cpp
//
// Written By: Danny Kent (jaus AT dannykent DOT com) 
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: Defines the standard JAUS UDP interface on port 3792. Is compliant with the 
// 				ETG/OPC style of UDP header

#include "nodeManager/JudpInterface.h"
#include "nodeManager/JausSubsystemCommunicationManager.h"
#include "nodeManager/JausNodeCommunicationManager.h"
#include "nodeManager/JausComponentCommunicationManager.h"
#include "nodeManager/events/ErrorEvent.h"
#include "nodeManager/events/JausMessageEvent.h"

JudpInterface::JudpInterface(FileLoader *configData, EventHandler *handler, JausCommunicationManager *commMngr)
{
	this->commMngr = commMngr;
	this->eventHandler = handler;
	this->name = JUDP_NAME;
	this->configData = configData;
	this->multicast = false;
	this->subsystemGatewayDiscovered = false;
	
	// Determine the type of our commMngr
	if(dynamic_cast<JausSubsystemCommunicationManager  *>(this->commMngr))
	{
		this->type = SUBSYSTEM_INTERFACE;
	}
	else if(dynamic_cast<JausNodeCommunicationManager *>(this->commMngr))
	{
		this->type = NODE_INTERFACE;
	}
	else if(dynamic_cast<JausComponentCommunicationManager *>(this->commMngr))
	{
		this->type = COMPONENT_INTERFACE;
	}
	else
	{
		this->type = UNKNOWN_INTERFACE;
	}


	// NOTE: This value should exist in the properties file and should be checked 
	// in the NodeManager class prior to constructing this object
	mySubsystemId = configData->GetConfigDataInt("JAUS", "SubsystemId");
	if(mySubsystemId < JAUS_MINIMUM_SUBSYSTEM_ID || mySubsystemId > JAUS_MAXIMUM_SUBSYSTEM_ID)
	{
		// Invalid ID
		// TODO: Throw an exception? Log an error.
		mySubsystemId = JAUS_INVALID_SUBSYSTEM_ID;
		return;
	}

	// Setup our UDP Socket
	if(!this->openSocket())
	{
		throw "JudpInterface: Could not open socket\n";
	}
}

bool JudpInterface::startInterface(void)
{
	// Set our thread running flag
	this->running = true;

	// Setup our pThread
	this->startThread();

	// Setup our receiveThread
	this->startRecvThread();

	return true;
}

bool JudpInterface::stopInterface(void)
{
	this->running = false;
	
	// Stop our pThread
	this->stopThread();

	// Stop our receiveThread
	this->stopRecvThread();

	return true;
}

JudpInterface::~JudpInterface(void)
{
	if(running)
	{
		this->stopInterface();
	}
	this->closeSocket();

	// TODO: Check our threadIds to see if they terminated properly
}

InetAddress JudpInterface::getInetAddress(void)
{
	return this->socket->address;
}

bool JudpInterface::processMessage(JausMessage message)
{
	switch(this->type)
	{
		case SUBSYSTEM_INTERFACE:
			// if subs==BROADCAST send multicast
			if(message->destination->subsystem == JAUS_BROADCAST_SUBSYSTEM_ID)
			{
				if(this->multicast)
				{
					// Send multicast packet
					sendJausMessage(this->multicastData, message);
					jausMessageDestroy(message);
					return true;
				}
				else
				{
					// Unicast to all known subsystems
					HASH_MAP<int, JudpTransportData>::iterator iter;
					for(iter = addressMap.begin(); iter != addressMap.end(); iter++)
					{
						sendJausMessage(iter->second, message);
					}
					jausMessageDestroy(message);
					return true;
				}
			}
			else
			{
				// Unicast
				if(addressMap.find(message->destination->subsystem) != addressMap.end())
				{
					sendJausMessage(addressMap.find(message->destination->subsystem)->second, message);
					jausMessageDestroy(message);
					return true;
				}
				else
				{
					// Don't know how to send this message
					jausMessageDestroy(message);
					return false;
				}
			}
			break;

		case NODE_INTERFACE:
			if(	message->destination->subsystem == mySubsystemId || 
				message->destination->subsystem == JAUS_BROADCAST_SUBSYSTEM_ID )
			{
				// if node==BROADCAST send multicast
				if(message->destination->node == JAUS_BROADCAST_NODE_ID)
				{
					if(this->multicast)
					{
						// Send multicast packet
						sendJausMessage(this->multicastData, message);
						jausMessageDestroy(message);
						return true;
					}
					else
					{
						// Unicast to all known nodes
						HASH_MAP<int, JudpTransportData>::iterator iter;
						for(iter = addressMap.begin(); iter != addressMap.end(); iter++)
						{
							sendJausMessage(iter->second, message);
						}
						jausMessageDestroy(message);
						return true;
					}
				}
				else
				{
					// Unicast
					if(addressMap.find(message->destination->node) != addressMap.end())
					{
						sendJausMessage(addressMap.find(message->destination->node)->second, message);
						jausMessageDestroy(message);
						return true;
					}
					else
					{
						// Don't know how to send this message
						jausMessageDestroy(message);
						return false;
					}
				}
			}
			else
			{
				// Message for other subsystem
				if(subsystemGatewayDiscovered)
				{
					sendJausMessage(subsystemGatewayData, message);
					jausMessageDestroy(message);
					return true;
				}
				else
				{
					// Don't know how to send this message
					jausMessageDestroy(message);
					return false;
				}
			}
			break;

		case COMPONENT_INTERFACE:
			// if cmpt == BROADCAST || inst == BROADCAST send unicast to all components
			if( message->destination->component == JAUS_BROADCAST_COMPONENT_ID ||
				message->destination->instance == JAUS_BROADCAST_INSTANCE_ID )
			{
				if(this->multicast)
				{
					// Send multicast packet
					sendJausMessage(this->multicastData, message);
					jausMessageDestroy(message);
					return true;
				}
				else
				{
					// Unicast to all known subsystems
					HASH_MAP<int, JudpTransportData>::iterator iter;
					for(iter = addressMap.begin(); iter != addressMap.end(); iter++)
					{
						sendJausMessage(iter->second, message);
					}
					jausMessageDestroy(message);
					return true;
				}
			}
			else
			{
				// Unicast
				if(addressMap.find(jausAddressHash(message->destination)) != addressMap.end())
				{
					sendJausMessage(addressMap.find(jausAddressHash(message->destination))->second, message);
					jausMessageDestroy(message);
					return true;
				}
				else
				{
					// Don't know how to send this message
					jausMessageDestroy(message);
					return false;
				}
			}
			break;

		default:
			// Unknown type
			// No routing behavior
			jausMessageDestroy(message);
			return false;
	}
}

void JudpInterface::run()
{
	// Lock our mutex
	pthread_mutex_lock(&threadMutex);

	while(this->running)
	{
		pthread_cond_wait(&threadConditional, &threadMutex);
		
		while(!this->queue.isEmpty())
		{
			// Pop a packet off the queue and send it off
			processMessage(queue.pop());
		}
	}
	pthread_mutex_unlock(&threadMutex);
}

std::string JudpInterface::toString()
{
	char ret[256] = {0};
	char buf[80] = {0};
	if(this->socket)
	{
		inetAddressToBuffer(this->socket->address, buf, 80);
		sprintf(ret, "%s %s:%d", JUDP_NAME, buf, this->socket->port);
		return ret;
	}
	else
	{
		sprintf(ret, "%s Invalid.", JUDP_NAME);
		return ret;
	}
}

bool JudpInterface::openSocket(void)
{
	double socketTimeoutSec = 0;
	unsigned char socketTTL = 0;
	std::string multicastGroupString;

	switch(this->type)
	{
		case SUBSYSTEM_INTERFACE:
			// Read Subsystem UDP Parameters
			// Port is constant per JAUS Standard
			this->portNumber = JUDP_DATA_PORT;

			// IP Address
			if(this->configData->GetConfigDataString("Subsystem_Communications", "JUDP_IP_Address") == "")
			{
				// Cannot open specified IP Address				
				ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, "No IP Address specified!");
				this->eventHandler->handleEvent(e);
				return false;
			}
			else
			{
				this->ipAddress = inetAddressGetByString((char *)this->configData->GetConfigDataString("Subsystem_Communications", "JUDP_IP_Address").c_str());
				if(this->ipAddress == NULL)
				{
					// Cannot open specified IP Address
					char errorString[128] = {0};
					sprintf(errorString, "Could not open specified IP Address: %s", this->configData->GetConfigDataString("Subsystem_Communications", "JUDP_IP_Address").c_str());
					
					ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, errorString);
					this->eventHandler->handleEvent(e);
					return false;
				}
			}

			// Timeout
			if(this->configData->GetConfigDataString("Subsystem_Communications", "JUDP_Timeout_Sec") == "")
			{
				socketTimeoutSec = JUDP_DEFAULT_SUBSYSTEM_UDP_TIMEOUT_SEC;
			}
			else
			{
				socketTimeoutSec = this->configData->GetConfigDataDouble("Subsystem_Communications", "JUDP_Timeout_Sec");
			}

			// TTL
			if(this->configData->GetConfigDataString("Subsystem_Communications", "JUDP_TTL") == "")
			{
				socketTTL = JUDP_DEFAULT_SUBSYSTEM_TTL;
			}
			else
			{
				socketTTL = this->configData->GetConfigDataInt("Subsystem_Communications", "JUDP_TTL");
			}

			// Multicast
			if(this->configData->GetConfigDataString("Subsystem_Communications", "JUDP_Multicast") == "")
			{
				this->multicast = JUDP_DEFAULT_SUBSYSTEM_MULTICAST;
			}
			else
			{
				this->multicast = this->configData->GetConfigDataBool("Subsystem_Communications", "JUDP_Multicast");
			}

			if(this->multicast)
			{
				// Multicast Group
				if(this->configData->GetConfigDataString("Subsystem_Communications", "JUDP_Multicast_Group") == "")
				{
					multicastGroupString = JUDP_DEFAULT_SUBSYSTEM_MULTICAST_GROUP;
				}
				else
				{
					multicastGroupString = this->configData->GetConfigDataString("Subsystem_Communications", "JUDP_Multicast_Group");
				}
			}

			// Header Compression
			if(this->configData->GetConfigDataString("Subsystem_Communications", "JUDP_Header_Compression") == "")
			{
				this->supportHeaderCompression = JUDP_DEFAULT_SUBSYSTEM_HEADER_COMPRESSION;
			}
			else
			{
				this->supportHeaderCompression = this->configData->GetConfigDataBool("Subsystem_Communications", "JUDP_Header_Compression");
			}
			break;

		case NODE_INTERFACE:
			// Setup Node Configuration
			// Port is constant per JAUS Standard
			this->portNumber = JUDP_DATA_PORT;

			// IP Address
			if(this->configData->GetConfigDataString("Node_Communications", "JUDP_IP_Address") == "")
			{
				// Cannot open specified IP Address				
				ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, "No IP Address specified!");
				this->eventHandler->handleEvent(e);
				return false;
			}
			else
			{
				this->ipAddress = inetAddressGetByString((char *)this->configData->GetConfigDataString("Node_Communications", "JUDP_IP_Address").c_str());
				if(this->ipAddress == NULL)
				{
					// Cannot open specified IP Address
					char errorString[128] = {0};
					sprintf(errorString, "Could not open specified IP Address: %s", this->configData->GetConfigDataString("Node_Communications", "JUDP_IP_Address").c_str());
					
					ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, errorString);
					this->eventHandler->handleEvent(e);
					return false;
				}
			}

			// Timeout
			if(this->configData->GetConfigDataString("Node_Communications", "JUDP_Timeout_Sec") == "")
			{
				socketTimeoutSec = JUDP_DEFAULT_NODE_UDP_TIMEOUT_SEC;
			}
			else
			{
				socketTimeoutSec = this->configData->GetConfigDataDouble("Node_Communications", "JUDP_Timeout_Sec");
			}

			// TTL
			if(this->configData->GetConfigDataString("Node_Communications", "JUDP_TTL") == "")
			{
				socketTTL = JUDP_DEFAULT_NODE_TTL;
			}
			else
			{
				socketTTL = this->configData->GetConfigDataInt("Node_Communications", "JUDP_TTL");
			}

			// Multicast
			if(this->configData->GetConfigDataString("Node_Communications", "JUDP_Multicast") == "")
			{
				this->multicast = JUDP_DEFAULT_NODE_MULTICAST;
			}
			else
			{
				this->multicast = this->configData->GetConfigDataBool("Node_Communications", "JUDP_Multicast");
			}

			if(this->multicast)
			{
				// Multicast Group
				if(this->configData->GetConfigDataString("Node_Communications", "JUDP_Multicast_Group") == "")
				{
					multicastGroupString = JUDP_DEFAULT_NODE_MULTICAST_GROUP;
				}
				else
				{
					multicastGroupString = this->configData->GetConfigDataString("Node_Communications", "JUDP_Multicast_Group");
				}
			}

			// Header Compression
			if(this->configData->GetConfigDataString("Node_Communications", "JUDP_Header_Compression") == "")
			{
				this->supportHeaderCompression = JUDP_DEFAULT_NODE_HEADER_COMPRESSION;
			}
			else
			{
				this->supportHeaderCompression = this->configData->GetConfigDataBool("Node_Communications", "JUDP_Header_Compression");
			}
			break;

		case COMPONENT_INTERFACE:
			// Read Component Configuration
			// Port Number
			if(this->configData->GetConfigDataString("Component_Communications", "JUDP_Port") == "")
			{
				this->portNumber = JUDP_DEFAULT_COMPONENT_UDP_PORT;
			}
			else
			{
				this->portNumber = this->configData->GetConfigDataInt("Component_Communications", "JUDP_Port");
			}

			// IP Address
			if(this->configData->GetConfigDataString("Component_Communications", "JUDP_IP_Address") == "")
			{
				this->ipAddress = inetAddressGetByString((char *)JUDP_DEFAULT_COMPONENT_IP.c_str());
				if(this->ipAddress == NULL)
				{
					// Cannot open specified IP Address
					char errorString[128] = {0};
					sprintf(errorString, "Could not open default IP Address: %s", JUDP_DEFAULT_COMPONENT_IP.c_str());
					
					ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, errorString);
					this->eventHandler->handleEvent(e);
					return false;
				}
			}
			else
			{
				this->ipAddress = inetAddressGetByString((char *)this->configData->GetConfigDataString("Component_Communications", "JUDP_IP_Address").c_str());
				if(this->ipAddress == NULL)
				{
					// Cannot open specified IP Address
					char errorString[128] = {0};
					sprintf(errorString, "Could not open specified IP Address: %s", this->configData->GetConfigDataString("Component_Communications", "JUDP_IP_Address").c_str());
					
					ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, errorString);
					this->eventHandler->handleEvent(e);
					return false;
				}
			}

			// Timeout
			if(this->configData->GetConfigDataString("Component_Communications", "JUDP_Timeout_Sec") == "")
			{
				socketTimeoutSec = JUDP_DEFAULT_COMPONENT_UDP_TIMEOUT_SEC;
			}
			else
			{
				socketTimeoutSec = this->configData->GetConfigDataDouble("Component_Communications", "JUDP_Timeout_Sec");
			}

			// TTL
			if(this->configData->GetConfigDataString("Component_Communications", "JUDP_TTL") == "")
			{
				socketTTL = JUDP_DEFAULT_COMPONENT_TTL;
			}
			else
			{
				socketTTL = this->configData->GetConfigDataInt("Component_Communications", "JUDP_TTL");
			}

			// Multicast
			if(this->configData->GetConfigDataString("Component_Communications", "JUDP_Multicast") == "")
			{
				this->multicast = JUDP_DEFAULT_COMPONENT_MULTICAST;
			}
			else
			{
				this->multicast = this->configData->GetConfigDataBool("Component_Communications", "JUDP_Multicast");
			}

			if(this->multicast)
			{
				// Multicast Group
				if(this->configData->GetConfigDataString("Component_Communications", "JUDP_Multicast_Group") == "")
				{
					// Error. Component has no default Multicast group.
					ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, "No default Component Multicast Group and none defined.");
					this->eventHandler->handleEvent(e);
					
					inetAddressDestroy(this->ipAddress);
					return false;
				}
				else
				{
					multicastGroupString = this->configData->GetConfigDataString("Component_Communications", "JUDP_Multicast_Group");
				}
			}

			// Header Compression
			// No header compression for Component Interfaces
			this->supportHeaderCompression = false;
			break;

		default:
			// Unknown type
			ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, "Unknown JudpInterface type. Cannot open socket.");
			this->eventHandler->handleEvent(e);
			return false;
	}

	// Create Socket
	this->socket = multicastSocketCreate(this->portNumber, ipAddress);
	if(!this->socket)
	{
		// Error creating our socket
		char errorString[128] = {0};
		char buf[24] = {0};
		
		inetAddressToBuffer(this->ipAddress, buf, 24);
		sprintf(errorString, "Could not open socket: %s:%d", buf, this->portNumber);
		ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, errorString);
		this->eventHandler->handleEvent(e);
		
		inetAddressDestroy(this->ipAddress);
		return false;
	}
	else
	{
		this->ipAddress->value = socket->address->value;
		this->portNumber = socket->port;
	}
	inetAddressDestroy(this->ipAddress);

	// Setup Timeout
	multicastSocketSetTimeout(this->socket, socketTimeoutSec);

	// Setup TTL
	multicastSocketSetTTL(this->socket, socketTTL);

	// Setup Multicast
	if(this->multicast)
	{
		this->multicastGroup  = inetAddressGetByString((char *)multicastGroupString.c_str());
		if(multicastSocketJoinGroup(this->socket, this->multicastGroup) != 0)
		{
			// Error joining our group
			char errorString[128] = {0};
			char buf[24] = {0};
			
			inetAddressToString(this->multicastGroup, buf);
			sprintf(errorString, "Could not open socket: %s:%d", buf, this->portNumber);
			ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, errorString);
			this->eventHandler->handleEvent(e);
			return false;
		}

		// Setup Loopback
		multicastSocketSetLoopback(this->socket, LOOPBACK_DISABLED);

		// Setup Multicast UdpData
		multicastData.addressValue = multicastGroup->value;
		multicastData.port = this->socket->port;
		
		inetAddressDestroy(this->multicastGroup);
	}

	return true;
}

void JudpInterface::sendJausMessage(JudpTransportData data, JausMessage message)
{
	DatagramPacket packet = NULL;
	int result;

	switch(this->type)
	{
		case SUBSYSTEM_INTERFACE:
		case NODE_INTERFACE:
			if(this->supportHeaderCompression)
			{
				sendCompressedMessage(data, message);
			}
			else
			{
				sendUncompressedMessage(data, message);
			}
			return;

		case COMPONENT_INTERFACE:
			// Sends a JAUS Message with no transport header
			packet = datagramPacketCreate();
			packet->bufferSizeBytes = (int) jausMessageSize(message);
			packet->buffer = (unsigned char *) calloc(packet->bufferSizeBytes, 1);
			packet->port = data.port;
			packet->address->value = data.addressValue;

			if(jausMessageToBuffer(message, packet->buffer, packet->bufferSizeBytes))
			{
				result = multicastSocketSend(this->socket, packet);
			}

			free(packet->buffer);
			datagramPacketDestroy(packet);
			return;

		default:
			char errorString[128] = {0};
			sprintf(errorString, "Unknown socket type %d\n", this->type);
			ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, errorString);
			this->eventHandler->handleEvent(e);
			return;
	}
}

void JudpInterface::closeSocket(void)
{
	multicastSocketDestroy(this->socket);
}

void JudpInterface::startRecvThread()
{
	pthread_attr_init(&this->recvThreadAttr);
	pthread_attr_setdetachstate(&this->recvThreadAttr, PTHREAD_CREATE_JOINABLE);
	this->recvThreadId = pthread_create(&this->recvThread, &this->recvThreadAttr, JudpRecvThread, this);
	pthread_attr_destroy(&this->recvThreadAttr);
}

void JudpInterface::stopRecvThread()
{
	pthread_join(this->recvThread, NULL);
}

void JudpInterface::recvThreadRun()
{
	DatagramPacket packet;
	JausMessage rxMessage;
	JudpTransportData data;
	int index = 0;
	long bytesRecv = 0;
	int bytesUnpacked = 0;
	unsigned int bufferIndex = 0;
	JudpHeaderCompressionData hcData;

	packet = datagramPacketCreate();
	packet->bufferSizeBytes = JUDP_MAX_PACKET_SIZE;
	packet->buffer = (unsigned char *) calloc(packet->bufferSizeBytes, 1);
	
	while(this->running)
	{
		index = 0;
		bytesRecv = multicastSocketReceive(this->socket, packet);
		
		if(bytesRecv > 0)
		{
			bufferIndex = 0; 
			if(packet->buffer[0] != JUDP_VERSION_NUMBER)
			{
				// Error, wrong JUDP version inbound
				ErrorEvent *e = new ErrorEvent(ErrorEvent::Message, __FUNCTION__, __LINE__, "Invalid JUDP version number in received message");
				this->eventHandler->handleEvent(e);
				continue;
			}
			bufferIndex += 1;
			
			bytesUnpacked = this->headerCompressionDataFromBuffer(&hcData, packet->buffer + bufferIndex, packet->bufferSizeBytes - bufferIndex);
			if(bytesUnpacked == 0)
			{
				// Error unpacking headerCompressionData (it creates an error event, doing so here would be redundant)
				continue;
			}
			bufferIndex += bytesUnpacked;

			rxMessage = jausMessageCreate();
			if(hcData.flags == JUDP_HC_NO_COMPRESSION)
			{
				if(!receiveUncompressedMessage(rxMessage, packet->buffer + bufferIndex, packet->bufferSizeBytes - bufferIndex))
				{
					// Error receiving message
					jausMessageDestroy(rxMessage);
					continue;
				}
			}
			else
			{
				if(!receiveCompressedMessage(rxMessage, &hcData, packet->buffer + bufferIndex, packet->bufferSizeBytes - bufferIndex))
				{
					// Error receiving message
					jausMessageDestroy(rxMessage);
					continue;
				}
			}

			// Add to transportMap
			switch(this->type)
			{
				case SUBSYSTEM_INTERFACE:
					data.addressValue = packet->address->value;
					data.port = JUDP_DATA_PORT;
					this->addressMap[rxMessage->source->subsystem] = data;
					break;

				case NODE_INTERFACE:
					data.addressValue = packet->address->value;
					data.port = JUDP_DATA_PORT;
					if(rxMessage->source->subsystem == mySubsystemId)
					{
						this->addressMap[rxMessage->source->node] = data;
					}
					else
					{
						this->subsystemGatewayData = data;
						this->subsystemGatewayDiscovered = true;
					}
					break;

				case COMPONENT_INTERFACE:
					data.addressValue = packet->address->value;
					data.port = packet->port;
					this->addressMap[jausAddressHash(rxMessage->source)] = data;
					break;

				default:
					// Unknown type
					break;
			}

			// Received message Event	
			JausMessage tempMessage = jausMessageClone(rxMessage);
			JausMessageEvent *e = new JausMessageEvent(tempMessage, this, JausMessageEvent::Inbound);
			this->eventHandler->handleEvent(e);

			// Send to Communications manager
			this->commMngr->receiveJausMessage(rxMessage, this);
		}
	}

	free(packet->buffer);
	datagramPacketDestroy(packet);
}

void JudpInterface::sendCompressedMessage(JudpTransportData data, JausMessage message)
{
	//DatagramPacket packet = NULL;
	//int result;
	//int bufferIndex = 0;

	//packet = datagramPacketCreate();
	//packet->bufferSizeBytes = (int) jausMessageSize(message) + JUDP_PER_PACKET_HEADER_SIZE_BYTES + JUDP_PER_MESSAGE_HEADER_SIZE_BYTES;
	//packet->buffer = (unsigned char *) calloc(packet->bufferSizeBytes, 1);
	//packet->port = data.port;
	//packet->address->value = data.addressValue;
	//
	//bufferIndex = 0;
	//packet->buffer[0] = JUDP_VERSION_NUMBER;
	//bufferIndex += 1;

	ErrorEvent *e = new ErrorEvent(ErrorEvent::Message, __FUNCTION__, __LINE__, "Header Compression not yet supported!");
	this->eventHandler->handleEvent(e);
}

void JudpInterface::sendUncompressedMessage(JudpTransportData data, JausMessage message)
{
	DatagramPacket packet = NULL;
	int result;
	int bufferIndex = 0;
	unsigned int bytesPacked = 0;
	JudpHeaderCompressionData hcData = {0};

	packet = datagramPacketCreate();
	packet->bufferSizeBytes = (int) jausMessageSize(message) + JUDP_PER_PACKET_HEADER_SIZE_BYTES + JUDP_PER_MESSAGE_HEADER_SIZE_BYTES;
	packet->buffer = (unsigned char *) calloc(packet->bufferSizeBytes, 1);
	packet->port = data.port;
	packet->address->value = data.addressValue;
	
	bufferIndex = 0;
	packet->buffer[0] = JUDP_VERSION_NUMBER;
	bufferIndex += 1;

	hcData.flags = JUDP_HC_NO_COMPRESSION;
	hcData.headerNumber = 0;
	hcData.length = 0;
	hcData.messageLength = message->dataSize + JAUS_HEADER_SIZE_BYTES;
	bytesPacked += headerCompressionDataToBuffer(&hcData, packet->buffer+bufferIndex, packet->bufferSizeBytes - bufferIndex);
	if(bytesPacked == 0)
	{
		free(packet->buffer);
		datagramPacketDestroy(packet);
		return;
	}
	bufferIndex += bytesPacked;

	if(jausMessageToBuffer(message, packet->buffer + bufferIndex, packet->bufferSizeBytes - bufferIndex))
	{
		result = multicastSocketSend(this->socket, packet);
		JausMessage tempMessage = jausMessageClone(message);
		JausMessageEvent *e = new JausMessageEvent(tempMessage, this, JausMessageEvent::Outbound);
		this->eventHandler->handleEvent(e);
	}

	free(packet->buffer);
	datagramPacketDestroy(packet);
}


bool JudpInterface::receiveUncompressedMessage(JausMessage rxMessage, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	return jausMessageFromBuffer(rxMessage, buffer, bufferSizeBytes)? true : false;
}

bool JudpInterface::receiveCompressedMessage(JausMessage rxMessage, JudpHeaderCompressionData *hcData, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	ErrorEvent *e;

	switch(hcData->flags)
	{
		case JUDP_HC_ENGAGE_COMPRESSION:
			// No support for Header Compression, but we can still receive these messages
			return jausMessageFromBuffer(rxMessage, buffer, bufferSizeBytes)? true : false;

		case JUDP_HC_COMPRESSION_ACKNOWLEDGE:
		case JUDP_HC_COMPRESSED_MESSAGE:
			e = new ErrorEvent(ErrorEvent::Message, __FUNCTION__, __LINE__, "Header Compression not yet supported!");
			this->eventHandler->handleEvent(e);
			return false;
		
		default:
			e = new ErrorEvent(ErrorEvent::Message, __FUNCTION__, __LINE__, "Unknown Header Compression Flag!");
			this->eventHandler->handleEvent(e);
			return false;
	}
}

unsigned int JudpInterface::headerCompressionDataToBuffer(JudpHeaderCompressionData *hcData, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	if(bufferSizeBytes < JUDP_PER_MESSAGE_HEADER_SIZE_BYTES)
	{
		ErrorEvent *e = new ErrorEvent(ErrorEvent::Message, __FUNCTION__, __LINE__, "Insufficient Size for Per Message Header");
		this->eventHandler->handleEvent(e);
		return 0;
	}

	buffer[0] = (unsigned char) (hcData->headerNumber);
	buffer[1] = (unsigned char) (((hcData->length & 0x3F) << 6) | (hcData->flags & 0x03));
	
	// NOTE: The messageLength member is BIG ENDIAN, this is different for other JAUS messages
	buffer[2] = (unsigned char) ((hcData->messageLength & 0xFF00) >> 8);
	buffer[3] = (unsigned char) (hcData->messageLength & 0xFF);

	return JUDP_PER_MESSAGE_HEADER_SIZE_BYTES;
}

unsigned int JudpInterface::headerCompressionDataFromBuffer(JudpHeaderCompressionData *hcData, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	if(bufferSizeBytes < JUDP_PER_MESSAGE_HEADER_SIZE_BYTES)
	{
		ErrorEvent *e = new ErrorEvent(ErrorEvent::Message, __FUNCTION__, __LINE__, "Insufficient Size for Per Message Header");
		this->eventHandler->handleEvent(e);
		return 0;
	}

	hcData->headerNumber = buffer[0];
	hcData->length = (buffer[1] >> 2) & 0x3F;
	hcData->flags = (buffer[1] & 0x03);
	hcData->messageLength = buffer[3] + (buffer[2] << 8);

	return JUDP_PER_MESSAGE_HEADER_SIZE_BYTES;
}

void *JudpRecvThread(void *obj)
{
	JudpInterface *judpInterface = (JudpInterface *)obj;
	judpInterface->recvThreadRun();
	return NULL;
}
