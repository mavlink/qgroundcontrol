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
// File:		vss.c 
// Version:		0.4 alpha
// Written by:	Tom Galluzzo (galluzzt@ufl.edu) and Danny Kent (kentd@ufl.edu)
// Date:		07/01/2005
// Description:	This file contains the skeleton C code for implementing a JAUS component in a Linux environment
//				This code is designed to work with the node manager and JAUS library software written by CIMAR

#include <jaus.h>			// JAUS message set (USER: JAUS libraries must be installed first)
#include <openJaus.h>	// Node managment functions for sending and receiving JAUS messages (USER: Node Manager must be installed)
#include <stdlib.h>	
#include "vss.h"	// USER: Implement and rename this header file. Include prototypes for all public functions contained in this file.
#include "vehicleSim.h"	

// Private function prototypes
void vssReadyState(OjCmpt vss);
void vssQueryVelocityStateCallback(OjCmpt vss, JausMessage query);


OjCmpt vssCreate(void)
{
	OjCmpt cmpt;
	ReportVelocityStateMessage message;
	JausAddress vssAddr;
	
	cmpt = ojCmptCreate("vss", JAUS_VELOCITY_STATE_SENSOR, VSS_THREAD_DESIRED_RATE_HZ);

	ojCmptAddService(cmpt, JAUS_VELOCITY_STATE_SENSOR);
	ojCmptAddServiceInputMessage(cmpt, JAUS_VELOCITY_STATE_SENSOR, JAUS_QUERY_VELOCITY_STATE, 0xFF);
	ojCmptAddServiceOutputMessage(cmpt, JAUS_VELOCITY_STATE_SENSOR, JAUS_REPORT_VELOCITY_STATE, 0xFF);

	ojCmptSetStateCallback(cmpt, JAUS_READY_STATE, vssReadyState);
	ojCmptSetMessageCallback(cmpt, JAUS_QUERY_VELOCITY_STATE, vssQueryVelocityStateCallback);
	ojCmptAddSupportedSc(cmpt, JAUS_REPORT_VELOCITY_STATE);
	
	message = reportVelocityStateMessageCreate();
	vssAddr = ojCmptGetAddress(cmpt);
	jausAddressCopy(message->source, vssAddr);
	jausAddressDestroy(vssAddr);
	
	ojCmptSetUserData(cmpt, (void *)message);
	
	ojCmptSetState(cmpt, JAUS_READY_STATE);

	if(ojCmptRun(cmpt))
	{
		ojCmptDestroy(cmpt);
		return NULL;
	}

	return cmpt;
}

void vssDestroy(OjCmpt vss)
{
	ReportVelocityStateMessage message;
	
	message = (ReportVelocityStateMessage)ojCmptGetUserData(vss);

	ojCmptRemoveSupportedSc(vss, JAUS_REPORT_VELOCITY_STATE);	
	ojCmptDestroy(vss);

	reportVelocityStateMessageDestroy(message);	
}

// The series of functions below allow public access to essential component information
// Access:		Public (All)
double vssGetVelocityX(OjCmpt vss)
{
	ReportVelocityStateMessage message;

	message = (ReportVelocityStateMessage)ojCmptGetUserData(vss);
	
	return message->velocityXMps;
}

int vssGetScActive(OjCmpt vss)
{
	return ojCmptIsOutgoingScActive(vss, JAUS_REPORT_VELOCITY_STATE);
}

void vssQueryVelocityStateCallback(OjCmpt vss, JausMessage query)
{
	ReportVelocityStateMessage message;
	JausMessage txMessage;
	QueryVelocityStateMessage queryVelocityState;
	
	message = (ReportVelocityStateMessage)ojCmptGetUserData(vss);

	queryVelocityState = queryVelocityStateMessageFromJausMessage(query);
	if(queryVelocityState)
	{
		jausAddressCopy(message->destination, queryVelocityState->source);
		message->presenceVector = queryVelocityState->presenceVector;
		message->sequenceNumber = 0;
		message->properties.scFlag = 0;
		
		txMessage = reportVelocityStateMessageToJausMessage(message);
		ojCmptSendMessage(vss, txMessage);		
		jausMessageDestroy(txMessage);
		
		queryVelocityStateMessageDestroy(queryVelocityState);
	}
	else
	{
		////cError("vss: Error unpacking %s message.\n", jausMessageCommandCodeString(message));
	}
}


void vssReadyState(OjCmpt vss)
{
	JausMessage txMessage;
	ServiceConnection scList;
	ServiceConnection sc;

	ReportVelocityStateMessage message;

	message = (ReportVelocityStateMessage)ojCmptGetUserData(vss);

	message->velocityXMps = vehicleSimGetSpeed();
	message->velocityYMps = 0;
	message->velocityZMps = 0;
	message->velocityRmsMps = 0.01;
	message->rollRateRps = 0.0;
	message->pitchRateRps = 0.0;
	message->yawRateRps = 0.0;
	message->rateRmsRps = 0;

	// send message
	if(ojCmptIsOutgoingScActive(vss, JAUS_REPORT_VELOCITY_STATE))
	{
		scList = ojCmptGetScSendList(vss, JAUS_REPORT_VELOCITY_STATE);
		sc = scList;
		while(sc)
		{
			jausAddressCopy(message->destination, sc->address);
			message->presenceVector = sc->presenceVector;
			message->sequenceNumber = sc->sequenceNumber;
			message->properties.scFlag = JAUS_SERVICE_CONNECTION_MESSAGE;
			
			txMessage = reportVelocityStateMessageToJausMessage(message);
			ojCmptSendMessage(vss, txMessage);		
			jausMessageDestroy(txMessage);
	
			sc = sc->nextSc;
		}
		
		ojCmptDestroySendList(scList);
	}
}
