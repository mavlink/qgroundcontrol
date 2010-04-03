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
// File Name: JausOpcUdpInterface.cpp
//
// Written By: Danny Kent (jaus AT dannykent DOT com) 
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: Defines the standard JAUS UDP interface on port 3792. Is compliant with the 
// 				OPC/OPC style of UDP header

#include "nodeManager/JausOpcUdpInterface.h"
#include "nodeManager/JausSubsystemCommunicationManager.h"
#include "nodeManager/JausNodeCommunicationManager.h"
#include "nodeManager/JausComponentCommunicationManager.h"
#include "nodeManager/events/ErrorEvent.h"
#include "nodeManager/events/JausMessageEvent.h"

JausOpcUdpInterface::JausOpcUdpInterface(FileLoader *configData, EventHandler *handler, JausCommunicationManager *commMngr)
{
	this->commMngr = commMngr;
	this->eventHandler = handler;
	this->name = JAUS_OPC_UDP_NAME;
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


	// NOTE: These two values should exist in the properties file and should be checked 
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
		throw "JausOpcUdpInterface: Could not open socket\n";
	}
}

bool JausOpcUdpInterface::startInterface(void)
{
	// Set our thread running flag
	this->running = true;

	// Setup our pThread
	this->startThread();

	// Setup our receiveThread
	this->startRecvThread();

	return true;
}

bool JausOpcUdpInterface::stopInterface(void)
{
	this->running = false;
	
	// Stop our pThread
	this->stopThread();

	// Stop our receiveThread
	this->stopRecvThread();

	return true;
}

JausOpcUdpInterface::~JausOpcUdpInterface(void)
{
	if(running)
	{
		this->stopInterface();
	}
	this->closeSocket();

	// TODO: Check our threadIds to see if they terminated properly
}

InetAddress JausOpcUdpInterface::getInetAddress(void)
{
	return this->socket->address;
}

bool JausOpcUdpInterface::processMessage(JausMessage message)
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
					HASH_MAP<int, OpcUdpTransportData>::iterator iter;
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
						HASH_MAP<int, OpcUdpTransportData>::iterator iter;
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
					HASH_MAP<int, OpcUdpTransportData>::iterator iter;
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

void JausOpcUdpInterface::run()
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

std::string JausOpcUdpInterface::toString()
{
	char ret[256] = {0};
	char buf[80] = {0};
	if(this->socket)
	{
		inetAddressToBuffer(this->socket->address, buf, 80);
		sprintf(ret, "%s %s:%d", JAUS_OPC_UDP_NAME, buf, this->socket->port);
		return ret;
	}
	else
	{
		sprintf(ret, "%s Invalid.", JAUS_OPC_UDP_NAME);
		return ret;
	}
}

bool JausOpcUdpInterface::openSocket(void)
{
	double socketTimeoutSec = 0;
	unsigned char socketTTL = 0;
	std::string multicastGroupString;

	switch(this->type)
	{
		case SUBSYSTEM_INTERFACE:
			// Read Subsystem UDP Parameters
			// Port is constant per JAUS Standard
			this->portNumber = JAUS_OPC_UDP_DATA_PORT;

			// IP Address
			if(this->configData->GetConfigDataString("Subsystem_Communications", "JAUS_OPC_UDP_IP_Address") == "")
			{
				// Cannot open specified IP Address				
				ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, "No IP Address specified!");
				this->eventHandler->handleEvent(e);
				return false;
			}
			else
			{
				this->ipAddress = inetAddressGetByString((char *)this->configData->GetConfigDataString("Subsystem_Communications", "JAUS_OPC_UDP_IP_Address").c_str());
				if(this->ipAddress == NULL)
				{
					// Cannot open specified IP Address
					char errorString[128] = {0};
					sprintf(errorString, "Could not open specified IP Address: %s", this->configData->GetConfigDataString("Subsystem_Communications", "JAUS_OPC_UDP_IP_Address").c_str());
					
					ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, errorString);
					this->eventHandler->handleEvent(e);
					return false;
				}
			}

			// Timeout
			if(this->configData->GetConfigDataString("Subsystem_Communications", "JAUS_OPC_UDP_Timeout_Sec") == "")
			{
				socketTimeoutSec = OPC_UDP_DEFAULT_SUBSYSTEM_UDP_TIMEOUT_SEC;
			}
			else
			{
				socketTimeoutSec = this->configData->GetConfigDataDouble("Subsystem_Communications", "JAUS_OPC_UDP_Timeout_Sec");
			}

			// TTL
			if(this->configData->GetConfigDataString("Subsystem_Communications", "JAUS_OPC_UDP_TTL") == "")
			{
				socketTTL = OPC_UDP_DEFAULT_SUBSYSTEM_TTL;
			}
			else
			{
				socketTTL = this->configData->GetConfigDataInt("Subsystem_Communications", "JAUS_OPC_UDP_TTL");
			}

			// Multicast
			if(this->configData->GetConfigDataString("Subsystem_Communications", "JAUS_OPC_UDP_Multicast") == "")
			{
				this->multicast = OPC_UDP_DEFAULT_SUBSYSTEM_MULTICAST;
			}
			else
			{
				this->multicast = this->configData->GetConfigDataBool("Subsystem_Communications", "JAUS_OPC_UDP_Multicast");
			}

			if(this->multicast)
			{
				// Multicast Group
				if(this->configData->GetConfigDataString("Subsystem_Communications", "JAUS_OPC_UDP_Multicast_Group") == "")
				{
					multicastGroupString = OPC_UDP_DEFAULT_SUBSYSTEM_MULTICAST_GROUP;
				}
				else
				{
					multicastGroupString = this->configData->GetConfigDataString("Subsystem_Communications", "JAUS_OPC_UDP_Multicast_Group");
				}
			}
			break;

		case NODE_INTERFACE:
			// Setup Node Configuration
			// Port is constant per JAUS Standard
			this->portNumber = JAUS_OPC_UDP_DATA_PORT;

			// IP Address
			if(this->configData->GetConfigDataString("Node_Communications", "JAUS_OPC_UDP_IP_Address") == "")
			{
				// Cannot open specified IP Address				
				ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, "No IP Address specified!");
				this->eventHandler->handleEvent(e);
				return false;
			}
			else
			{
				this->ipAddress = inetAddressGetByString((char *)this->configData->GetConfigDataString("Node_Communications", "JAUS_OPC_UDP_IP_Address").c_str());
				if(this->ipAddress == NULL)
				{
					// Cannot open specified IP Address
					char errorString[128] = {0};
					sprintf(errorString, "Could not open specified IP Address: %s", this->configData->GetConfigDataString("Node_Communications", "JAUS_OPC_UDP_IP_Address").c_str());
					
					ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, errorString);
					this->eventHandler->handleEvent(e);
					return false;
				}
			}

			// Timeout
			if(this->configData->GetConfigDataString("Node_Communications", "JAUS_OPC_UDP_Timeout_Sec") == "")
			{
				socketTimeoutSec = OPC_UDP_DEFAULT_NODE_UDP_TIMEOUT_SEC;
			}
			else
			{
				socketTimeoutSec = this->configData->GetConfigDataDouble("Node_Communications", "JAUS_OPC_UDP_Timeout_Sec");
			}

			// TTL
			if(this->configData->GetConfigDataString("Node_Communications", "JAUS_OPC_UDP_TTL") == "")
			{
				socketTTL = OPC_UDP_DEFAULT_NODE_TTL;
			}
			else
			{
				socketTTL = this->configData->GetConfigDataInt("Node_Communications", "JAUS_OPC_UDP_TTL");
			}

			// Multicast
			if(this->configData->GetConfigDataString("Node_Communications", "JAUS_OPC_UDP_Multicast") == "")
			{
				this->multicast = OPC_UDP_DEFAULT_NODE_MULTICAST;
			}
			else
			{
				this->multicast = this->configData->GetConfigDataBool("Node_Communications", "JAUS_OPC_UDP_Multicast");
			}

			if(this->multicast)
			{
				// Multicast Group
				if(this->configData->GetConfigDataString("Node_Communications", "JAUS_OPC_UDP_Multicast_Group") == "")
				{
					multicastGroupString = OPC_UDP_DEFAULT_NODE_MULTICAST_GROUP;
				}
				else
				{
					multicastGroupString = this->configData->GetConfigDataString("Node_Communications", "JAUS_OPC_UDP_Multicast_Group");
				}
			}
			break;

		case COMPONENT_INTERFACE:
			// Read Component Configuration
			// Port Number
			if(this->configData->GetConfigDataString("Component_Communications", "JAUS_OPC_UDP_Port") == "")
			{
				this->portNumber = OPC_UDP_DEFAULT_COMPONENT_UDP_PORT;
			}
			else
			{
				this->portNumber = this->configData->GetConfigDataInt("Component_Communications", "JAUS_OPC_UDP_Port");
			}

			// IP Address
			if(this->configData->GetConfigDataString("Component_Communications", "JAUS_OPC_UDP_IP_Address") == "")
			{
				this->ipAddress = inetAddressGetByString((char *)OPC_UDP_DEFAULT_COMPONENT_IP.c_str());
				if(this->ipAddress == NULL)
				{
					// Cannot open specified IP Address
					char errorString[128] = {0};
					sprintf(errorString, "Could not open default IP Address: %s", OPC_UDP_DEFAULT_COMPONENT_IP.c_str());
					
					ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, errorString);
					this->eventHandler->handleEvent(e);
					return false;
				}
			}
			else
			{
				this->ipAddress = inetAddressGetByString((char *)this->configData->GetConfigDataString("Component_Communications", "JAUS_OPC_UDP_IP_Address").c_str());
				if(this->ipAddress == NULL)
				{
					// Cannot open specified IP Address
					char errorString[128] = {0};
					sprintf(errorString, "Could not open specified IP Address: %s", this->configData->GetConfigDataString("Component_Communications", "JAUS_OPC_UDP_IP_Address").c_str());
					
					ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, errorString);
					this->eventHandler->handleEvent(e);
					return false;
				}
			}

			// Timeout
			if(this->configData->GetConfigDataString("Component_Communications", "JAUS_OPC_UDP_Timeout_Sec") == "")
			{
				socketTimeoutSec = OPC_UDP_DEFAULT_COMPONENT_UDP_TIMEOUT_SEC;
			}
			else
			{
				socketTimeoutSec = this->configData->GetConfigDataDouble("Component_Communications", "JAUS_OPC_UDP_Timeout_Sec");
			}

			// TTL
			if(this->configData->GetConfigDataString("Component_Communications", "JAUS_OPC_UDP_TTL") == "")
			{
				socketTTL = OPC_UDP_DEFAULT_COMPONENT_TTL;
			}
			else
			{
				socketTTL = this->configData->GetConfigDataInt("Component_Communications", "JAUS_OPC_UDP_TTL");
			}

			// Multicast
			if(this->configData->GetConfigDataString("Component_Communications", "JAUS_OPC_UDP_Multicast") == "")
			{
				this->multicast = OPC_UDP_DEFAULT_COMPONENT_MULTICAST;
			}
			else
			{
				this->multicast = this->configData->GetConfigDataBool("Component_Communications", "JAUS_OPC_UDP_Multicast");
			}

			if(this->multicast)
			{
				// Multicast Group
				if(this->configData->GetConfigDataString("Component_Communications", "JAUS_OPC_UDP_Multicast_Group") == "")
				{
					// Error. Component has no default Multicast group.
					ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, "No default Component Multicast Group and none defined.");
					this->eventHandler->handleEvent(e);
					
					inetAddressDestroy(this->ipAddress);
					return false;
				}
				else
				{
					multicastGroupString = this->configData->GetConfigDataString("Component_Communications", "JAUS_OPC_UDP_Multicast_Group");
				}
			}
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

void JausOpcUdpInterface::sendJausMessage(OpcUdpTransportData data, JausMessage message)
{
	DatagramPacket packet = NULL;
	int result;

	JausMessage tempMessage = jausMessageClone(message);
	JausMessageEvent *e = new JausMessageEvent(tempMessage, this, JausMessageEvent::Outbound);
	this->eventHandler->handleEvent(e);

	switch(this->type)
	{
		case SUBSYSTEM_INTERFACE:
		case NODE_INTERFACE:
			packet = datagramPacketCreate();
			packet->bufferSizeBytes = (int) jausMessageSize(message) + JAUS_OPC_UDP_HEADER_SIZE_BYTES;
			packet->buffer = (unsigned char *) calloc(packet->bufferSizeBytes, 1);
			packet->port = data.port;
			packet->address->value = data.addressValue;
			
			memcpy(packet->buffer, JAUS_OPC_UDP_HEADER, JAUS_OPC_UDP_HEADER_SIZE_BYTES);
			if(jausMessageToBuffer(message, packet->buffer + JAUS_OPC_UDP_HEADER_SIZE_BYTES, packet->bufferSizeBytes - JAUS_OPC_UDP_HEADER_SIZE_BYTES))
			{
				result = multicastSocketSend(this->socket, packet);
			}

			free(packet->buffer);
			datagramPacketDestroy(packet);
			return;

		case COMPONENT_INTERFACE:
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

void JausOpcUdpInterface::closeSocket(void)
{
	multicastSocketDestroy(this->socket);
}

void JausOpcUdpInterface::startRecvThread()
{
	pthread_attr_init(&this->recvThreadAttr);
	pthread_attr_setdetachstate(&this->recvThreadAttr, PTHREAD_CREATE_JOINABLE);
	this->recvThreadId = pthread_create(&this->recvThread, &this->recvThreadAttr, OpcUdpRecvThread, this);
	pthread_attr_destroy(&this->recvThreadAttr);
}

void JausOpcUdpInterface::stopRecvThread()
{
	pthread_join(this->recvThread, NULL);
}

void JausOpcUdpInterface::recvThreadRun()
{
	DatagramPacket packet;
	JausMessage rxMessage;
	OpcUdpTransportData data;
	int index = 0;
	long bytesRecv = 0;

	packet = datagramPacketCreate();
	packet->bufferSizeBytes = JAUS_HEADER_SIZE_BYTES + JAUS_MAX_DATA_SIZE_BYTES + JAUS_OPC_UDP_HEADER_SIZE_BYTES;
	packet->buffer = (unsigned char *) calloc(packet->bufferSizeBytes, 1);
	
	while(this->running)
	{
		index = 0;
		bytesRecv = multicastSocketReceive(this->socket, packet);
		
		if(bytesRecv > 0)
		{
			if(!strncmp((char *)packet->buffer, JAUS_OPC_UDP_HEADER, JAUS_OPC_UDP_HEADER_SIZE_BYTES)) // equals 1 if same
			{
				index += JAUS_OPC_UDP_HEADER_SIZE_BYTES;
			}
			else if(this->type == SUBSYSTEM_INTERFACE || this->type == NODE_INTERFACE)
			{
				char errorString[128] = {0};
				sprintf(errorString, "Received packet on %s with invalid header. First byte is: %d (%c)", this->toString().c_str(), (char) packet->buffer[0], (char) packet->buffer[0]);
				ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, errorString);
				this->eventHandler->handleEvent(e);
				continue;
			}

			rxMessage = jausMessageCreate();
			if(jausMessageFromBuffer(rxMessage, packet->buffer + index, packet->bufferSizeBytes - index))
			{
				JausMessage tempMessage = jausMessageClone(rxMessage);
				JausMessageEvent *e = new JausMessageEvent(tempMessage, this, JausMessageEvent::Inbound);
				this->eventHandler->handleEvent(e);
				
				// Add to transportMap
				switch(this->type)
				{
					case SUBSYSTEM_INTERFACE:
						data.addressValue = packet->address->value;
						data.port = JAUS_OPC_UDP_DATA_PORT;
						this->addressMap[rxMessage->source->subsystem] = data;
						break;

					case NODE_INTERFACE:
						data.addressValue = packet->address->value;
						data.port = JAUS_OPC_UDP_DATA_PORT;
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

				this->commMngr->receiveJausMessage(rxMessage, this);
			}
			else
			{
				jausMessageDestroy(rxMessage);
			}
		}
	}

	free(packet->buffer);
	datagramPacketDestroy(packet);
}

void *OpcUdpRecvThread(void *obj)
{
	JausOpcUdpInterface *etgUdpInterface = (JausOpcUdpInterface *)obj;
	etgUdpInterface->recvThreadRun();
	return NULL;
}
