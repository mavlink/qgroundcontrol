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
// File:		scManager.c
// Version:		3.3 BETA
// Written by:	Tom Galluzzo (galluzzo AT gmail DOT com) and 
//              Danny Kent (jaus AT dannykent DOT com)
// Date:		04/15/08
// Description:	Provides the core service connection management routines

#include <stdlib.h>
#include <pthread.h>
#include "nodeManagerInterface/nodeManagerInterface.h"

SupportedScMessage scFindSupportedScMsgInList(SupportedScMessage, unsigned short);
ServiceConnection scFindScInList(ServiceConnection, ServiceConnection);
int scGetAvailableInstanceId(ServiceConnection, ServiceConnection);

ServiceConnection serviceConnectionCreate(void)
{
	ServiceConnection sc;
	
	sc = (ServiceConnection) malloc(sizeof(ServiceConnectionStruct));
	if(sc)
	{
		sc->address = jausAddressCreate();
		if(!sc->address)
		{
			// Failed to init address
			free(sc);
			return NULL;
		}
		
		sc->queue = queueCreate();
		if(!sc->queue)
		{
			// Failed to init queue
			jausAddressDestroy(sc->address);
			free(sc);
			return NULL;
		}
				
		return sc;
	}
	else
	{
		return NULL;
	}
}

void serviceConnectionDestroy(ServiceConnection sc)
{
	jausAddressDestroy(sc->address);
	queueDestroy(sc->queue, (void *)jausMessageDestroy);
	free(sc);
}

ServiceConnectionManager scManagerCreate(void)
{
	ServiceConnectionManager scm = NULL;
	int retVal;

	scm = (ServiceConnectionManager)malloc(sizeof(ServiceConnectionManagerStruct));
	if(scm == NULL)
	{
		return NULL;
	}
	
	scm->supportedScMsgList = NULL;
	scm->incomingSc = NULL;
	scm->supportedScMsgCount = 0;
	scm->outgoingScCount = 0;
	scm->incomingScCount = 0;
	
	retVal = pthread_mutex_init(&scm->mutex, NULL);
	if(retVal != 0)
	{
		// Failed to init mutex
		free(scm);
		return NULL;
	}
	
	return scm;	
}

void scManagerDestroy(ServiceConnectionManager scm)
{
	SupportedScMessage supportedScMsg;
	ServiceConnection sc;
	
	// Only attempt to destroy the scm if it is a non-NULL pointer
	if(scm == NULL)
	{
		return;
	}
	
	// Free the supported message list
	while(scm->supportedScMsgList)
	{
		supportedScMsg = scm->supportedScMsgList;
		scm->supportedScMsgList = supportedScMsg->nextSupportedScMsg;
		
		// Free all the service connections
		while(supportedScMsg->scList)
		{
			sc = supportedScMsg->scList;
			supportedScMsg->scList = sc->nextSc;
			serviceConnectionDestroy(sc);
		}
		
		free(supportedScMsg);
	}
	
	pthread_mutex_destroy(&scm->mutex);
	free(scm);	
}

void scManagerProcessConfirmScMessage(NodeManagerInterface nmi, ConfirmServiceConnectionMessage message)
{
	ServiceConnection prevSc = NULL;
	ServiceConnection sc;
	TerminateServiceConnectionMessage terminateSc;	
	JausMessage txMessage;
	
	pthread_mutex_lock(&nmi->scm->mutex);
	
	sc = nmi->scm->incomingSc;
	
	while(sc)
	{
		if(	sc->commandCode == message->serviceConnectionCommandCode && jausAddressEqual(sc->address, message->source) )
		{
			if( message->responseCode == JAUS_SC_SUCCESSFUL )
			{
				sc->confirmedUpdateRateHz = message->confirmedPeriodicUpdateRateHertz;
				sc->instanceId = message->instanceId;
				sc->isActive = JAUS_TRUE;
				sc->sequenceNumber = 65535;
				sc->lastSentTime = ojGetTimeSec();
			}
			else
			{
				// Set SC Inactive
				sc->isActive = JAUS_FALSE;

				// Remove Service Connection
				if(prevSc)
				{
					prevSc->nextSc = sc->nextSc;
					sc->nextSc = NULL;
				}
				else
				{
					nmi->scm->incomingSc = sc->nextSc;
					sc->nextSc = NULL;
				}
				nmi->scm->incomingScCount--;
			}
			
			pthread_mutex_unlock(&nmi->scm->mutex);
			return;
		}
		prevSc = sc;
		sc = sc->nextSc;
	}

	// The SC was not found, so send a terminate to prevent streaming
	if( message->responseCode == JAUS_SC_SUCCESSFUL )
	{
		terminateSc = terminateServiceConnectionMessageCreate();
		jausAddressCopy(terminateSc->source, nmi->cmpt->address);
		jausAddressCopy(terminateSc->destination, message->source);
		terminateSc->serviceConnectionCommandCode = message->serviceConnectionCommandCode;
		terminateSc->instanceId = message->instanceId;
		
		txMessage = terminateServiceConnectionMessageToJausMessage(terminateSc);
		nodeManagerSend(nmi, txMessage);
		jausMessageDestroy(txMessage);
	
		terminateServiceConnectionMessageDestroy(terminateSc);
	}
	
	pthread_mutex_unlock(&nmi->scm->mutex);
}

void scManagerProcessCreateScMessage(NodeManagerInterface nmi, CreateServiceConnectionMessage message)
{
	SupportedScMessage supportedScMsg;
	ServiceConnection sc;
	ServiceConnection newSc;
	JausMessage txMessage;
	ConfirmServiceConnectionMessage confScMsg;
	
	pthread_mutex_lock(&nmi->scm->mutex);
	
	supportedScMsg = scFindSupportedScMsgInList(nmi->scm->supportedScMsgList, message->serviceConnectionCommandCode);
	if(supportedScMsg == NULL)
	{
		confScMsg = confirmServiceConnectionMessageCreate();
		jausAddressCopy(confScMsg->source, nmi->cmpt->address);
		jausAddressCopy(confScMsg->destination, message->source);
		confScMsg->serviceConnectionCommandCode = message->serviceConnectionCommandCode;
		confScMsg->instanceId = 0;
		confScMsg->confirmedPeriodicUpdateRateHertz = 0;
		confScMsg->responseCode = JAUS_SC_COMPONENT_NOT_CAPABLE;
	
		txMessage = confirmServiceConnectionMessageToJausMessage(confScMsg); 
		nodeManagerSend(nmi, txMessage);
		jausMessageDestroy(txMessage);	
	
		confirmServiceConnectionMessageDestroy(confScMsg);
		
		pthread_mutex_unlock(&nmi->scm->mutex);
		return;
	}
	
	newSc = (ServiceConnection)malloc( sizeof(ServiceConnectionStruct) );
	if(newSc == NULL) 
	{
		// Send negative conf (could not create sc)
		confScMsg = confirmServiceConnectionMessageCreate();
		jausAddressCopy(confScMsg->source, nmi->cmpt->address);
		jausAddressCopy(confScMsg->destination, message->source);
		confScMsg->serviceConnectionCommandCode = message->serviceConnectionCommandCode;
		confScMsg->instanceId = 0;
		confScMsg->confirmedPeriodicUpdateRateHertz = 0;
		confScMsg->responseCode = JAUS_SC_CONNECTION_REFUSED;
	
		txMessage = confirmServiceConnectionMessageToJausMessage(confScMsg); 
		nodeManagerSend(nmi, txMessage);
		jausMessageDestroy(txMessage);	
	
		confirmServiceConnectionMessageDestroy(confScMsg);
		pthread_mutex_unlock(&nmi->scm->mutex);
		return;
	}
	
	newSc->commandCode = message->serviceConnectionCommandCode;
	newSc->presenceVector = message->presenceVector;
	newSc->address = jausAddressCreate();
	jausAddressCopy(newSc->address, message->source);
	newSc->instanceId = scGetAvailableInstanceId(supportedScMsg->scList, newSc);
	if(newSc->instanceId == -1)
	{
		// Send negative conf (could not create sc)
		confScMsg = confirmServiceConnectionMessageCreate();
		jausAddressCopy(confScMsg->source, nmi->cmpt->address);
		jausAddressCopy(confScMsg->destination, message->source);
		confScMsg->serviceConnectionCommandCode = message->serviceConnectionCommandCode;
		confScMsg->instanceId = 0;
		confScMsg->confirmedPeriodicUpdateRateHertz = 0;
		confScMsg->responseCode = JAUS_SC_CONNECTION_REFUSED;
	
		txMessage = confirmServiceConnectionMessageToJausMessage(confScMsg); 
		nodeManagerSend(nmi, txMessage);
		jausMessageDestroy(txMessage);	
	
		confirmServiceConnectionMessageDestroy(confScMsg);

		jausAddressDestroy(newSc->address);
		free(newSc);
		pthread_mutex_unlock(&nmi->scm->mutex);
		return;
	}
	newSc->queue = NULL;
	newSc->queueSize = 0;

	sc = scFindScInList(supportedScMsg->scList, newSc);
	if(sc == NULL) // Test to see if the sc does not already exist	
	{
		// The sc doesent exist, so we insert the new one into the list
		sc = newSc;
		sc->nextSc = supportedScMsg->scList;
		supportedScMsg->scList = sc;
		nmi->scm->outgoingScCount++;
	}
	else
	{
		jausAddressDestroy(newSc->address);
		free(newSc);
	}

	sc->requestedUpdateRateHz = message->requestedPeriodicUpdateRateHertz;
	sc->lastSentTime = 0.0;
	sc->sequenceNumber = 0;
	sc->isActive = JAUS_TRUE;
	sc->confirmedUpdateRateHz = message->requestedPeriodicUpdateRateHertz;	// TODO: calculate confirmedUpdateRateHz
	
	confScMsg = confirmServiceConnectionMessageCreate();
	jausAddressCopy(confScMsg->source, nmi->cmpt->address);
	jausAddressCopy(confScMsg->destination, message->source);
	confScMsg->serviceConnectionCommandCode = sc->commandCode;
	confScMsg->instanceId = (JausByte)sc->instanceId;
	confScMsg->confirmedPeriodicUpdateRateHertz = sc->confirmedUpdateRateHz;
	confScMsg->responseCode = JAUS_SC_SUCCESSFUL;

	txMessage = confirmServiceConnectionMessageToJausMessage(confScMsg); 
	nodeManagerSend(nmi, txMessage);
	jausMessageDestroy(txMessage);	

	confirmServiceConnectionMessageDestroy(confScMsg);
	
	pthread_mutex_unlock(&nmi->scm->mutex);
}

void scManagerProcessActivateScMessage(NodeManagerInterface nmi, ActivateServiceConnectionMessage message)
{
	SupportedScMessage supportedScMsg;
	ServiceConnection sc;
	ServiceConnection messageSc;

	pthread_mutex_lock(&nmi->scm->mutex);

	supportedScMsg = scFindSupportedScMsgInList(nmi->scm->supportedScMsgList, message->serviceConnectionCommandCode);
	if(supportedScMsg == NULL)
	{
		pthread_mutex_unlock(&nmi->scm->mutex);
		return;
	}

	messageSc = (ServiceConnection)malloc( sizeof(ServiceConnectionStruct) );
	if(messageSc == NULL) 
	{
		pthread_mutex_unlock(&nmi->scm->mutex);
		return;
	}

	messageSc->address = jausAddressCreate();
	jausAddressCopy(messageSc->address, message->source);
	messageSc->instanceId = message->instanceId;

	sc = scFindScInList(supportedScMsg->scList, messageSc);
	if(sc != NULL)
	{
		sc->isActive = JAUS_TRUE;
	}
		
	jausAddressDestroy(messageSc->address);
	free(messageSc);
	pthread_mutex_unlock(&nmi->scm->mutex);
}

void scManagerProcessSuspendScMessage(NodeManagerInterface nmi, SuspendServiceConnectionMessage message)
{
	SupportedScMessage supportedScMsg;
	ServiceConnection sc;
	ServiceConnection messageSc;

	pthread_mutex_lock(&nmi->scm->mutex);
	
	supportedScMsg = scFindSupportedScMsgInList(nmi->scm->supportedScMsgList, message->serviceConnectionCommandCode);
	if(supportedScMsg == NULL)
	{
		pthread_mutex_unlock(&nmi->scm->mutex);
		return;
	}

	messageSc = (ServiceConnection)malloc( sizeof(ServiceConnectionStruct) );
	if(messageSc == NULL) 
	{
		pthread_mutex_unlock(&nmi->scm->mutex);
		return;
	}

	messageSc->address = jausAddressCreate();
	jausAddressCopy(messageSc->address, message->source);
	messageSc->instanceId = message->instanceId;

	sc = scFindScInList(supportedScMsg->scList, messageSc);
	if(sc != NULL)
	{
		sc->isActive = JAUS_FALSE;
	}
	
	jausAddressDestroy(messageSc->address);
	free(messageSc);
	pthread_mutex_unlock(&nmi->scm->mutex);
}

void scManagerProcessTerminateScMessage(NodeManagerInterface nmi, TerminateServiceConnectionMessage message)
{
	SupportedScMessage supportedScMsg;
	ServiceConnection sc;
	ServiceConnection prevSc;
	
	pthread_mutex_lock(&nmi->scm->mutex);
	
	supportedScMsg = scFindSupportedScMsgInList(nmi->scm->supportedScMsgList, message->serviceConnectionCommandCode);
	if(supportedScMsg == NULL)
	{
		pthread_mutex_unlock(&nmi->scm->mutex);
		return;
	}

	if( supportedScMsg->scList == NULL )
	{
		pthread_mutex_unlock(&nmi->scm->mutex);
		return;
	}

	sc = supportedScMsg->scList;
	
	if(	jausAddressEqual(sc->address, message->source) &&
		sc->commandCode == message->serviceConnectionCommandCode &&
		sc->instanceId == message->instanceId )
	{
		// Remove sc from list
		supportedScMsg->scList = sc->nextSc;
		serviceConnectionDestroy(sc);
		nmi->scm->outgoingScCount--;
		pthread_mutex_unlock(&nmi->scm->mutex);
		return;
	}
	
	prevSc = sc;
	sc = prevSc->nextSc;
	while(sc)
	{
		if(	jausAddressEqual(sc->address, message->source) &&
			sc->commandCode == message->serviceConnectionCommandCode &&
			sc->instanceId == message->instanceId )
		{
			// Remove sc from list
			prevSc->nextSc = sc->nextSc;
			serviceConnectionDestroy(sc);
			nmi->scm->outgoingScCount--;
			pthread_mutex_unlock(&nmi->scm->mutex);
			return;
		}	
		prevSc = sc;
		sc = sc->nextSc;
	}
	
	pthread_mutex_unlock(&nmi->scm->mutex);
}

void scManagerProcessUpdatedSubystem(NodeManagerInterface nmi, JausSubsystem subsystem)
{
	SupportedScMessage supportedScMsg;
	ServiceConnection sc;
	ServiceConnection prevSc;

	pthread_mutex_lock(&nmi->scm->mutex);
	
	supportedScMsg = nmi->scm->supportedScMsgList;
	while(supportedScMsg)
	{
		prevSc = NULL;
		sc = supportedScMsg->scList;
		while(sc)
		{
			if(	!nodeManagerVerifyAddress(nmi, sc->address) )
			{
				// Remove sc from list
				if(prevSc)
				{
					prevSc->nextSc = sc->nextSc;
					serviceConnectionDestroy(sc);
					sc = prevSc->nextSc;
				}
				else
				{
					supportedScMsg->scList = sc->nextSc;
					serviceConnectionDestroy(sc);
					sc = supportedScMsg->scList;
				}
				nmi->scm->outgoingScCount--;
			}
			else
			{
				prevSc = sc;	
				sc = sc->nextSc;
			}
		}
		supportedScMsg = supportedScMsg->nextSupportedScMsg;
	}

	prevSc = NULL;
	sc = nmi->scm->incomingSc;
	while(sc)
	{
		if( !nodeManagerVerifyAddress(nmi, sc->address) )
		{
			// Remove sc from list
			sc->isActive = JAUS_FALSE;			

			// Clear out Inbound Queue
			queueEmpty(sc->queue, (void *)jausMessageDestroy);
			
			if(prevSc)
			{
				prevSc->nextSc = sc->nextSc;
				sc->nextSc = NULL;
				sc = prevSc->nextSc;
			}
			else
			{
				nmi->scm->incomingSc = sc->nextSc;
				sc->nextSc = NULL;
				sc = nmi->scm->incomingSc;
			}
			nmi->scm->incomingScCount--;
		}
		else
		{
			prevSc = sc;
			sc = sc->nextSc;
		}
	}
	
	pthread_mutex_unlock(&nmi->scm->mutex);	
}

void scManagerAddSupportedMessage(NodeManagerInterface nmi, unsigned short commandCode)
{
	SupportedScMessage supportedScMsg;

	pthread_mutex_lock(&nmi->scm->mutex);
	
	supportedScMsg = scFindSupportedScMsgInList(nmi->scm->supportedScMsgList, commandCode);
	if(supportedScMsg == NULL)
	{																					
		supportedScMsg = (SupportedScMessage)malloc(sizeof(struct SupportedScMessageStruct));
		if(supportedScMsg == NULL) 
		{
			// TODO: Throw error
			pthread_mutex_unlock(&nmi->scm->mutex);
			return;
		}

		supportedScMsg->commandCode = commandCode; 
		supportedScMsg->scList = NULL;
		
		supportedScMsg->nextSupportedScMsg = nmi->scm->supportedScMsgList;
		nmi->scm->supportedScMsgList = supportedScMsg;
		nmi->scm->supportedScMsgCount++;
	}
	
	pthread_mutex_unlock(&nmi->scm->mutex);
}

void scManagerRemoveSupportedMessage(NodeManagerInterface nmi, unsigned short commandCode)
{
	SupportedScMessage prevSupportedScMsg = NULL;
	SupportedScMessage supportedScMsg;
	ServiceConnection sc;
	TerminateServiceConnectionMessage terminateSc;	
	JausMessage txMessage;
	
	pthread_mutex_lock(&nmi->scm->mutex);
	
	supportedScMsg = nmi->scm->supportedScMsgList;
	
	while(supportedScMsg)
	{
		if(supportedScMsg->commandCode == commandCode)
		{
			// Remove Service Connection
			if(prevSupportedScMsg)
			{
				prevSupportedScMsg->nextSupportedScMsg = supportedScMsg->nextSupportedScMsg;
			}
			else
			{
				nmi->scm->supportedScMsgList = supportedScMsg->nextSupportedScMsg;
			}
			
			// Terminate and free all the service connections
			while(supportedScMsg->scList)
			{
				sc = supportedScMsg->scList;
				supportedScMsg->scList = sc->nextSc;
				
				terminateSc = terminateServiceConnectionMessageCreate();
				jausAddressCopy(terminateSc->source, nmi->cmpt->address);
				jausAddressCopy(terminateSc->destination, sc->address);
				terminateSc->serviceConnectionCommandCode = sc->commandCode;
				terminateSc->instanceId = sc->instanceId;
	
				txMessage = terminateServiceConnectionMessageToJausMessage(terminateSc);
				nodeManagerSend(nmi, txMessage);
				jausMessageDestroy(txMessage);

				terminateServiceConnectionMessageDestroy(terminateSc);

				serviceConnectionDestroy(sc);
			}
			free(supportedScMsg);
			nmi->scm->supportedScMsgCount--;
			pthread_mutex_unlock(&nmi->scm->mutex);
			return;
		}
		prevSupportedScMsg = supportedScMsg;
		supportedScMsg = supportedScMsg->nextSupportedScMsg;
	}
	
	pthread_mutex_unlock(&nmi->scm->mutex);
}

JausBoolean scManagerQueryActiveMessage(NodeManagerInterface nmi, unsigned short commandCode)
{
	SupportedScMessage supportedScMsg;

	pthread_mutex_lock(&nmi->scm->mutex);	
	
	supportedScMsg = scFindSupportedScMsgInList(nmi->scm->supportedScMsgList, commandCode);

	pthread_mutex_unlock(&nmi->scm->mutex);	
	
	if(supportedScMsg != NULL && supportedScMsg->scList != NULL)
	{
		return JAUS_TRUE;
	}
	else
	{
		return JAUS_FALSE;
	}
}

ServiceConnection scManagerGetSendList(NodeManagerInterface nmi, unsigned short commandCode)
{
	SupportedScMessage supportedScMsg;
	ServiceConnection sc;
	ServiceConnection newSc = NULL;
	ServiceConnection firstSc = NULL;
	double currentTime = ojGetTimeSec();

	pthread_mutex_lock(&nmi->scm->mutex);	
	
	// find the SC object associated with this command code
	supportedScMsg = scFindSupportedScMsgInList(nmi->scm->supportedScMsgList, commandCode);
	if(supportedScMsg)
	{
		sc = supportedScMsg->scList;
	}
	else
	{
		pthread_mutex_unlock(&nmi->scm->mutex);
		return NULL;
	}
	
	while(sc)
	{
		// Check for update rate
		if(sc->isActive && sc->lastSentTime < (currentTime - 1.0/sc->confirmedUpdateRateHz))
		{
			sc->lastSentTime = currentTime;
			if(newSc == NULL)
			{
				newSc = (ServiceConnection)malloc(sizeof(ServiceConnectionStruct));
				firstSc = newSc;
			}
			else
			{
				newSc->nextSc = (ServiceConnection)malloc(sizeof(ServiceConnectionStruct));
				newSc = newSc->nextSc;
			}
			
			*newSc = *sc;
			newSc->nextSc = NULL;
			sc->sequenceNumber++;
		}

		sc = sc->nextSc;
	}
	
	pthread_mutex_unlock(&nmi->scm->mutex);
	return firstSc;
}

void scManagerDestroySendList(ServiceConnection sc)
{
	ServiceConnection deadSc;
	
	while(sc)
	{
		deadSc = sc;
		sc = sc->nextSc;
		free(deadSc);
	}
}

SupportedScMessage scFindSupportedScMsgInList(SupportedScMessage supportedScMsg, unsigned short commandCode)
{	
	while(supportedScMsg)
	{
		if(supportedScMsg->commandCode == commandCode)
		{
			return supportedScMsg;
		}
		supportedScMsg = supportedScMsg->nextSupportedScMsg;
	}
	
	return NULL;
}

ServiceConnection scFindScInList(ServiceConnection sc, ServiceConnection newSc)
{
	// NOTE: destination, commandCode and presenceVector define the sc equals (==) functionality
	//		 the updateRate is not included 
	
	while(sc)
	{
		if(	jausAddressEqual(sc->address, newSc->address) && sc->instanceId == newSc->instanceId )
		{
			return sc;
		}		
		sc = sc->nextSc;
	}
	
	return NULL;
}

int scGetAvailableInstanceId(ServiceConnection sc, ServiceConnection newSc)
{
	int i;
	unsigned char instanceAvailable[256];
	
	memset(instanceAvailable, 1, 256);
	
	while(sc)
	{
		if(jausAddressEqual(sc->address, newSc->address))
		{
			if(sc->presenceVector == newSc->presenceVector)
			{
				return sc->instanceId;
			}
			else
			{
				instanceAvailable[sc->instanceId] = 0;
			}
		}
		sc = sc->nextSc;
	}
	
	for(i = 0; i<256; i++)
	{
		if(instanceAvailable[i])
		{
			return i;
		}
	}

	return -1;
}

JausBoolean scManagerCreateServiceConnection(NodeManagerInterface nmi, ServiceConnection sc)
{
	CreateServiceConnectionMessage createSc;
	JausMessage txMessage;
	JausAddress localAddress;
	
	ServiceConnection prevSc = NULL;
	ServiceConnection testSc = NULL;
	
	pthread_mutex_lock(&nmi->scm->mutex);
	
	testSc = nmi->scm->incomingSc;
	
	if(!sc)
	{
		pthread_mutex_unlock(&nmi->scm->mutex);
		return JAUS_FALSE;
	}
	
	// Remove this service connection from the list of incoming service connections
	while(testSc)
	{                      
		if(sc == testSc)
		{               
			if(prevSc)
			{
				prevSc->nextSc = testSc->nextSc;
			}
			else
			{
		        nmi->scm->incomingSc = testSc->nextSc;
			}
			nmi->scm->incomingScCount--;
		}
		
		prevSc = testSc;
		testSc = testSc->nextSc;
	}
	
	sc->confirmedUpdateRateHz = 0;
	sc->lastSentTime = 0;
	sc->sequenceNumber = 65535;
	sc->instanceId = -1;
	sc->isActive = JAUS_FALSE;
	sc->nextSc = NULL;

	createSc = createServiceConnectionMessageCreate();
	jausAddressCopy(createSc->source, nmi->cmpt->address);
	createSc->serviceConnectionCommandCode = sc->commandCode;
	createSc->requestedPeriodicUpdateRateHertz = sc->requestedUpdateRateHz;
	createSc->presenceVector = sc->presenceVector;
	
	// If the subsystem for this service connection is known
	if(sc->address && sc->address->subsystem != 0)
	{
		jausAddressCopy(createSc->destination, sc->address);
			
		txMessage = createServiceConnectionMessageToJausMessage(createSc);
		nodeManagerSend(nmi, txMessage);
		jausMessageDestroy(txMessage);
		createServiceConnectionMessageDestroy(createSc);

		// Add the service connection to the front of the incoming service connection list
		sc->nextSc = nmi->scm->incomingSc;
		nmi->scm->incomingSc = sc;		
		nmi->scm->incomingScCount++;
		
		pthread_mutex_unlock(&nmi->scm->mutex);
		return JAUS_TRUE;
	}
	// otherwise the subsystem is unknown so we assume it is the same subsystem?
	else
	{		
		localAddress = jausAddressCreate();
		localAddress->subsystem = nmi->cmpt->address->subsystem;
		localAddress->node = 0;
		localAddress->component = sc->address->component;
		localAddress->instance = 0;
		
		// Lookup Address from nodeManager
		// Tests if the target component exists or not
		if(nodeManagerLookupAddress(nmi, localAddress))
		{
			jausAddressCopy(createSc->destination, localAddress);
			jausAddressCopy(sc->address, localAddress);
				
			txMessage = createServiceConnectionMessageToJausMessage(createSc);
			nodeManagerSend(nmi, txMessage);
			jausMessageDestroy(txMessage);
			
			jausAddressDestroy(localAddress);
			createServiceConnectionMessageDestroy(createSc);
	
			// Add the service connection to the front of the incoming service connection list
			sc->nextSc = nmi->scm->incomingSc;
			nmi->scm->incomingSc = sc;		
			nmi->scm->incomingScCount++;
		
			pthread_mutex_unlock(&nmi->scm->mutex);
			return JAUS_TRUE;
		}
		else
		{
			jausAddressDestroy(localAddress);
			createServiceConnectionMessageDestroy(createSc);
			
			pthread_mutex_unlock(&nmi->scm->mutex);
			return JAUS_FALSE;
		}
	}
	
	pthread_mutex_unlock(&nmi->scm->mutex);	
}

JausBoolean scManagerTerminateServiceConnection(NodeManagerInterface nmi, ServiceConnection deadSc)
{
	TerminateServiceConnectionMessage terminateSc;
	JausMessage txMessage;
	ServiceConnection prevSc = NULL;
	ServiceConnection sc = NULL;

	pthread_mutex_lock(&nmi->scm->mutex);
	
	sc = nmi->scm->incomingSc;
	
	while(sc)
	{
		if(sc->commandCode == deadSc->commandCode && jausAddressEqual(sc->address, deadSc->address) )
		{
			if(sc->instanceId > -1)
			{
				terminateSc = terminateServiceConnectionMessageCreate();
				jausAddressCopy(terminateSc->source, nmi->cmpt->address);
				jausAddressCopy(terminateSc->destination, deadSc->address);
				terminateSc->serviceConnectionCommandCode = deadSc->commandCode;
				terminateSc->instanceId = deadSc->instanceId;
	
				txMessage = terminateServiceConnectionMessageToJausMessage(terminateSc);
				nodeManagerSend(nmi, txMessage);
				jausMessageDestroy(txMessage);

				terminateServiceConnectionMessageDestroy(terminateSc);
			}			

			// Set SC to inactive
			sc->isActive = JAUS_FALSE;
			
			// Empty any Remaining Queue
			queueEmpty(sc->queue, (void *)jausMessageDestroy);

			// Remove Service Connection
			if(prevSc)
			{
				prevSc->nextSc = sc->nextSc;
				sc->nextSc = NULL;
			}
			else
			{
				nmi->scm->incomingSc = sc->nextSc;
				sc->nextSc = NULL;
			}
			
			nmi->scm->incomingScCount--;
			pthread_mutex_unlock(&nmi->scm->mutex);
			return JAUS_TRUE;
		}
		else
		{
			prevSc = sc;
			sc = sc->nextSc;
		}
	}
	pthread_mutex_unlock(&nmi->scm->mutex);
	return JAUS_FALSE;
}

JausBoolean scManagerReceiveServiceConnection(NodeManagerInterface nmi, ServiceConnection requestSc, JausMessage *message)
{
	ServiceConnection prevSc;
	ServiceConnection sc;

	pthread_mutex_unlock(&nmi->scm->mutex);	
	
	sc = nmi->scm->incomingSc;
	prevSc = NULL;
	while(sc)
	{
		if(sc->commandCode == requestSc->commandCode && jausAddressEqual(sc->address, requestSc->address) )
		{
			if(ojGetTimeSec() > (sc->lastSentTime + sc->timeoutSec))
			{
				// Connection has Timed Out
				sc->isActive = JAUS_FALSE;
				queueEmpty(sc->queue, (void *)jausMessageDestroy);				

				// Remove Service Connection
				if(prevSc)
				{
					prevSc->nextSc = sc->nextSc;
					sc->nextSc = NULL;
				}
				else
				{
					nmi->scm->incomingSc = sc->nextSc;
					sc->nextSc = NULL;
				}
				nmi->scm->incomingScCount--;
				pthread_mutex_unlock(&nmi->scm->mutex);
				return JAUS_FALSE;
			}

			if(sc->queue->size)
			{
				*message = (JausMessage)queuePop(sc->queue);
				pthread_mutex_unlock(&nmi->scm->mutex);
				return JAUS_TRUE;
			}
			else
			{
				pthread_mutex_unlock(&nmi->scm->mutex);
				return JAUS_FALSE;
			}
		}
		else
		{
			prevSc = sc;
			sc = sc->nextSc;
		}
	}

	pthread_mutex_unlock(&nmi->scm->mutex);
	return JAUS_FALSE;
}

void scManagerReceiveMessage(NodeManagerInterface nmi, JausMessage message)
{
	ServiceConnection sc;
	char string[32] = {0};
	
	pthread_mutex_lock(&nmi->scm->mutex);
	
	sc = nmi->scm->incomingSc;
	while(sc)
	{
		if(sc->commandCode == message->commandCode && jausAddressEqual(sc->address, message->source) )
		{
			if(sc->isActive)
			{
				sc->lastSentTime = ojGetTimeSec();
				
				if(sc->queueSize && sc->queueSize == sc->queue->size)
				{
					jausMessageDestroy(queuePop(sc->queue));
					queuePush(sc->queue, (void *)message);
				}
				else
				{
					queuePush(sc->queue, (void *)message);
				} 
			}			
			else
			{
				// TODO: Error? received a message for inactive SC
				jausMessageDestroy(message);
			}
			pthread_mutex_unlock(&nmi->scm->mutex);
			return;		
		}
		sc = sc->nextSc;
	}

	jausAddressToString(message->source, string);
	jausMessageDestroy(message);
	pthread_mutex_unlock(&nmi->scm->mutex);
}

int scManagerUpdateServiceConnection(ServiceConnection sc, unsigned short sequenceNumber)
{
	int returnValue = JAUS_FALSE;

	sc->lastSentTime = ojGetTimeSec();

	if(sequenceNumber == sc->sequenceNumber)
	{
		returnValue =  SC_ERROR_SEQUENCE_NUMBERS_EQUAL;
	}
	else if(sequenceNumber == 0 && sc->sequenceNumber != 65535)
	{
		returnValue = SC_ERROR_SEQUENCE_NUMBER_OUT_OF_SYNC;
	}
	else if(sequenceNumber - sc->sequenceNumber != 1 && sequenceNumber !=0)
	{
		returnValue = SC_ERROR_SEQUENCE_NUMBER_OUT_OF_SYNC;
	}
	else
	{
		returnValue = JAUS_TRUE;
	}
	
	sc->sequenceNumber = sequenceNumber;
	
	return returnValue;
}
