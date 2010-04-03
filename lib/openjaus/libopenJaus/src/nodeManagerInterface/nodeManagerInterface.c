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
// File:		libNodeManager.c
// Version:		3.3 BETA
// Written by:	Tom Galluzzo (galluzzo AT gmail DOT com) and Danny Kent (jaus AT dannykent DOT com)
// Date:		04/15/08
// Description:	The libnodeManager provides an interface for a commponent to access core node management,
//				service connections, large message handling and default message processing

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined (WIN32)
	#define WIN32_LEAN_AND_MEAN
	#include <winsock2.h>
	#include <windows.h>
#elif defined(__linux) || defined(linux) || defined(__linux__) || defined(__APPLE__)
	#include <unistd.h>
	#include <sys/time.h>
	#include <errno.h>
#endif

#include "nodeManagerInterface/nodeManagerInterface.h"

#define JAUS_OPC_UDP_HEADER				"JAUS01.0"
#define JAUS_OPC_UDP_HEADER_SIZE_BYTES	8 

#define NODE_MANAGER_INTERFACE_PORT 24627
#define NODE_MANAGER_MESSAGE_PORT 24629
#define NODE_MANAGER_TIMEOUT_SEC 3.0

#define INTERFACE_THREAD_TIMEOUT_SEC	3.0
#define INTERFACE_SOCKET_TIMEOUT_SEC	1.0
#define MESSAGE_SOCKET_TIMEOUT_SEC		0.5

#define INTERFACE_MESSAGE_SIZE_BYTES 							8
#define INTERFACE_MESSAGE_CHECK_IN								0x01
#define INTERFACE_MESSAGE_REPORT_ADDRESS						0x02
#define INTERFACE_MESSAGE_CHECK_OUT								0x03
#define INTERFACE_MESSAGE_VERIFY_ADDRESS						0x04
#define INTERFACE_MESSAGE_ADDRESS_VERIFIED						0x05
#define INTERFACE_MESSAGE_GET_COMPONENT_ADDRESS_LIST			0x06
#define INTERFACE_MESSAGE_COMPONENT_ADDRESS_LIST_RESPONSE		0x07
#define INTERFACE_MESSAGE_LOOKUP_ADDRESS						0x08
#define INTERFACE_MESSAGE_LOOKUP_ADDRESS_RESPONSE				0x09
#define INTERFACE_MESSAGE_LOOKUP_SERVICE_ADDRESS				0x0A
#define INTERFACE_MESSAGE_LOOKUP_SERVICE_ADDRESS_RESPONSE		0x0B
#define INTERFACE_MESSAGE_LOOKUP_SERVICE_ADDRESS_LIST			0x0C
#define INTERFACE_MESSAGE_LOOKUP_SERVICE_ADDRESS_LIST_RESPONSE	0x0D

static int checkIntoNodeManager(NodeManagerInterface);
static int checkOutOfNodeManager(NodeManagerInterface);
void *heartbeatThread(void *);
void *receiveThread(void *);

NodeManagerInterface nodeManagerOpen(JausComponent cmpt)
{
	NodeManagerInterface nmi = NULL;

	if(cmpt == NULL)
	{
		return NULL;
	}
	
	nmi = (NodeManagerInterface)malloc(sizeof(NodeManagerInterfaceStruct));
	if(nmi == NULL)
	{
		return NULL;
	}
	
	nmi->cmpt = cmpt;
	nmi->timestamp = ojGetTimeSec();
	pthread_cond_init(&nmi->recvCondition, NULL);
	pthread_cond_init(&nmi->hbWakeCondition, NULL);
	
	nmi->ipAddress = inetAddressGetLocalHost();
	if(nmi->ipAddress == NULL)
	{
		free(nmi);
		return NULL;
	}

	nmi->interfaceSocket = datagramSocketCreate(0, nmi->ipAddress);
	if(nmi->interfaceSocket == NULL)
	{
		inetAddressDestroy(nmi->ipAddress);
		free(nmi);
		return NULL;
	}
	datagramSocketSetTimeout(nmi->interfaceSocket, INTERFACE_SOCKET_TIMEOUT_SEC);
	
	memset(&nmi->messageSocket, 0, sizeof(nmi->messageSocket));
	nmi->messageSocket = datagramSocketCreate(0, nmi->ipAddress);
	if(nmi->messageSocket == NULL)
	{
		datagramSocketDestroy(nmi->interfaceSocket);
		inetAddressDestroy(nmi->ipAddress);
		free(nmi);
		return NULL;
	}
	datagramSocketSetTimeout(nmi->messageSocket, MESSAGE_SOCKET_TIMEOUT_SEC);

	if(checkIntoNodeManager(nmi))
	{
		datagramSocketDestroy(nmi->messageSocket);
		datagramSocketDestroy(nmi->interfaceSocket);
		inetAddressDestroy(nmi->ipAddress);
		free(nmi);
		return NULL;
	}

	nmi->receiveQueue = queueCreate();
	
	nmi->scm = scManagerCreate();
	if(nmi->scm == NULL)
	{
		queueDestroy(nmi->receiveQueue, NULL);
		checkOutOfNodeManager(nmi);
		datagramSocketDestroy(nmi->messageSocket);
		datagramSocketDestroy(nmi->interfaceSocket);
		inetAddressDestroy(nmi->ipAddress);
		free(nmi);
		return NULL;
	}
		
	nmi->lmh = lmHandlerCreate();
	if(nmi->lmh == NULL)
	{
		scManagerDestroy(nmi->scm);
		queueDestroy(nmi->receiveQueue, NULL);
		checkOutOfNodeManager(nmi);
		datagramSocketDestroy(nmi->messageSocket);
		datagramSocketDestroy(nmi->interfaceSocket);
		inetAddressDestroy(nmi->ipAddress);
		free(nmi);
		return NULL;
	}

	nmi->isOpen = JAUS_TRUE;

	if(pthread_create(&nmi->heartbeatThreadId, NULL, heartbeatThread, (void *)nmi))
	{
		lmHandlerDestroy(nmi->lmh);
		scManagerDestroy(nmi->scm);
		queueDestroy(nmi->receiveQueue, NULL);
		checkOutOfNodeManager(nmi);
		datagramSocketDestroy(nmi->messageSocket);
		datagramSocketDestroy(nmi->interfaceSocket);
		inetAddressDestroy(nmi->ipAddress);
		free(nmi);
		return NULL;
	}

	if(pthread_create(&nmi->receiveThreadId, NULL, receiveThread, (void *)nmi))
	{
		pthread_cancel(nmi->heartbeatThreadId);
		lmHandlerDestroy(nmi->lmh);
		scManagerDestroy(nmi->scm);
		queueDestroy(nmi->receiveQueue, NULL);
		checkOutOfNodeManager(nmi);
		datagramSocketDestroy(nmi->messageSocket);
		datagramSocketDestroy(nmi->interfaceSocket);
		inetAddressDestroy(nmi->ipAddress);
		free(nmi);
		return NULL;
	}
	
	scManagerAddSupportedMessage(nmi, JAUS_REPORT_COMPONENT_STATUS);
	scManagerAddSupportedMessage(nmi, JAUS_REPORT_COMPONENT_AUTHORITY);

	return nmi;
}

int nodeManagerClose(NodeManagerInterface nmi)
{
	int result = 0;
	
	if(nmi == NULL)
	{
		return -1;
	}
	
	if(nmi->isOpen) // Execute the close only if the connection is open
	{
		scManagerRemoveSupportedMessage(nmi, JAUS_REPORT_COMPONENT_STATUS);
		scManagerRemoveSupportedMessage(nmi, JAUS_REPORT_COMPONENT_AUTHORITY);

		nmi->isOpen = 0;

		pthread_cond_signal(&nmi->hbWakeCondition);
		pthread_join(nmi->heartbeatThreadId, NULL);

		CLOSE_SOCKET(nmi->messageSocket->descriptor);
		pthread_join(nmi->receiveThreadId, NULL);

		lmHandlerDestroy(nmi->lmh);
		scManagerDestroy(nmi->scm);
		queueDestroy(nmi->receiveQueue, (void*)jausMessageDestroy); 
		checkOutOfNodeManager(nmi);
		datagramSocketDestroy(nmi->messageSocket);
		datagramSocketDestroy(nmi->interfaceSocket);
		inetAddressDestroy(nmi->ipAddress);
		pthread_cond_destroy(&nmi->recvCondition);
		pthread_cond_destroy(&nmi->hbWakeCondition);
		free(nmi);
	}
	else
	{
		return -2;
	}

	return result;
};

static int checkIntoNodeManager(NodeManagerInterface nmi)
{
	DatagramPacket packet;

	packet = datagramPacketCreate();

	packet->bufferSizeBytes = INTERFACE_MESSAGE_SIZE_BYTES;
	packet->buffer = (unsigned char *)malloc(packet->bufferSizeBytes);
	memset(packet->buffer, 0, packet->bufferSizeBytes);	
	packet->buffer[0]= INTERFACE_MESSAGE_CHECK_IN;
	packet->buffer[1]= nmi->cmpt->address->component;
	packet->buffer[2] = (unsigned char)(nmi->messageSocket->port & 0xFF);
	packet->buffer[3] = (unsigned char)((nmi->messageSocket->port & 0xFF00) >> 8);

	packet->port = NODE_MANAGER_INTERFACE_PORT;
	packet->address->value = nmi->ipAddress->value;

	datagramSocketSend(nmi->interfaceSocket, packet);
	datagramSocketReceive(nmi->interfaceSocket, packet);

	if(packet->buffer[0] == INTERFACE_MESSAGE_REPORT_ADDRESS)
	{
		nmi->cmpt->address->instance = packet->buffer[1];
		nmi->cmpt->address->component = packet->buffer[2];
		nmi->cmpt->address->node = packet->buffer[3];
		nmi->cmpt->address->subsystem = packet->buffer[4];

		free(packet->buffer);
		datagramPacketDestroy(packet);
		return 0;
	}
	else
	{
		free(packet->buffer);
		datagramPacketDestroy(packet);
		return -1;
	}
}

static int checkOutOfNodeManager(NodeManagerInterface nmi)
{
	DatagramPacket packet;

	packet = datagramPacketCreate();

	packet->bufferSizeBytes = INTERFACE_MESSAGE_SIZE_BYTES * sizeof(unsigned char);
	packet->buffer = (unsigned char*)malloc(packet->bufferSizeBytes);
	memset(packet->buffer, 0, packet->bufferSizeBytes);	
	packet->buffer[0]= INTERFACE_MESSAGE_CHECK_OUT;
	packet->buffer[1]= nmi->cmpt->address->instance;
	packet->buffer[2] = nmi->cmpt->address->component;
	packet->buffer[3] = nmi->cmpt->address->node;
	packet->buffer[4] = nmi->cmpt->address->subsystem;
	packet->port = NODE_MANAGER_INTERFACE_PORT;
	packet->address->value = nmi->ipAddress->value;

	datagramSocketSend(nmi->interfaceSocket, packet);

	// datagramSocketReceive(nmi->interfaceSocket, &packet);  TODO: Implement reply

	free(packet->buffer);
	datagramPacketDestroy(packet);
	return 1;
}

void *heartbeatThread(void *threadArgument)
{
	pthread_mutex_t hbMutex = PTHREAD_MUTEX_INITIALIZER;
	int condition = -1;
	struct timespec timeLimitSpec;
	double timeLimitSec = 0;
	NodeManagerInterface nmi = (NodeManagerInterface)threadArgument;
	JausMessage txMessage;
	ReportHeartbeatPulseMessage heartbeat = reportHeartbeatPulseMessageCreate();
	
	nmi->heartbeatThreadRunning = 1;
	nmi->heartbeatCount = 0;
	
	jausAddressCopy(heartbeat->source, nmi->cmpt->address);
	heartbeat->destination->subsystem = nmi->cmpt->address->subsystem;
	heartbeat->destination->node = nmi->cmpt->address->node;
	heartbeat->destination->component = JAUS_NODE_MANAGER_COMPONENT;
	heartbeat->destination->instance = 1;

	txMessage = reportHeartbeatPulseMessageToJausMessage(heartbeat);

	while(nmi->isOpen)
	{
		nodeManagerSend(nmi, txMessage);
		nmi->heartbeatCount++;
		if( (ojGetTimeSec() - NODE_MANAGER_TIMEOUT_SEC) > nmi->timestamp)
		{
			// TODO: Capture Error
			//printf("libNodeManager: Node Manager Has Timed Out\n");
			//nmi->cmpt->state = JAUS_FAILURE_STATE;
			break;
		}

		timeLimitSec = ojGetTimeSec() + 1.0;
		timeLimitSpec.tv_sec = (long)timeLimitSec;
		timeLimitSpec.tv_nsec = (long)(1e9 * (timeLimitSec - (double)timeLimitSpec.tv_sec));

		pthread_mutex_lock(&hbMutex);
		condition = pthread_cond_timedwait(&nmi->hbWakeCondition, &hbMutex, &timeLimitSpec);
		pthread_mutex_unlock(&hbMutex);
	}

	jausMessageDestroy(txMessage);	
	reportHeartbeatPulseMessageDestroy(heartbeat);
	nmi->heartbeatThreadRunning = 0;
	
	return NULL;
}

void *receiveThread(void *threadArgument)
{
	int index;
	
	NodeManagerInterface nmi = (NodeManagerInterface)threadArgument;
	DatagramPacket packet;

	JausMessage message;

	nmi->receiveThreadRunning = 1;

	nmi->receiveCount = 0;
	
	packet = datagramPacketCreate();

	packet->bufferSizeBytes = JAUS_HEADER_SIZE_BYTES + JAUS_MAX_DATA_SIZE_BYTES;
	packet->buffer = (unsigned char*)malloc(packet->bufferSizeBytes);
	memset(packet->buffer, 0, packet->bufferSizeBytes);
	
	while(nmi->isOpen)
	{
		if(datagramSocketReceive(nmi->messageSocket, packet) > 0)
		{	
			index = 0;
			if(!strncmp((char *)packet->buffer, JAUS_OPC_UDP_HEADER, JAUS_OPC_UDP_HEADER_SIZE_BYTES)) // equals 1 if same
			{
				index += JAUS_OPC_UDP_HEADER_SIZE_BYTES;
			}

			message = jausMessageCreate();
			if(jausMessageFromBuffer(message, packet->buffer + index, packet->bufferSizeBytes - index))
			{
				if(message->dataFlag)
				{
					lmHandlerReceiveLargeMessage(nmi, message);
				}
				else
				{
					if(message->properties.scFlag)
					{
						if(	(message->commandCode >= JAUS_CREATE_SERVICE_CONNECTION && 
							message->commandCode <= JAUS_TERMINATE_SERVICE_CONNECTION) ||
							message->commandCode == JAUS_CREATE_EVENT ||
							message->commandCode == JAUS_CONFIRM_EVENT_REQUEST ||
							message->commandCode == JAUS_CANCEL_EVENT)
						{
							// This is to take Service Connection Control messages and send them on through
							// to the regular receiveQueue. JAUS 3.2 RA says to set the properties.scFlag bit if it is
							// a Service Connection Control message, but logically they do not need to go
							// to the scManager and instead to the component
							queuePush(nmi->receiveQueue, (void *)message);
						}
						else
						{
							scManagerReceiveMessage(nmi, message);
						}
					}
					else
					{
						queuePush(nmi->receiveQueue, (void *)message);
					}
				}
				pthread_cond_signal(&nmi->recvCondition);
				nmi->receiveCount++;
			}
			else
			{
				jausMessageDestroy(message);
			}
		}
	}
	
	free(packet->buffer);

	datagramPacketDestroy(packet);

	nmi->receiveThreadRunning = 0;
	
	return NULL;
}

JausAddressList *nodeManagerGetComponentAddressList(NodeManagerInterface nmi, unsigned char componentId)
{	
	JausAddressList *addressList = NULL;
	JausAddressList *currentAddress = NULL;
	DatagramPacket packet;

	if(!nmi || !nmi->isOpen || !componentId)
	{
		return NULL;
	}

	packet = datagramPacketCreate();

	packet->bufferSizeBytes = INTERFACE_MESSAGE_SIZE_BYTES;
	packet->buffer = (unsigned char *)malloc(packet->bufferSizeBytes);
	memset(packet->buffer, 0, packet->bufferSizeBytes);	
	packet->buffer[0] = INTERFACE_MESSAGE_GET_COMPONENT_ADDRESS_LIST;
	packet->buffer[1] = componentId;
	packet->buffer[2] = 0;
	packet->buffer[3] = 0;
	packet->buffer[4] = 0;

	packet->port = NODE_MANAGER_INTERFACE_PORT;
	packet->address->value = nmi->ipAddress->value;
	
	datagramSocketSend(nmi->interfaceSocket, packet);
	datagramSocketReceive(nmi->interfaceSocket, packet);

	while(packet->buffer[0] == INTERFACE_MESSAGE_COMPONENT_ADDRESS_LIST_RESPONSE && packet->buffer[4])
	{
		if(currentAddress)
		{
			currentAddress->nextAddress = (JausAddressList *)malloc(sizeof(JausAddressList));
			currentAddress = currentAddress->nextAddress;
			currentAddress->address = jausAddressCreate();
			currentAddress->address->instance = packet->buffer[1];
			currentAddress->address->component = packet->buffer[2];
			currentAddress->address->node = packet->buffer[3];
			currentAddress->address->subsystem = packet->buffer[4];			
			currentAddress->nextAddress = NULL;
		}
		else
		{
			addressList = (JausAddressList *)malloc(sizeof(JausAddressList));
			currentAddress = addressList;
			currentAddress->address = jausAddressCreate();
			currentAddress->address->instance = packet->buffer[1];
			currentAddress->address->component = packet->buffer[2];
			currentAddress->address->node = packet->buffer[3];
			currentAddress->address->subsystem = packet->buffer[4];			
			currentAddress->nextAddress = NULL;
		}

		packet->buffer[0] = INTERFACE_MESSAGE_GET_COMPONENT_ADDRESS_LIST;
		packet->buffer[1] = 0;
		packet->buffer[2] = 0;
		packet->buffer[3] = 0;
		packet->buffer[4] = 0;

		datagramSocketSend(nmi->interfaceSocket, packet);
		datagramSocketReceive(nmi->interfaceSocket, packet);
	}
	
	free(packet->buffer);
	datagramPacketDestroy(packet);

	return addressList;	
}

JausBoolean nodeManagerLookupAddress(NodeManagerInterface nmi, JausAddress lookupAddress)
{	
	DatagramPacket packet;

	if(!nmi || !nmi->isOpen)
	{
		return JAUS_FALSE;
	}

	packet = datagramPacketCreate();

	packet->bufferSizeBytes = INTERFACE_MESSAGE_SIZE_BYTES;
	packet->buffer = (unsigned char *)malloc(packet->bufferSizeBytes);
	memset(packet->buffer, 0, packet->bufferSizeBytes);	
	packet->buffer[0] = INTERFACE_MESSAGE_LOOKUP_ADDRESS;
	packet->buffer[1] = lookupAddress->instance;
	packet->buffer[2] = lookupAddress->component;
	packet->buffer[3] = lookupAddress->node;
	packet->buffer[4] = lookupAddress->subsystem;

	packet->port = NODE_MANAGER_INTERFACE_PORT;
	packet->address->value = nmi->ipAddress->value;
	
	datagramSocketSend(nmi->interfaceSocket, packet);
	datagramSocketReceive(nmi->interfaceSocket, packet);

	if(	packet->buffer[0] == INTERFACE_MESSAGE_LOOKUP_ADDRESS_RESPONSE && packet->buffer[5])
	{
		lookupAddress->instance = packet->buffer[1];
		lookupAddress->component = packet->buffer[2];
		lookupAddress->node = packet->buffer[3];
		lookupAddress->subsystem = packet->buffer[4];			
		
		free(packet->buffer);
		datagramPacketDestroy(packet);
		return JAUS_TRUE;
	}
	else
	{
		free(packet->buffer);
		datagramPacketDestroy(packet);
		return JAUS_FALSE;
	}
}

JausAddressList *nodeManagerLookupServiceAddressList(NodeManagerInterface nmi, JausAddress lookupAddress, unsigned short commandCode, int serviceCommandType)
{	
	JausAddressList *addressList = NULL;
	JausAddressList *currentAddress = NULL;
	DatagramPacket packet;

	if(!nmi || !nmi->isOpen || !commandCode)
	{
		return NULL;
	}

	packet = datagramPacketCreate();

	packet->bufferSizeBytes = INTERFACE_MESSAGE_SIZE_BYTES;
	packet->buffer = (unsigned char *)malloc(packet->bufferSizeBytes);
	memset(packet->buffer, 0, packet->bufferSizeBytes);	
	packet->buffer[0] = INTERFACE_MESSAGE_LOOKUP_SERVICE_ADDRESS_LIST;
	packet->buffer[1] = lookupAddress->instance;
	packet->buffer[2] = lookupAddress->component;
	packet->buffer[3] = lookupAddress->node;
	packet->buffer[4] = lookupAddress->subsystem;
	packet->buffer[5] = (unsigned char)(commandCode & 0xFF);
	packet->buffer[6] = (unsigned char)((commandCode & 0xFF00) >> 8);
	packet->buffer[7] = (unsigned char)(serviceCommandType & 0xFF);

	packet->port = NODE_MANAGER_INTERFACE_PORT;
	packet->address->value = nmi->ipAddress->value;
	
	datagramSocketSend(nmi->interfaceSocket, packet);
	datagramSocketReceive(nmi->interfaceSocket, packet);

	while(packet->buffer[0] == INTERFACE_MESSAGE_COMPONENT_ADDRESS_LIST_RESPONSE && packet->buffer[5])
	{
		if(currentAddress)
		{
			currentAddress->nextAddress = (JausAddressList *)malloc(sizeof(JausAddressList));
			currentAddress = currentAddress->nextAddress;
			currentAddress->address = jausAddressCreate();
			currentAddress->address->instance = packet->buffer[1];
			currentAddress->address->component = packet->buffer[2];
			currentAddress->address->node = packet->buffer[3];
			currentAddress->address->subsystem = packet->buffer[4];			
			currentAddress->nextAddress = NULL;
		}
		else
		{
			addressList = (JausAddressList *)malloc(sizeof(JausAddressList));
			currentAddress = addressList;
			currentAddress->address = jausAddressCreate();
			currentAddress->address->instance = packet->buffer[1];
			currentAddress->address->component = packet->buffer[2];
			currentAddress->address->node = packet->buffer[3];
			currentAddress->address->subsystem = packet->buffer[4];			
			currentAddress->nextAddress = NULL;
		}

		packet->buffer[0] = INTERFACE_MESSAGE_GET_COMPONENT_ADDRESS_LIST;
		packet->buffer[1] = lookupAddress->instance;
		packet->buffer[2] = lookupAddress->component;
		packet->buffer[3] = lookupAddress->node;
		packet->buffer[4] = lookupAddress->subsystem;
		packet->buffer[5] = 0;
		packet->buffer[6] = 0;
		packet->buffer[7] = (unsigned char)(serviceCommandType & 0xFF);

		datagramSocketSend(nmi->interfaceSocket, packet);
		datagramSocketReceive(nmi->interfaceSocket, packet);
	}
	
	free(packet->buffer);
	datagramPacketDestroy(packet);

	return addressList;	
}

JausBoolean nodeManagerLookupServiceAddress(NodeManagerInterface nmi, JausAddress lookupAddress, unsigned short commandCode, int serviceCommandType)
{	
	DatagramPacket packet;

	if(!nmi || !nmi->isOpen)
	{
		return JAUS_FALSE;
	}

	packet = datagramPacketCreate();

	packet->bufferSizeBytes = INTERFACE_MESSAGE_SIZE_BYTES;
	packet->buffer = (unsigned char *)malloc(packet->bufferSizeBytes);
	memset(packet->buffer, 0, packet->bufferSizeBytes);	
	packet->buffer[0] = INTERFACE_MESSAGE_LOOKUP_SERVICE_ADDRESS;
	packet->buffer[1] = lookupAddress->instance;
	packet->buffer[2] = lookupAddress->component;
	packet->buffer[3] = lookupAddress->node;
	packet->buffer[4] = lookupAddress->subsystem;
	packet->buffer[5] = (unsigned char)(commandCode & 0xFF);
	packet->buffer[6] = (unsigned char)((commandCode & 0xFF00) >> 8);
	packet->buffer[7] = (unsigned char)(serviceCommandType & 0xFF);

	packet->port = NODE_MANAGER_INTERFACE_PORT;
	packet->address->value = nmi->ipAddress->value;
	
	datagramSocketSend(nmi->interfaceSocket, packet);
	datagramSocketReceive(nmi->interfaceSocket, packet);

	if(	packet->buffer[0] == INTERFACE_MESSAGE_LOOKUP_SERVICE_ADDRESS_RESPONSE && packet->buffer[5])
	{
		lookupAddress->instance = packet->buffer[1];
		lookupAddress->component = packet->buffer[2];
		lookupAddress->node = packet->buffer[3];
		lookupAddress->subsystem = packet->buffer[4];			
		
		free(packet->buffer);
		datagramPacketDestroy(packet);
		return JAUS_TRUE;
	}
	else
	{
		free(packet->buffer);
		datagramPacketDestroy(packet);
		return JAUS_FALSE;
	}
}

void nodeManagerDestroyAddressList(JausAddressList *addressList)
{
	JausAddressList *deadAddressList;

	while(addressList)
	{
		deadAddressList = addressList;
		addressList = addressList->nextAddress;
		
		jausAddressDestroy(deadAddressList->address);
		free(deadAddressList);
	}
}

int nodeManagerVerifyAddress(NodeManagerInterface nmi, JausAddress address)
{
	DatagramPacket packet;
	int response = 0;

	packet = datagramPacketCreate();

	packet->bufferSizeBytes = INTERFACE_MESSAGE_SIZE_BYTES;
	packet->buffer = (unsigned char *)malloc(packet->bufferSizeBytes);
	memset(packet->buffer, 0, packet->bufferSizeBytes);	
	packet->buffer[0] = INTERFACE_MESSAGE_VERIFY_ADDRESS;
	packet->buffer[1] = (unsigned char)address->instance;
	packet->buffer[2] = (unsigned char)address->component;
	packet->buffer[3] = (unsigned char)address->node;
	packet->buffer[4] = (unsigned char)address->subsystem;

	packet->port = NODE_MANAGER_INTERFACE_PORT;
	packet->address->value = nmi->ipAddress->value;
	
	datagramSocketSend(nmi->interfaceSocket, packet);
	datagramSocketReceive(nmi->interfaceSocket, packet);

	if(packet->buffer[0] == INTERFACE_MESSAGE_ADDRESS_VERIFIED)
	{
		response = packet->buffer[1];

		free(packet->buffer);
		datagramPacketDestroy(packet);
		return response;
	}
	else
	{
		free(packet->buffer);
		datagramPacketDestroy(packet);
		return 0;
	}
		
}

int nodeManagerReceive(NodeManagerInterface nmi, JausMessage *message)
{
	if(nmi->isOpen && nmi->receiveQueue->size)
	{
		*message = (JausMessage)queuePop(nmi->receiveQueue);
		return 1;		
	}
	else
	{
		return 0;
	}
}

int nodeManagerTimedReceive(NodeManagerInterface nmi, JausMessage *message, double timeLimitSec)
{
	pthread_mutex_t recvMutex = PTHREAD_MUTEX_INITIALIZER;
	int condition = -1;
	struct timespec timeLimitSpec;
		
	if(nmi->isOpen)
	{
		if(ojGetTimeSec() > timeLimitSec)
		{
			return NMI_RECEIVE_TIMED_OUT;			
		}
		else if(nmi->receiveQueue->size)
		{
			*message = (JausMessage)queuePop(nmi->receiveQueue);
			return NMI_MESSAGE_RECEIVED;
		}
		else
		{
			timeLimitSpec.tv_sec = (long)timeLimitSec;
			timeLimitSpec.tv_nsec = (long)(1e9 * (timeLimitSec - (double)timeLimitSpec.tv_sec));

			pthread_mutex_lock(&recvMutex);
			condition = pthread_cond_timedwait(&nmi->recvCondition, &recvMutex, &timeLimitSpec);
			pthread_mutex_unlock(&recvMutex);
			
			switch(condition)
			{
				case 0: // Conditional Signaled
					*message = (JausMessage)queuePop(nmi->receiveQueue);
					return NMI_MESSAGE_RECEIVED;
				
				case ETIMEDOUT: // our time is up
					return NMI_RECEIVE_TIMED_OUT;
					
				default: // Some other error occured
					return NMI_CONDITIONAL_WAIT_ERROR;
			}
		}
	}
	else
	{
		return NMI_CLOSED_ERROR;
	}
}

int nodeManagerSend(NodeManagerInterface nmi, JausMessage message)
{
	int result = -1;
	
	if(nmi->isOpen)
	{
		result = lmHandlerSendLargeMessage(nmi, message);
	}
		
	return result;
}

int nodeManagerSendSingleMessage(NodeManagerInterface nmi, JausMessage message)
{
	DatagramPacket packet;
	int result = -1;
	
	if(nmi->isOpen)
	{
		packet = datagramPacketCreate();
		packet->bufferSizeBytes = (int)jausMessageSize(message);
		packet->buffer = (unsigned char*)malloc(packet->bufferSizeBytes);
		packet->port = NODE_MANAGER_MESSAGE_PORT;
		packet->address->value = nmi->ipAddress->value;
		memset(packet->buffer, 0, packet->bufferSizeBytes);
		
		if(jausMessageToBuffer(message, packet->buffer, packet->bufferSizeBytes))
		{
			result = datagramSocketSend(nmi->messageSocket, packet);
		}
		free(packet->buffer);
		datagramPacketDestroy(packet);
	}
		
	return result;
}

void nodeManagerSendCoreServiceConnections(NodeManagerInterface nmi)
{
	JausMessage message = NULL;
	ServiceConnection scList = NULL;
	ServiceConnection sc;
	ReportComponentStatusMessage reportStatus;	
	ReportComponentAuthorityMessage reportAuthority;	

	// Respond to a ReportComponentStatus Service Connection
	scList = scManagerGetSendList(nmi, JAUS_REPORT_COMPONENT_STATUS);
	if(scList)
	{
		reportStatus = reportComponentStatusMessageCreate();
		jausAddressCopy(reportStatus->source, nmi->cmpt->address);
		reportStatus->properties.scFlag = JAUS_SERVICE_CONNECTION_MESSAGE;
		reportStatus->primaryStatusCode = nmi->cmpt->state;

		sc = scList;
		while(sc)
		{
				jausAddressCopy(reportStatus->destination, sc->address);
				reportStatus->sequenceNumber = sc->sequenceNumber;

				message = reportComponentStatusMessageToJausMessage(reportStatus);
				nodeManagerSend(nmi, message);
				jausMessageDestroy(message);

				sc = sc->nextSc;
		}
		reportComponentStatusMessageDestroy(reportStatus);
	}
	scManagerDestroySendList(scList);

	scList = NULL;
	// Respond to a ReportComponentAuthorityMessage Service Connection
	scList = scManagerGetSendList(nmi, JAUS_REPORT_COMPONENT_AUTHORITY);
	if(scList)
	{
		reportAuthority = reportComponentAuthorityMessageCreate();
		jausAddressCopy(reportAuthority->source, nmi->cmpt->address);
		reportAuthority->properties.scFlag = JAUS_SERVICE_CONNECTION_MESSAGE;
		reportAuthority->authorityCode = nmi->cmpt->authority;

		sc = scList;
		while(sc)
		{
				jausAddressCopy(reportAuthority->destination, sc->address);
				reportAuthority->sequenceNumber = sc->sequenceNumber;

				message = reportComponentAuthorityMessageToJausMessage(reportAuthority);
				nodeManagerSend(nmi, message);
				jausMessageDestroy(message);

				sc = sc->nextSc;
		}
		reportComponentAuthorityMessageDestroy(reportAuthority);
	}
	scManagerDestroySendList(scList);
}
