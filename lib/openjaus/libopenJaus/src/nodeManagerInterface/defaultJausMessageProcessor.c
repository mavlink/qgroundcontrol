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
// File:		defaultJausMessageProcessor.c
// Version:		3.3 BETA
// Written by:	Tom Galluzzo (galluzzo AT gmail DOT com) and Danny Kent (jaus AT dannykent DOT com)
// Date:		04/15/08
// Description:	Processes a given JAUS message with the expected JAUS defined behavior

#include <stdio.h>
#include <string.h>

#include "nodeManagerInterface/nodeManagerInterface.h"

void defaultJausMessageProcessor(JausMessage message, NodeManagerInterface nmi, JausComponent cmpt)
{
	defaultJausMessageProcessorNoDestroy(message, nmi, cmpt);
	jausMessageDestroy(message);
}

void defaultJausMessageProcessorNoDestroy(JausMessage message, NodeManagerInterface nmi, JausComponent cmpt)
{
	JausMessage txMessage;
	SetComponentAuthorityMessage setComponentAuthority;
	RequestComponentControlMessage requestComponentControl;
	ReleaseComponentControlMessage releaseComponentControl;
	RejectComponentControlMessage rejectComponentControl;
	ConfirmComponentControlMessage confirmComponentControl;
	ReportComponentAuthorityMessage reportComponentAuthority;
	ReportComponentStatusMessage reportComponentStatus;
	ReportIdentificationMessage reportIdentification;
	ReportConfigurationMessage reportConfMsg;
	ConfirmServiceConnectionMessage confScMsg;
	CreateServiceConnectionMessage createScMsg;
	ActivateServiceConnectionMessage activateServiceConnection;
	SuspendServiceConnectionMessage suspendServiceConnection;
	TerminateServiceConnectionMessage terminateServiceConnection;
	QueryServicesMessage queryServices;
	ReportServicesMessage reportServices;
	
	char string[128] = {0};
	
	if(message == NULL)
	{
		return;
	}
	
	if(nmi == NULL)
	{
		return;
	}
	
	if(nmi->scm == NULL) 
	{
		return;
	}
	
	if(cmpt->controller.active && !jausAddressEqual(message->source, cmpt->controller.address) && jausMessageIsRejectableCommand(message) )
	{
		jausAddressToString(message->source, string);
		////cError("DefaultMessageProcessor: Command %s, from non-controlling source: %s\n", jausMessageCommandCodeString(message));
		return;		
	}	

	switch(message->commandCode) // Switch the processing algorithm according to the JAUS message type
	{
		// Set the component authority according to the incoming authority code
		case JAUS_SET_COMPONENT_AUTHORITY: 
			setComponentAuthority = setComponentAuthorityMessageFromJausMessage(message);
			if(setComponentAuthority)
			{
				cmpt->authority = setComponentAuthority->authorityCode;				
			}
			else
			{
				//TODO: Throw errors (need a way to capture errors in this library)
				////cError("DefaultMessageProcessor: Error unpacking %s message.\n", jausMessageCommandCodeString(message));
			}
			break;
		
		case JAUS_SHUTDOWN:
			cmpt->state = JAUS_SHUTDOWN_STATE; 
			break;
			
		case JAUS_STANDBY:
			if(cmpt->state == JAUS_READY_STATE)
			{
				cmpt->state = JAUS_STANDBY_STATE;
			}
			break;
			
		case JAUS_RESUME:
			if(cmpt->state == JAUS_STANDBY_STATE)
			{
				cmpt->state = JAUS_READY_STATE;
			}
			break;
			
		case JAUS_RESET:
			cmpt->state = JAUS_INITIALIZE_STATE;
			break;
			
		case JAUS_SET_EMERGENCY:
			cmpt->state = JAUS_EMERGENCY_STATE; 
			break;
			
		case JAUS_CLEAR_EMERGENCY:
			cmpt->state = JAUS_STANDBY_STATE;
			break;
			
		case JAUS_REQUEST_COMPONENT_CONTROL:
			requestComponentControl = requestComponentControlMessageFromJausMessage(message);
			if(requestComponentControl)
			{
				if(cmpt->controller.active)
				{
					if(requestComponentControl->authorityCode > cmpt->controller.authority) // Test for higher authority
					{	
						// Terminate control of current component
						rejectComponentControl = rejectComponentControlMessageCreate();
						jausAddressCopy(rejectComponentControl->source, cmpt->address);
						jausAddressCopy(rejectComponentControl->destination, cmpt->controller.address);
						txMessage = rejectComponentControlMessageToJausMessage(rejectComponentControl); 
						nodeManagerSend(nmi, txMessage);
						jausMessageDestroy(txMessage);
						
						// Accept control of new component
						confirmComponentControl = confirmComponentControlMessageCreate();
						jausAddressCopy(confirmComponentControl->source, cmpt->address);
						jausAddressCopy(confirmComponentControl->destination, message->source);
						confirmComponentControl->responseCode = JAUS_CONTROL_ACCEPTED;
						txMessage = confirmComponentControlMessageToJausMessage(confirmComponentControl); 
						nodeManagerSend(nmi, txMessage);
						jausMessageDestroy(txMessage);
						
						// Update cmpt controller information
						jausAddressCopy(cmpt->controller.address, message->source);
						cmpt->controller.authority = requestComponentControl->authorityCode;
					
						rejectComponentControlMessageDestroy(rejectComponentControl);
						confirmComponentControlMessageDestroy(confirmComponentControl);						
					}	
					else
					{
						if(	!jausAddressEqual(message->source, cmpt->controller.address))
						{
							rejectComponentControl = rejectComponentControlMessageCreate();
							jausAddressCopy(rejectComponentControl->source, cmpt->address);
							jausAddressCopy(rejectComponentControl->destination, message->source);
							txMessage = rejectComponentControlMessageToJausMessage(rejectComponentControl); 
							nodeManagerSend(nmi, txMessage);
							jausMessageDestroy(txMessage);

							rejectComponentControlMessageDestroy(rejectComponentControl);
						}
						else
						{
							// Reaccept control of new component
							confirmComponentControl = confirmComponentControlMessageCreate();
							jausAddressCopy(confirmComponentControl->source, cmpt->address);
							jausAddressCopy(confirmComponentControl->destination, message->source);
							confirmComponentControl->responseCode = JAUS_CONTROL_ACCEPTED;
							txMessage = confirmComponentControlMessageToJausMessage(confirmComponentControl); 
							nodeManagerSend(nmi, txMessage);
							jausMessageDestroy(txMessage);
							
							confirmComponentControlMessageDestroy(confirmComponentControl);						
						}
					}
				}					
				else // Not currently under component control, so give control
				{
					confirmComponentControl = confirmComponentControlMessageCreate();
					jausAddressCopy(confirmComponentControl->source, cmpt->address);
					jausAddressCopy(confirmComponentControl->destination, message->source);
					confirmComponentControl->responseCode = JAUS_CONTROL_ACCEPTED;
					txMessage = confirmComponentControlMessageToJausMessage(confirmComponentControl); 
					nodeManagerSend(nmi, txMessage);
					jausMessageDestroy(txMessage);
	
					jausAddressCopy(cmpt->controller.address, message->source);
					cmpt->controller.authority = requestComponentControl->authorityCode;
					cmpt->controller.active = JAUS_TRUE;

					confirmComponentControlMessageDestroy(confirmComponentControl);						
				}

				requestComponentControlMessageDestroy(requestComponentControl);
			}
			else
			{
				//cError("DefaultMessageProcessor: Error unpacking %s message.\n", jausMessageCommandCodeString(message));
			}
			break;
			
		case JAUS_RELEASE_COMPONENT_CONTROL:
			releaseComponentControl = releaseComponentControlMessageFromJausMessage(message);
			if(releaseComponentControl)
			{
				cmpt->controller.active = JAUS_FALSE;
				cmpt->controller.address->subsystem = 0;
				cmpt->controller.address->node = 0;
				cmpt->controller.address->component = 0;
				cmpt->controller.address->instance = 0;
				cmpt->controller.authority = 0;
				cmpt->controller.state = JAUS_UNDEFINED_STATE;
				releaseComponentControlMessageDestroy(releaseComponentControl);
			}
			else
			{
				//cError("DefaultMessageProcessor: Error unpacking %s message.\n", jausMessageCommandCodeString(message));
			}
			break;

		case JAUS_CONFIRM_SERVICE_CONNECTION:
			confScMsg = confirmServiceConnectionMessageFromJausMessage(message);
			if(confScMsg)
			{
				scManagerProcessConfirmScMessage(nmi, confScMsg);
				confirmServiceConnectionMessageDestroy(confScMsg);
			}
			else
			{
				//cError("DefaultMessageProcessor: Error unpacking %s message.\n", jausMessageCommandCodeString(message));
			}
			break;

		case JAUS_CREATE_SERVICE_CONNECTION:
			createScMsg = createServiceConnectionMessageFromJausMessage(message);
			if(createScMsg)
			{
				scManagerProcessCreateScMessage(nmi, createScMsg);
				createServiceConnectionMessageDestroy(createScMsg);
			}
			else
			{
				//cError("DefaultMessageProcessor: Error unpacking %s message.\n", jausMessageCommandCodeString(message));
			}
			
			break;
			
		case JAUS_ACTIVATE_SERVICE_CONNECTION:
			activateServiceConnection = activateServiceConnectionMessageFromJausMessage(message);
			if(activateServiceConnection)
			{
				scManagerProcessActivateScMessage(nmi, activateServiceConnection);
				activateServiceConnectionMessageDestroy(activateServiceConnection);
			}
			else
			{
				//cError("DefaultMessageProcessor: Error unpacking %s message.\n", jausMessageCommandCodeString(message));
			}
			
			break;
			
		case JAUS_SUSPEND_SERVICE_CONNECTION:
			suspendServiceConnection = suspendServiceConnectionMessageFromJausMessage(message);
			if(suspendServiceConnection)
			{
				scManagerProcessSuspendScMessage(nmi, suspendServiceConnection);
				suspendServiceConnectionMessageDestroy(suspendServiceConnection);
			}
			else
			{
				//cError("DefaultMessageProcessor: Error unpacking %s message.\n", jausMessageCommandCodeString(message));
			}
			break;

		case JAUS_TERMINATE_SERVICE_CONNECTION:
			terminateServiceConnection = terminateServiceConnectionMessageFromJausMessage(message);
			if(terminateServiceConnection)
			{
				scManagerProcessTerminateScMessage(nmi, terminateServiceConnection);
				terminateServiceConnectionMessageDestroy(terminateServiceConnection);
			}
			else
			{
				//cError("DefaultMessageProcessor: Error unpacking %s message.\n", jausMessageCommandCodeString(message));
			}
			break;
					
		case JAUS_QUERY_COMPONENT_AUTHORITY:
			reportComponentAuthority = reportComponentAuthorityMessageCreate();
			jausAddressCopy(reportComponentAuthority->source, cmpt->address);
			jausAddressCopy(reportComponentAuthority->destination, message->source);
			reportComponentAuthority->authorityCode = cmpt->authority;
			txMessage = reportComponentAuthorityMessageToJausMessage(reportComponentAuthority);	
			nodeManagerSend(nmi, txMessage);
			jausMessageDestroy(txMessage);
			
			reportComponentAuthorityMessageDestroy(reportComponentAuthority);
			break;
			
		case JAUS_QUERY_COMPONENT_STATUS:
			reportComponentStatus = reportComponentStatusMessageCreate();
			jausAddressCopy(reportComponentStatus->source, cmpt->address);
			jausAddressCopy(reportComponentStatus->destination, message->source);
			reportComponentStatus->primaryStatusCode = cmpt->state;
			
			txMessage = reportComponentStatusMessageToJausMessage(reportComponentStatus);	
			nodeManagerSend(nmi, txMessage);
			jausMessageDestroy(txMessage);

			reportComponentStatusMessageDestroy(reportComponentStatus);
			break;

		case JAUS_REPORT_HEARTBEAT_PULSE:
			if(	message->source->subsystem == cmpt->address->subsystem &&
				message->source->node == cmpt->address->node &&
				message->source->component == JAUS_NODE_MANAGER)
			{
				nmi->timestamp = ojGetTimeSec();
			}
			break;
					
		case JAUS_QUERY_IDENTIFICATION:
			reportIdentification = reportIdentificationMessageCreate();
			jausAddressCopy(reportIdentification->source, cmpt->address);
			jausAddressCopy(reportIdentification->destination, message->source);
			sprintf(reportIdentification->identification, "%s", cmpt->identification);
			reportIdentification->queryType = JAUS_QUERY_FIELD_COMPONENT_IDENTITY; // A component can only respond with its identification
			txMessage = reportIdentificationMessageToJausMessage(reportIdentification);	
			nodeManagerSend(nmi, txMessage);
			jausMessageDestroy(txMessage);

			reportIdentificationMessageDestroy(reportIdentification);
			break;
			
		case JAUS_REPORT_CONFIGURATION:
			reportConfMsg = reportConfigurationMessageFromJausMessage(message);
			if(reportConfMsg)
			{
				jausSubsystemDestroy(cmpt->node->subsystem);
				cmpt->node->subsystem = jausSubsystemClone(reportConfMsg->subsystem);
				reportConfigurationMessageDestroy(reportConfMsg);
				scManagerProcessUpdatedSubystem(nmi, cmpt->node->subsystem);
				
				// Check for Controlleer
				if(	cmpt->controller.active == JAUS_TRUE && !nodeManagerVerifyAddress(nmi, cmpt->controller.address) )
				{
					// TODO: Throw errors (need a way to capture errors in this library)
					//cError("Active Component Controller (%d.%d.%d.%d) lost.\n", 
					//		cmpt->controller.address->subsystem, cmpt->controller.address->node, 
					//		cmpt->controller.address->component, cmpt->controller.address->instance);

					// Disable Controller
					cmpt->controller.active = JAUS_FALSE;
					cmpt->controller.address->subsystem = 0;
					cmpt->controller.address->node = 0;
					cmpt->controller.address->component = 0;
					cmpt->controller.address->instance = 0;
					cmpt->controller.state = JAUS_UNDEFINED_STATE;
					cmpt->controller.authority = 0;
				}
			}
			else
			{
				//cError("DefaultMessageProcessor: Error unpacking %s message.\n", jausMessageCommandCodeString(message));
			}
			break;

		case JAUS_QUERY_SERVICES:
			queryServices = queryServicesMessageFromJausMessage(message);
			if(queryServices)
			{
				// Respond with our services
				reportServices = reportServicesMessageCreate();

				jausAddressCopy(reportServices->destination, message->source);
				jausAddressCopy(reportServices->source, nmi->cmpt->address);
				jausServicesDestroy(reportServices->jausServices);
				reportServices->jausServices = jausServicesClone(nmi->cmpt->services);

				txMessage = reportServicesMessageToJausMessage(reportServices);
				reportServicesMessageDestroy(reportServices);

				nodeManagerSend(nmi, txMessage);
				jausMessageDestroy(txMessage);
				queryServicesMessageDestroy(queryServices);
			}
			else
			{
				//cError("DefaultMessageProcessor: Error unpacking %s message.\n", jausMessageCommandCodeString(message));
			}
			break;

		default:
			// Destroy the rxMessage
			jausAddressToString(message->source, string);
			//cError("DefaultMessageProcessor: Unhandled code: %s, from: %s\n", jausMessageCommandCodeString(message), string);
			break;
	}
}

