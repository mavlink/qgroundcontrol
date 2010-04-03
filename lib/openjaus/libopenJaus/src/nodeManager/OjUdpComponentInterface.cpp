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
// File Name: OjUdpComponentInterface.cpp
//
// Written By: Danny Kent (jaus AT dannykent DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file provides an interface through which the NM communicates 
//				to components using a non-JAUS compliant synchronous UDP port over 
//				the loopback device.

#include "nodeManager/OjUdpComponentInterface.h"
#include "nodeManager/events/ErrorEvent.h"

OjUdpComponentInterface::OjUdpComponentInterface(FileLoader *configData, EventHandler *handler, JausComponentCommunicationManager *cmptMngr)
{
	this->configData = configData;
	this->commMngr = cmptMngr;
	this->eventHandler = handler;
	
	this->systemTree = cmptMngr->getSystemTree();
	this->nodeManager = cmptMngr->getNodeManagerComponent();
	this->type = COMPONENT_INTERFACE;
	this->name = "OpenJAUS UDP Component to Node Manager Interface";
	this->portMap.empty();

	// Open our socket
	if(!this->openSocket())
	{
		throw "OjUdpComponentInterface: Could not open socket\n";
	}
}

OjUdpComponentInterface::~OjUdpComponentInterface(void)
{
	if(running)
	{
		this->stopInterface();
	}
	this->closeSocket();	
}

unsigned int OjUdpComponentInterface::getPort(void)
{
	return this->socket->port;
}

bool OjUdpComponentInterface::startInterface()
{
	// Setup our thread control flag
	this->running = true;

	// Setup our pThread
	this->startThread();

	return true;
}

bool OjUdpComponentInterface::stopInterface()
{
	// Set our thread control flag to false 
	this->running = false;

	// Stop our pThread
	this->stopThread();

	return true;
}

bool OjUdpComponentInterface::processMessage(JausMessage message)
{ 
	// This interface does not transport jaus messages
	// This isn't an "error" because the cmptMngr send JausMessages to all known interfaces for broadcasts
	jausMessageDestroy(message);
	return false;
}

bool OjUdpComponentInterface::sendDatagramPacket(DatagramPacket dgPacket)
{
	return true;
}

bool OjUdpComponentInterface::openSocket(void)
{
	// Read Configuration
	if(this->configData->GetConfigDataString("Component_Communications", "OpenJAUS_UDP_Port") == "")
	{
		this->portNumber = OJ_UDP_DEFAULT_PORT;
	}
	else
	{
		this->portNumber = this->configData->GetConfigDataInt("Component_Communications", "OpenJAUS_UDP_Port");
	}

	// IP Address
	if(this->configData->GetConfigDataString("Component_Communications", "OpenJAUS_UDP_IP_Address") == "")
	{
		this->ipAddress = inetAddressGetByString((char *)OJ_UDP_DEFAULT_COMPONENT_IP.c_str());
		if(this->ipAddress == NULL)
		{
			// Cannot open specified IP Address
			char errorString[128] = {0};
			sprintf(errorString, "Could not open default IP Address: %s", OJ_UDP_DEFAULT_COMPONENT_IP.c_str());
			
			ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, errorString);
			this->eventHandler->handleEvent(e);
			return false;
		}
	}
	else
	{
		this->ipAddress = inetAddressGetByString((char *)this->configData->GetConfigDataString("Component_Communications", "OpenJAUS_UDP_IP_Address").c_str());
		if(this->ipAddress == NULL)
		{
			// Cannot open specified IP Address
			char errorString[128] = {0};
			sprintf(errorString, "Could not open specified IP Address: %s", this->configData->GetConfigDataString("Component_Communications", "OpenJAUS_UDP_IP_Address").c_str());
			
			ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, errorString);
			this->eventHandler->handleEvent(e);
			return false;
		}
	}
	
	// Create Component Socket
	this->socket = datagramSocketCreate(this->portNumber, ipAddress);
	if(!this->socket)
	{
		// Error creating our socket
		return false;
	}
	
	inetAddressDestroy(this->ipAddress);

	// Setup Timeout
	if(this->configData->GetConfigDataString("Component_Communications", "OpenJAUS_UDP_Timeout_Sec") == "")
	{
		datagramSocketSetTimeout(this->socket, OJ_UDP_DEFAULT_TIMEOUT);
	}
	else
	{
		datagramSocketSetTimeout(this->socket, this->configData->GetConfigDataDouble("Component_Communications", "OpenJAUS_UDP_Timeout_Sec"));
	}

	return true;
}

void OjUdpComponentInterface::closeSocket(void)
{
	datagramSocketDestroy(this->socket);
}

void OjUdpComponentInterface::run()
{
	DatagramPacket packet;
	JausAddress address, currentAddress;
	JausAddress lookupAddress;
	int bytesRecv = 0;
	char buf[256] = {0};
	int componentId = 0;
	int commandCode = 0;
	int serviceType = 0;

	packet = datagramPacketCreate();
	packet->bufferSizeBytes = OJ_UDP_INTERFACE_MESSAGE_SIZE_BYTES;
	packet->buffer = (unsigned char *) calloc(packet->bufferSizeBytes, 1);
	
	while(this->running)
	{
		bytesRecv = datagramSocketReceive(this->socket, packet);
		if(bytesRecv == OJ_UDP_INTERFACE_MESSAGE_SIZE_BYTES)
		{
			// This is to ensure we are using a valid NM pointer (this occurs
			this->nodeManager = ((JausComponentCommunicationManager *)this->commMngr)->getNodeManagerComponent();

			switch(packet->buffer[0])
			{
				case CHECK_IN:
					componentId = (packet->buffer[1] & 0xFF);
					if(componentId < JAUS_MINIMUM_COMPONENT_ID || componentId > JAUS_MAXIMUM_COMPONENT_ID)
					{
						sprintf(buf, "Invalid Component Id (%d) trying to check in.", componentId);
						ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, buf);
						this->eventHandler->handleEvent(e);
						return;
					}

					address = this->nodeManager->checkInLocalComponent(componentId);
					if(!address || !jausAddressIsValid(address))
					{
						sprintf(buf, "Cannot add local component with Id: %d.", componentId);
						ErrorEvent *e = new ErrorEvent(ErrorEvent::Warning, __FUNCTION__, __LINE__, buf);
						this->eventHandler->handleEvent(e);
						
						// TODO: Send back checkin error reply (no available instance)							
						break;
					}

					memset(packet->buffer, 0, OJ_UDP_INTERFACE_MESSAGE_SIZE_BYTES);
					packet->buffer[0] = REPORT_ADDRESS;
					packet->buffer[1] = (unsigned char)(address->instance & 0xFF);
					packet->buffer[2] = (unsigned char)(address->component & 0xFF);
					packet->buffer[3] = (unsigned char)(address->node & 0xFF);
					packet->buffer[4] = (unsigned char)(address->subsystem & 0xFF);

					datagramSocketSend(this->socket, packet);
					jausAddressDestroy(address);
					break;

				case CHECK_OUT:
					nodeManager->checkOutLocalComponent(packet->buffer[4], packet->buffer[3], packet->buffer[2], packet->buffer[1]);
					break;
				    
				case VERIFY_ADDRESS:
					lookupAddress = jausAddressCreate();
					lookupAddress->instance = (packet->buffer[1] & 0xFF);
					lookupAddress->component = (packet->buffer[2] & 0xFF);
					lookupAddress->node = (packet->buffer[3] & 0xFF);
					lookupAddress->subsystem = (packet->buffer[4] & 0xFF);						

					memset(packet->buffer, 0, OJ_UDP_INTERFACE_MESSAGE_SIZE_BYTES);
					packet->buffer[0] = ADDRESS_VERIFIED;
					if(this->systemTree->hasComponent(lookupAddress))
					{
						packet->buffer[1] = JAUS_TRUE;
					}
					else
					{
						packet->buffer[1] = JAUS_FALSE;
					}

					datagramSocketSend(this->socket, packet);
					jausAddressDestroy(lookupAddress);
					break;
					
				case GET_COMPONENT_ADDRESS_LIST:
					componentId  = packet->buffer[1] & 0xFF;
					
					address = systemTree->lookUpAddress(JAUS_ADDRESS_WILDCARD_OCTET, JAUS_ADDRESS_WILDCARD_OCTET, componentId, JAUS_ADDRESS_WILDCARD_OCTET);
					memset(packet->buffer, 0, OJ_UDP_INTERFACE_MESSAGE_SIZE_BYTES);
					packet->buffer[0] = COMPONENT_ADDRESS_LIST_RESPONSE;
					//		
					if(!address || !jausAddressIsValid(address))
					{
						packet->buffer[1] = (unsigned char)(JAUS_INVALID_INSTANCE_ID & 0xFF);
						packet->buffer[2] = (unsigned char)(JAUS_INVALID_COMPONENT_ID & 0xFF);
						packet->buffer[3] = (unsigned char)(JAUS_INVALID_NODE_ID & 0xFF);
						packet->buffer[4] = (unsigned char)(JAUS_INVALID_SUBSYSTEM_ID & 0xFF);
					}
					else
					{
						packet->buffer[1] = (unsigned char)(address->instance & 0xFF);
						packet->buffer[2] = (unsigned char)(address->component & 0xFF);
						packet->buffer[3] = (unsigned char)(address->node & 0xFF);
						packet->buffer[4] = (unsigned char)(address->subsystem & 0xFF);
					}
					datagramSocketSend(this->socket, packet);
					jausAddressDestroy(address);
					break;
	
				case LOOKUP_ADDRESS:
					lookupAddress = jausAddressCreate();
					lookupAddress->instance = (packet->buffer[1] & 0xFF);
					lookupAddress->component = (packet->buffer[2] & 0xFF);
					lookupAddress->node = (packet->buffer[3] & 0xFF);
					lookupAddress->subsystem = (packet->buffer[4] & 0xFF);						

					memset(packet->buffer, 0, OJ_UDP_INTERFACE_MESSAGE_SIZE_BYTES);
					packet->buffer[0] = LOOKUP_ADDRESS_RESPONSE;
					
					address = systemTree->lookUpAddress(lookupAddress);
					if(address && jausAddressIsValid(address))
					{
						packet->buffer[5] = (unsigned char) JAUS_TRUE;
					}
					else
					{
						packet->buffer[5] = (unsigned char) JAUS_FALSE;					
					}
					
					packet->buffer[1] = (unsigned char) (address->instance & 0xFF);
					packet->buffer[2] = (unsigned char) (address->component & 0xFF);
					packet->buffer[3] = (unsigned char) (address->node & 0xFF);
					packet->buffer[4] = (unsigned char) (address->subsystem & 0xFF);
					datagramSocketSend(this->socket, packet);
					
					while(address)								// NMJ
					{											// NMJ
						currentAddress = address;				// NMJ
						address = address->next;				// NMJ
						jausAddressDestroy(currentAddress);		// NMJ
					}											// NMJ
					
					jausAddressDestroy(lookupAddress);
					break;

				case LOOKUP_SERVICE_ADDRESS:
					lookupAddress = jausAddressCreate();
					lookupAddress->instance = (packet->buffer[1] & 0xFF);
					lookupAddress->component = (packet->buffer[2] & 0xFF);
					lookupAddress->node = (packet->buffer[3] & 0xFF);
					lookupAddress->subsystem = (packet->buffer[4] & 0xFF);

					commandCode = (packet->buffer[5] & 0xFF) + ((packet->buffer[6] & 0xFF) << 8);
					serviceType = (packet->buffer[7] & 0xFF);

					memset(packet->buffer, 0, OJ_UDP_INTERFACE_MESSAGE_SIZE_BYTES);
					packet->buffer[0] = LOOKUP_SERVICE_ADDRESS_RESPONSE;
					
					address = systemTree->lookUpServiceInSystem(commandCode, serviceType);
					if(address && jausAddressIsValid(address)) 
					{
						packet->buffer[5] = (unsigned char) JAUS_TRUE;
						packet->buffer[1] = (unsigned char) (address->instance & 0xFF);
						packet->buffer[2] = (unsigned char) (address->component & 0xFF);
						packet->buffer[3] = (unsigned char) (address->node & 0xFF);
						packet->buffer[4] = (unsigned char) (address->subsystem & 0xFF);
					}
					else
					{
						packet->buffer[5] = (unsigned char) JAUS_FALSE;
					}

					datagramSocketSend(this->socket, packet);
					
					if(address)
					{
						jausAddressDestroy(address);
						jausAddressDestroy(lookupAddress);
					}
					
					break;

				//case InterfaceMessage.NODE_MANAGER_LOOKUP_SERVICE_ADDRESS_LIST:
				//	lookupAddress = jausAddressCreate();
				//	lookupAddress->instance(packet->buffer[1] & 0xFF);
				//	lookupAddress->component(packet->buffer[2] & 0xFF);
				//	lookupAddress->node(packet->buffer[3] & 0xFF);
				//	lookupAddress->subsystem(packet->buffer[4] & 0xFF);						
				//	
				//	commandCode = (packet->buffer[5] & 0xFF) + ((packet->buffer[6] & 0xFF) << 8);
				//	serviceType = (packet->buffer[7] & 0xFF);

				//	memset(packet->buffer, 0, OJ_UDP_INTERFACE_MESSAGE_SIZE_BYTES);
				//	packet->buffer[0] = LOOKUP_SERVICE_ADDRESS_LIST_RESPONSE;

				//	//System.out.println("CmptInterface: Get address for component ID: " + componentId);
				//	//address = subsystemTable.lookupServiceAddressList(address, commandCode, serviceCommandType);
				//	
				//	if(lookupAddress->subsystem == 

				//	if(address == null)
				//	{
				//		replyMessage.getData()[0] = (byte)(0 & 0xff);
				//		replyMessage.getData()[1] = (byte)(0 & 0xff);
				//		replyMessage.getData()[2] = (byte)(0 & 0xff);
				//		replyMessage.getData()[3] = (byte)(0 & 0xff);
				//		replyMessage.getData()[4] = (byte)0;
				//		//System.out.println("CmptInterface: Get address for component ID: " + componentId + " returning: 0.0.0.0");
				//	}
				//	else	
				//	{
				//		replyMessage.getData()[0] = (byte)(address.getInstance() & 0xff);
				//		replyMessage.getData()[1] = (byte)(address.getComponent() & 0xff);
				//		replyMessage.getData()[2] = (byte)(address.getNode() & 0xff);
				//		replyMessage.getData()[3] = (byte)(address.getSubsystem() & 0xff);							
				//		replyMessage.getData()[4] = (byte)1;
				//		//System.out.println("CmptInterface: Get address for component ID: " + componentId + " returning: " + address);
				//	}
				//	
				//  replyBuffer = new byte[8];
				//	replyMessage.pack(replyBuffer);
				//	outPacket = new DatagramPacket(replyBuffer, replyBuffer.length, packet.getAddress(), packet.getPort());
				//	socket.send(outPacket);
				//	break;
				    
				default:
					sprintf(buf, "Unknown Interface Message Received. CC: 0x%02X\n", packet->buffer[0]);
					ErrorEvent *e = new ErrorEvent(ErrorEvent::Warning, __FUNCTION__, __LINE__, buf);
					this->eventHandler->handleEvent(e);
					break;
			}
		}
	}
	
	free(packet->buffer);
	datagramPacketDestroy(packet);
}

bool OjUdpComponentInterface::processOjNodemanagerInterfaceMessage(char *buffer)
{
	return true;
}

std::string OjUdpComponentInterface::toString()
{
	return this->name;
}

