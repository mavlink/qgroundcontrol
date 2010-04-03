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
// File Name: ojCmpt.h
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


#ifndef OJ_CMPT_H
#define OJ_CMPT_H

#include <jaus.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#define OJ_CMPT_MAX_STATE_COUNT			32
#define OJ_CMPT_MAX_INCOMING_SC_COUNT	32
#define OJ_CMPT_MIN_FREQUENCY_HZ		0.1
#define OJ_CMPT_MAX_FREQUENCY_HZ		1000.0
#define OJ_CMPT_DEFAULT_FREQUENCY_HZ	1.0
	
typedef struct OjCmptStruct *OjCmpt;

JAUS_EXPORT OjCmpt ojCmptCreate(char *name, JausByte id, double frequency);
JAUS_EXPORT void ojCmptDestroy(OjCmpt ojCmpt);
JAUS_EXPORT int ojCmptRun(OjCmpt ojCmpt);

JAUS_EXPORT void ojCmptSetFrequencyHz(OjCmpt ojCmpt, double stateFrequencyHz);
JAUS_EXPORT void ojCmptSetState(OjCmpt ojCmpt, int state);

JAUS_EXPORT int ojCmptSetStateCallback(OjCmpt ojCmpt, int state, void (*stateCallbackFunction)(OjCmpt));			// Calls method from stateHandler
JAUS_EXPORT int ojCmptSetMainCallback(OjCmpt ojCmpt, void (*mainCallbackFunction)(OjCmpt));							// Calls method from stateHandler
JAUS_EXPORT void ojCmptSetMessageCallback(OjCmpt ojCmpt, unsigned short commandCode, void (*messageFunction)(OjCmpt, JausMessage));	// Calls method from messageHandler
JAUS_EXPORT void ojCmptSetMessageProcessorCallback(OjCmpt ojCmpt, void (*processMessageFunction)(OjCmpt, JausMessage));	// Calls method from messageHandler
JAUS_EXPORT void ojCmptSetUserData(OjCmpt ojCmpt, void *data);
JAUS_EXPORT void ojCmptSetAuthority(OjCmpt ojCmpt, JausByte authority);

JAUS_EXPORT int ojCmptSendMessage(OjCmpt ojCmpt, JausMessage message);

// Accessors
JAUS_EXPORT JausByte ojCmptGetAuthority(OjCmpt ojCmpt);
JAUS_EXPORT JausAddress ojCmptGetAddress(OjCmpt ojCmpt);
JAUS_EXPORT JausAddress ojCmptGetControllerAddress(OjCmpt ojCmpt);
JAUS_EXPORT JausBoolean ojCmptHasController(OjCmpt ojCmpt);
JAUS_EXPORT int ojCmptGetState(OjCmpt ojCmpt);
JAUS_EXPORT void ojCmptDefaultMessageProcessor(OjCmpt ojCmpt, JausMessage message);
JAUS_EXPORT char* ojCmptGetName(OjCmpt ojCmpt);
JAUS_EXPORT void *ojCmptGetUserData(OjCmpt ojCmpt);
JAUS_EXPORT double ojCmptGetRateHz(OjCmpt ojCmpt);

// Services
JAUS_EXPORT JausBoolean ojCmptAddService(OjCmpt ojCmpt, JausUnsignedShort serviceType);
JAUS_EXPORT JausBoolean ojCmptAddServiceInputMessage(OjCmpt ojCmpt, JausUnsignedShort serviceType, JausUnsignedShort commandCode, JausUnsignedInteger presenceVector);
JAUS_EXPORT JausBoolean ojCmptAddServiceOutputMessage(OjCmpt ojCmpt, JausUnsignedShort serviceType, JausUnsignedShort commandCode, JausUnsignedInteger presenceVector);

// Incoming Service Connections
JAUS_EXPORT int ojCmptEstablishSc(OjCmpt ojCmpt, JausUnsignedShort cCode, JausUnsignedInteger pv, JausAddress address, double rateHz, double timeoutSec, int qSize);
JAUS_EXPORT int ojCmptTerminateSc(OjCmpt ojCmpt, int scIndex);
JAUS_EXPORT JausBoolean ojCmptIsIncomingScActive(OjCmpt ojCmpt, int scIndex);
// OjCmpt add receive SC method

// Outgoing Service Connections
JAUS_EXPORT void ojCmptAddSupportedSc(OjCmpt ojCmpt, unsigned short commandCode);		// Add service connection support for this message
JAUS_EXPORT void ojCmptRemoveSupportedSc(OjCmpt ojCmpt, unsigned short commandCode);	// Removes service connection support for this message
JAUS_EXPORT ServiceConnection ojCmptGetScSendList(OjCmpt ojCmpt, unsigned short commandCode);
JAUS_EXPORT void ojCmptDestroySendList(ServiceConnection scList);
JAUS_EXPORT JausBoolean ojCmptIsOutgoingScActive(OjCmpt ojCmpt, unsigned short commandCode);
	
// System Discovery
JAUS_EXPORT int ojCmptLookupAddress(OjCmpt ojCmpt, JausAddress address);


#ifdef __cplusplus
}
#endif

#endif // OJ_CMPT_H

