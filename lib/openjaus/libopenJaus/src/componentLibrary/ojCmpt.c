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
// File Name: ojCmpt.c
//
// Written By: Tom Galluzzo (galluzzo AT gmail DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file contains the code associated with the OpenJAUS
//				Component Library. The Component Library is a wrapper around
//				common component functionality which accelerates and eases
//				the creation of JAUS components.

#include <stdio.h>
#include <stdlib.h>
#include <jaus.h>
#include "nodeManagerInterface/nodeManagerInterface.h"
#include "componentLibrary/ojCmpt.h"

typedef	struct 
{
	void (*function)(OjCmpt, JausMessage);
	unsigned short commandCode;
}MessageCallback;				// An array to hold the pointers to the message callback functions

struct OjCmptStruct
{
	double frequencyHz;		// Desired frequency of the component 
	double rateHz;		// Actual running frequency of the component 

	JausComponent jaus;				// A pointer to the JausComponent structure found in the OpenJAUS libjaus library.

	pthread_t thread;			// Pointer to the pthread that is running the component thread
	pthread_mutex_t scMutex;	// TODO: Lock mutex when accessing service connection arrays

	void (*stateCallback[OJ_CMPT_MAX_STATE_COUNT])(OjCmpt);	// An array to hold the pointers to the callback functions
	void (*mainCallback)(OjCmpt);					
	void (*processMessageCallback)(OjCmpt, JausMessage);			
	
	MessageCallback *messageCallback;				// An array to hold the pointers to the message callback functions
	int messageCallbackCount;

	int state;
	int run;

	ServiceConnection inConnection[OJ_CMPT_MAX_INCOMING_SC_COUNT];

	NodeManagerInterface nmi;	
	
	void *userData;
};

void* ojCmptThread(void *threadData);
void ojCmptProcessMessage(OjCmpt ojCmpt, JausMessage message);
void ojCmptManageServiceConnections(OjCmpt ojCmpt);

OjCmpt ojCmptCreate(char *name, JausByte id, double stateFrequencyHz)
{
	OjCmpt ojCmpt;
	int i;

	ojCmpt = (OjCmpt)malloc(sizeof(struct OjCmptStruct));

	ojCmpt->jaus = jausComponentCreate();
	ojCmpt->jaus->identification = (char *)calloc(strlen(name) + 1, 1);
	strcpy(ojCmpt->jaus->identification, name);
	ojCmpt->jaus->address->component = id;
	ojCmptSetState(ojCmpt, JAUS_UNDEFINED_STATE);
	ojCmptSetFrequencyHz(ojCmpt, stateFrequencyHz);

	for(i=0; i<OJ_CMPT_MAX_STATE_COUNT; i++)
	{
		ojCmpt->stateCallback[i] = NULL;
	}
	ojCmpt->mainCallback = NULL;
	ojCmpt->processMessageCallback = NULL;
	ojCmpt->messageCallback = NULL;
	ojCmpt->messageCallbackCount = 0;
	
	ojCmpt->run = FALSE;

	if (!jausServiceAddCoreServices(ojCmpt->jaus->services))	// Add core services
	{
		free(ojCmpt->jaus->identification);
		ojCmpt->jaus->identification = NULL;
		jausComponentDestroy(ojCmpt->jaus);
		free(ojCmpt);
		return NULL;
	}

	for(i=0; i<OJ_CMPT_MAX_INCOMING_SC_COUNT; i++)
	{
		ojCmpt->inConnection[i] = NULL;
	}
	
	ojCmpt->nmi = nodeManagerOpen(ojCmpt->jaus);
	if(ojCmpt->nmi == NULL)
	{
		free(ojCmpt->jaus->identification);
		ojCmpt->jaus->identification = NULL;
		jausComponentDestroy(ojCmpt->jaus);
		free(ojCmpt);
		return NULL; 
	}

	return ojCmpt;
}

int ojCmptRun(OjCmpt ojCmpt)
{
	pthread_attr_t attr;	// Thread attributed for the component threads spawned in this function

	ojCmpt->run = TRUE;

	pthread_attr_init(&attr);
	if(pthread_create(&ojCmpt->thread, &attr, ojCmptThread, (void*)ojCmpt) != 0)
	{
		nodeManagerClose(ojCmpt->nmi); // Close Node Manager Connection
		free(ojCmpt->jaus->identification);
		ojCmpt->jaus->identification = NULL;
		jausComponentDestroy(ojCmpt->jaus);
		free(ojCmpt);
		pthread_attr_destroy(&attr);
		return -1;
	}
	pthread_attr_destroy(&attr);

	return 0;
}

void ojCmptDestroy(OjCmpt ojCmpt)
{
	RejectComponentControlMessage rejectComponentControl;
	JausMessage txMessage;
	int i = 0;
	
	if(ojCmpt->run == TRUE)
	{
		ojCmpt->run = FALSE;
		pthread_cond_signal(&ojCmpt->nmi->recvCondition);
		pthread_join(ojCmpt->thread, NULL);
	}

	if(ojCmpt->jaus->controller.active)
	{
		// Terminate control of current component
		rejectComponentControl = rejectComponentControlMessageCreate();
		jausAddressCopy(rejectComponentControl->source, ojCmpt->jaus->address);
		jausAddressCopy(rejectComponentControl->destination, ojCmpt->jaus->controller.address);

		txMessage = rejectComponentControlMessageToJausMessage(rejectComponentControl);
		nodeManagerSend(ojCmpt->nmi, txMessage);
		jausMessageDestroy(txMessage);

		rejectComponentControlMessageDestroy(rejectComponentControl);
	}
	
	for(i=0; i<OJ_CMPT_MAX_INCOMING_SC_COUNT; i++)
	{
		if(ojCmpt->inConnection[i])
		{	
			if(ojCmpt->inConnection[i]->isActive)
			{
				scManagerTerminateServiceConnection(ojCmpt->nmi, ojCmpt->inConnection[i]);
			}
			serviceConnectionDestroy(ojCmpt->inConnection[i]);		
		}
	}
	
	if(ojCmpt->messageCallback)
	{
		free(ojCmpt->messageCallback);
	}
	
	if(ojCmpt->nmi)
	{
		nodeManagerClose(ojCmpt->nmi); // Close Node Manager Connection
	}
	
	free(ojCmpt->jaus->identification);
	ojCmpt->jaus->identification = NULL;
	jausComponentDestroy(ojCmpt->jaus);
	free(ojCmpt);	
};

void ojCmptSetFrequencyHz(OjCmpt ojCmpt, double stateFrequencyHz)
{
	if(stateFrequencyHz > OJ_CMPT_MIN_FREQUENCY_HZ && stateFrequencyHz < OJ_CMPT_MAX_FREQUENCY_HZ)
	{
		ojCmpt->frequencyHz = stateFrequencyHz;
	}
	else
	{
		ojCmpt->frequencyHz = OJ_CMPT_DEFAULT_FREQUENCY_HZ;
	}
}

void ojCmptSetState(OjCmpt ojCmpt, int state)
{
	if(state < 0 || state > OJ_CMPT_MAX_STATE_COUNT)
	{
		ojCmpt->state = JAUS_UNDEFINED_STATE;
		ojCmpt->jaus->state = JAUS_UNDEFINED_STATE;
	}
	else
	{
		ojCmpt->state = state;
		if(state > JAUS_EMERGENCY_STATE)
		{
			ojCmpt->jaus->state = JAUS_UNDEFINED_STATE;
		}
		else
		{
			ojCmpt->jaus->state = state;
		}
	}
}

void ojCmptSetAuthority(OjCmpt ojCmpt, JausByte authority)
{
	ojCmpt->jaus->authority = authority;
}

JausByte ojCmptGetAuthority(OjCmpt ojCmpt)
{
	return ojCmpt->jaus->authority;
}

int ojCmptSetStateCallback(OjCmpt ojCmpt, int state, void (*stateCallbackFunction)(OjCmpt))			// Calls method from stateHandler
{
	if(state < 0 || state > OJ_CMPT_MAX_STATE_COUNT)
	{
		return -1;
	}
	else
	{
		ojCmpt->stateCallback[state] = stateCallbackFunction;
		return 0;
	}
}

void ojCmptSetMessageCallback(OjCmpt ojCmpt, unsigned short commandCode, void (*messageFunction)(OjCmpt, JausMessage))	// Calls method from messageHandler
{
	int i;

	for(i=0; i<ojCmpt->messageCallbackCount; i++)
	{
		if(ojCmpt->messageCallback[i].commandCode == commandCode)
		{
			ojCmpt->messageCallback[i].function = messageFunction;
			return;
		}
	}

	if(ojCmpt->messageCallback)
	{
		ojCmpt->messageCallbackCount++;
		ojCmpt->messageCallback = (MessageCallback *)realloc(ojCmpt->messageCallback, ojCmpt->messageCallbackCount * sizeof(MessageCallback));
	}
	else
	{
		ojCmpt->messageCallbackCount = 1;
		ojCmpt->messageCallback = (MessageCallback *)malloc(sizeof(MessageCallback));
	}

	ojCmpt->messageCallback[ojCmpt->messageCallbackCount - 1].commandCode = commandCode;
	ojCmpt->messageCallback[ojCmpt->messageCallbackCount - 1].function = messageFunction;
}

void ojCmptSetMessageProcessorCallback(OjCmpt ojCmpt, void (*processMessageFunction)(OjCmpt, JausMessage))	// Calls method from messageHandler
{
	ojCmpt->processMessageCallback = processMessageFunction;
}

void ojCmptSetUserData(OjCmpt ojCmpt, void *data)
{
	ojCmpt->userData = data;
}

void *ojCmptGetUserData(OjCmpt ojCmpt)
{
	return ojCmpt->userData;
}

void ojCmptProcessMessage(OjCmpt ojCmpt, JausMessage message)
{
	int i = 0;
	
	for(i=0; i<ojCmpt->messageCallbackCount; i++)
	{
		if(ojCmpt->messageCallback[i].commandCode == message->commandCode && ojCmpt->messageCallback[i].function)
		{
			ojCmpt->messageCallback[i].function(ojCmpt, message);
			i = -1;
			break;
		}
	}
	
	if(i == ojCmpt->messageCallbackCount)
	{
		if(ojCmpt->processMessageCallback)
		{
			ojCmpt->processMessageCallback(ojCmpt, message);
		}
		else
		{
			defaultJausMessageProcessorNoDestroy(message, ojCmpt->nmi, ojCmpt->jaus);
		}
	}
	
	jausMessageDestroy(message);
}

double ojCmptGetRateHz(OjCmpt ojCmpt)
{
	return ojCmpt->rateHz;
}

void* ojCmptThread(void *threadData)
{
	int i = 0;
	OjCmpt ojCmpt;
	JausMessage rxMessage;
	double prevTime = 0;
	double time = ojGetTimeSec();
	double nextStateTime = ojGetTimeSec();

	// Get handle to OpenJausComponent that was created 
	ojCmpt = (OjCmpt)threadData;
	
	while(ojCmpt->run) // Execute state machine code while not in the SHUTDOWN state
	{
		switch(nodeManagerTimedReceive(ojCmpt->nmi, &rxMessage, nextStateTime))
		{
			case NMI_MESSAGE_RECEIVED:
				// If we were sent a message
				if(rxMessage)
				{	
					ojCmptProcessMessage(ojCmpt, rxMessage);
				}
				
				// Always check for messages on incoming SC queues
				for(i=0; i<OJ_CMPT_MAX_INCOMING_SC_COUNT; i++)
				{
					if(ojCmpt->inConnection[i] && ojCmpt->inConnection[i]->isActive) // Attempt to process incoming message
					{
						if(scManagerReceiveServiceConnection(ojCmpt->nmi, ojCmpt->inConnection[i], &rxMessage))
						{
							ojCmptProcessMessage(ojCmpt, rxMessage);
						}
					}
				}
				break;
				
			case NMI_RECEIVE_TIMED_OUT:
				prevTime = time;
				time = ojGetTimeSec();
				ojCmpt->rateHz = 1.0/(time-prevTime); // Compute the update rate of this thread
				nextStateTime = time + 1.0/ojCmpt->frequencyHz;
				
				if(ojCmpt->mainCallback)
				{
					ojCmpt->mainCallback(ojCmpt);
				}

				if(ojCmpt->state != JAUS_UNDEFINED_STATE && ojCmpt->stateCallback[ojCmpt->state])
				{
					ojCmpt->stateCallback[ojCmpt->state](ojCmpt);
				}
				
				ojCmptManageServiceConnections(ojCmpt);
				nodeManagerSendCoreServiceConnections(ojCmpt->nmi);
				break;
				
			case NMI_CONDITIONAL_WAIT_ERROR:
				printf("ojCmpt.c : Conditional Wait Error in nodeManagerTimedReceive, Exiting ojCmpt Thread\n");
				ojCmpt->run = FALSE;
				break;

			case NMI_CLOSED_ERROR:
				printf("ojCmpt.c : Node Manager Interface closed when calling nodeManagerTimedReceive, Exiting ojCmpt Thread\n");
				ojCmpt->run = FALSE;
				break;

			default:
				printf("ojCmpt.c : Error in nodeManagerTimedReceive, Exiting ojCmpt Thread\n");
				ojCmpt->run = FALSE;
				break;
		}
				
	}	

	return NULL;
}

char* ojCmptGetName(OjCmpt ojCmpt)
{
	char *name;

	name = (char *)malloc(strlen(ojCmpt->jaus->identification)+1);
	strcpy(name, ojCmpt->jaus->identification);

	return name;
}

JausAddress ojCmptGetAddress(OjCmpt ojCmpt)
{
	return jausAddressClone(ojCmpt->jaus->address);
}

JausAddress ojCmptGetControllerAddress(OjCmpt ojCmpt)
{
	if(ojCmptHasController(ojCmpt))
	{
		return jausAddressClone(ojCmpt->jaus->controller.address);			
	}
	else
	{
		return jausAddressCreate();
	}
}

JausBoolean ojCmptHasController(OjCmpt ojCmpt)
{
	return ojCmpt->jaus->controller.active;
}

void ojCmptDefaultMessageProcessor(OjCmpt ojCmpt, JausMessage message)
{
	defaultJausMessageProcessorNoDestroy(message, ojCmpt->nmi, ojCmpt->jaus);
}

int ojCmptGetState(OjCmpt ojCmpt)
{
	return ojCmpt->state;
}

int ojCmptSendMessage(OjCmpt ojCmpt, JausMessage message)
{
	jausAddressCopy(message->source, ojCmpt->jaus->address);
	return nodeManagerSend(ojCmpt->nmi, message);
}

JausBoolean ojCmptAddService(OjCmpt ojCmpt, JausUnsignedShort serviceType)
{
	JausService service = jausServiceCreate(serviceType); 
	if(service)
	{
		return jausServiceAddService(ojCmpt->jaus->services, service);
	}
	return JAUS_FALSE;
}

JausBoolean ojCmptAddServiceInputMessage(OjCmpt ojCmpt, JausUnsignedShort serviceType, JausUnsignedShort commandCode, JausUnsignedInteger presenceVector)
{
	JausService service = jausServiceRetrieveService(ojCmpt->jaus->services, serviceType);
	
	if(service)
	{
		return jausServiceAddInputCommand(service, commandCode, presenceVector);
	}
	return JAUS_FALSE;
}

JausBoolean ojCmptAddServiceOutputMessage(OjCmpt ojCmpt, JausUnsignedShort serviceType, JausUnsignedShort commandCode, JausUnsignedInteger presenceVector)
{
	JausService service = jausServiceRetrieveService(ojCmpt->jaus->services, serviceType);
	
	if(service)
	{
		return jausServiceAddOutputCommand(service, commandCode, presenceVector);
	}
	return JAUS_FALSE;
}

void ojCmptAddSupportedSc(OjCmpt ojCmpt, unsigned short commandCode)		// Add service connection support for this message
{
	scManagerAddSupportedMessage(ojCmpt->nmi, commandCode);
}

void ojCmptRemoveSupportedSc(OjCmpt ojCmpt, unsigned short commandCode)	// Removes service connection support for this message
{
	scManagerRemoveSupportedMessage(ojCmpt->nmi, commandCode);
}

ServiceConnection ojCmptGetScSendList(OjCmpt ojCmpt, unsigned short commandCode)
{
	return scManagerGetSendList(ojCmpt->nmi, commandCode);
}

void ojCmptDestroySendList(ServiceConnection scList)
{
	scManagerDestroySendList(scList);
}

JausBoolean ojCmptIsOutgoingScActive(OjCmpt ojCmpt, unsigned short commandCode)
{
	return scManagerQueryActiveMessage(ojCmpt->nmi, commandCode);
}

// Incoming Service Connections
void ojCmptManageServiceConnections(OjCmpt ojCmpt)
{
	int i = 0;
	double time = ojGetTimeSec();
	
	// Manage Incoming Connections
	for(i=0; i<OJ_CMPT_MAX_INCOMING_SC_COUNT; i++)
	{
		// If not active then attempt to initiate SC
		if(ojCmpt->inConnection[i] && !ojCmpt->inConnection[i]->isActive && time > ojCmpt->inConnection[i]->nextRequestTimeSec)
		{
			// set up the service connection
			scManagerCreateServiceConnection(ojCmpt->nmi, ojCmpt->inConnection[i]);
			ojCmpt->inConnection[i]->nextRequestTimeSec = time + ojCmpt->inConnection[i]->timeoutSec;
		}
	}
}

int ojCmptEstablishSc(OjCmpt ojCmpt, JausUnsignedShort cCode, JausUnsignedInteger pv, JausAddress address, double rateHz, double timeoutSec, int qSize)
{
	int i = 0;
	
	for(i=0; i<OJ_CMPT_MAX_INCOMING_SC_COUNT; i++)
	{
		if(ojCmpt->inConnection[i] == NULL)
		{
			break;
		}
	}

	if(i < OJ_CMPT_MAX_INCOMING_SC_COUNT)
	{
		ojCmpt->inConnection[i] = serviceConnectionCreate();
		ojCmpt->inConnection[i]->requestedUpdateRateHz = rateHz;
		// TODO: Locate Command Code service in system based on address
		jausAddressCopy(ojCmpt->inConnection[i]->address, address);
		ojCmpt->inConnection[i]->presenceVector = pv;
		ojCmpt->inConnection[i]->commandCode = cCode;
		ojCmpt->inConnection[i]->isActive = JAUS_FALSE;
		ojCmpt->inConnection[i]->queueSize = qSize;
		ojCmpt->inConnection[i]->timeoutSec = timeoutSec;
		ojCmpt->inConnection[i]->nextRequestTimeSec = 0;
		return i;
	}

	return -1;	
}

int ojCmptTerminateSc(OjCmpt ojCmpt, int scIndex)
{	
	if(scIndex < 0 || scIndex >= OJ_CMPT_MAX_INCOMING_SC_COUNT || ojCmpt->inConnection[scIndex] == NULL)
	{
		return FALSE;
	}
	
	if(ojCmpt->inConnection[scIndex]->isActive)
	{	
		scManagerTerminateServiceConnection(ojCmpt->nmi, ojCmpt->inConnection[scIndex]);
	}
	serviceConnectionDestroy(ojCmpt->inConnection[scIndex]);
	ojCmpt->inConnection[scIndex] = NULL;
	
	return TRUE;
}

JausBoolean ojCmptIsIncomingScActive(OjCmpt ojCmpt, int scIndex)
{	
	if(scIndex < 0 || scIndex >= OJ_CMPT_MAX_INCOMING_SC_COUNT || ojCmpt->inConnection[scIndex] == NULL)
	{
		return JAUS_FALSE;
	}
	
	return ojCmpt->inConnection[scIndex]->isActive;
}

int ojCmptLookupAddress(OjCmpt ojCmpt, JausAddress address)
{
	return nodeManagerLookupAddress(ojCmpt->nmi, address);
}
