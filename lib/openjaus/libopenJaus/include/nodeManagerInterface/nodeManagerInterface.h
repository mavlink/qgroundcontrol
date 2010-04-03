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
#ifndef NODE_MANAGER_H
#define NODE_MANAGER_H

#include "utils/datagramSocket.h"
#include "utils/inetAddress.h"
#include "utils/queue.h"
#include "utils/timeLib.h"
#include <jaus.h>
#include <pthread.h>
#include <string.h>

#ifdef WIN32
	#define JAUS_EXPORT	__declspec(dllexport)
#else
	#define JAUS_EXPORT
#endif

#define	OJ_NODE_MANAGER_INTERFACE_VERSION	"3.3.0"

#define	NMI_MESSAGE_RECEIVED		1
#define	NMI_RECEIVE_TIMED_OUT		2
#define NMI_CONDITIONAL_WAIT_ERROR	3
#define NMI_CLOSED_ERROR			4

#ifdef __cplusplus
extern "C" 
{
#endif

#define SC_ERROR_SEQUENCE_NUMBERS_EQUAL 			-1
#define SC_ERROR_SEQUENCE_NUMBER_OUT_OF_SYNC 		-2
#define SC_ERROR_SERVICE_CONNECTION_QUEUE_EMPTY		-1
#define SC_ERROR_SERVICE_CONNECTION_DOES_NOT_EXIST	-2

typedef struct ServiceConnectionStruct
{
	double requestedUpdateRateHz;
	double confirmedUpdateRateHz;
	double lastSentTime;
	double timeoutSec;
	double nextRequestTimeSec;

	JausAddress address;
	JausUnsignedInteger presenceVector;
	JausUnsignedShort commandCode;
	JausUnsignedShort sequenceNumber;

	int instanceId;
	JausBoolean isActive;
	
	Queue queue;
	unsigned int queueSize;
	
	struct ServiceConnectionStruct *nextSc;
}ServiceConnectionStruct;

typedef ServiceConnectionStruct *ServiceConnection;

typedef struct SupportedScMessageStruct
{
	unsigned short commandCode;
	ServiceConnection scList;
	struct SupportedScMessageStruct *nextSupportedScMsg;
}SupportedScMessageStruct;

typedef SupportedScMessageStruct *SupportedScMessage;

typedef struct
{
	SupportedScMessage supportedScMsgList;
	ServiceConnection incomingSc;
	int supportedScMsgCount;
	int outgoingScCount;
	int incomingScCount;
	pthread_mutex_t mutex;
}ServiceConnectionManagerStruct;

typedef ServiceConnectionManagerStruct *ServiceConnectionManager;

typedef struct JausAddressListStruct
{
	JausAddress address;
	struct JausAddressListStruct *nextAddress;
}JausAddressList;

typedef struct
{
	JausArray messageLists;
}LargeMessageHandlerStruct;

typedef LargeMessageHandlerStruct *LargeMessageHandler;

typedef struct
{
	JausUnsignedShort commandCode;
	JausAddress source;
	JausArray messages;
}LargeMessageListStruct;

typedef LargeMessageListStruct *LargeMessageList;

typedef struct
{
	DatagramSocket interfaceSocket;
	DatagramSocket messageSocket;
	
	pthread_t heartbeatThreadId;
	int heartbeatThreadRunning;
	pthread_t receiveThreadId;
	int receiveThreadRunning;
	
	pthread_cond_t recvCondition;
	pthread_cond_t hbWakeCondition;
	       
	InetAddress ipAddress;
	
	Queue receiveQueue;
	
	JausComponent cmpt;
	
	int isOpen;

	int heartbeatCount;
	int receiveCount;
	
	double timestamp;
	
	ServiceConnectionManager scm;
	LargeMessageHandler lmh;
}NodeManagerInterfaceStruct;

typedef NodeManagerInterfaceStruct *NodeManagerInterface;

JAUS_EXPORT NodeManagerInterface nodeManagerOpen(JausComponent);
JAUS_EXPORT int nodeManagerClose(NodeManagerInterface);
JAUS_EXPORT int nodeManagerReceive(NodeManagerInterface, JausMessage *);
JAUS_EXPORT int nodeManagerTimedReceive(NodeManagerInterface nmi, JausMessage *message, double timeLimitSec);
JAUS_EXPORT int nodeManagerSend(NodeManagerInterface, JausMessage);
JAUS_EXPORT int nodeManagerSendSingleMessage(NodeManagerInterface, JausMessage);
JAUS_EXPORT JausAddressList *nodeManagerGetComponentAddressList(NodeManagerInterface, unsigned char);
JAUS_EXPORT void nodeManagerDestroyAddressList(JausAddressList *);
JAUS_EXPORT int nodeManagerVerifyAddress(NodeManagerInterface, JausAddress);
JAUS_EXPORT JausBoolean nodeManagerLookupAddress(NodeManagerInterface, JausAddress);
JAUS_EXPORT void nodeManagerSendCoreServiceConnections(NodeManagerInterface);
JAUS_EXPORT JausBoolean nodeManagerLookupServiceAddress(NodeManagerInterface, JausAddress, unsigned short, int);
JAUS_EXPORT JausAddressList* nodeManagerLookupServiceAddressList(NodeManagerInterface, JausAddress, unsigned short, int);

JAUS_EXPORT ServiceConnection serviceConnectionCreate(void);
JAUS_EXPORT void serviceConnectionDestroy(ServiceConnection sc);

JAUS_EXPORT ServiceConnectionManager scManagerCreate(void);
JAUS_EXPORT void scManagerDestroy(ServiceConnectionManager);

JAUS_EXPORT void scManagerProcessConfirmScMessage(NodeManagerInterface, ConfirmServiceConnectionMessage);
JAUS_EXPORT void scManagerProcessCreateScMessage(NodeManagerInterface, CreateServiceConnectionMessage);
JAUS_EXPORT void scManagerProcessActivateScMessage(NodeManagerInterface, ActivateServiceConnectionMessage);
JAUS_EXPORT void scManagerProcessSuspendScMessage(NodeManagerInterface, SuspendServiceConnectionMessage);
JAUS_EXPORT void scManagerProcessTerminateScMessage(NodeManagerInterface, TerminateServiceConnectionMessage);
JAUS_EXPORT void scManagerProcessUpdatedSubystem(NodeManagerInterface, JausSubsystem);

JAUS_EXPORT void scManagerAddSupportedMessage(NodeManagerInterface, unsigned short);
JAUS_EXPORT void scManagerRemoveSupportedMessage(NodeManagerInterface, unsigned short);
JAUS_EXPORT JausBoolean scManagerQueryActiveMessage(NodeManagerInterface, unsigned short);

JAUS_EXPORT ServiceConnection scManagerGetSendList(NodeManagerInterface, unsigned short);
JAUS_EXPORT void scManagerDestroySendList(ServiceConnection);

JAUS_EXPORT JausBoolean scManagerCreateServiceConnection(NodeManagerInterface nmi, ServiceConnection sc);
JAUS_EXPORT JausBoolean scManagerTerminateServiceConnection(NodeManagerInterface, ServiceConnection);
JAUS_EXPORT JausBoolean scManagerReceiveServiceConnection(NodeManagerInterface nmi, ServiceConnection requestSc, JausMessage *message);
JAUS_EXPORT void scManagerReceiveMessage(NodeManagerInterface, JausMessage);

JAUS_EXPORT void defaultJausMessageProcessor(JausMessage, NodeManagerInterface, JausComponent);
JAUS_EXPORT void defaultJausMessageProcessorNoDestroy(JausMessage message, NodeManagerInterface nmi, JausComponent cmpt);

JAUS_EXPORT LargeMessageHandler lmHandlerCreate(void);
JAUS_EXPORT void lmHandlerDestroy(LargeMessageHandler);
JAUS_EXPORT LargeMessageList lmListCreate(void);
JAUS_EXPORT void lmListDestroy(LargeMessageList);
JAUS_EXPORT void lmHandlerReceiveLargeMessage(NodeManagerInterface, JausMessage);
JAUS_EXPORT int lmHandlerMessageListEqual(LargeMessageList, LargeMessageList);
JAUS_EXPORT LargeMessageList lmHandlerGetMessageList(LargeMessageHandler, JausMessage);
JAUS_EXPORT int lmHandlerLargeMessageCheck(JausMessage, JausMessage);
JAUS_EXPORT int lmHandlerSendLargeMessage(NodeManagerInterface, JausMessage);

#ifdef __cplusplus
}
#endif

#endif // NODE_MANAGER_INTERFACE_H
