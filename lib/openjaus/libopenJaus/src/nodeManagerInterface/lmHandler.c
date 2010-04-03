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
// File:		lmHandler.c
// Version:		3.3 BETA
// Written by:	Danny Kent (jaus AT dannykent DOT com)
// Date:		04/15/08
// Description:	LargeMessageHandler handles the queueing of large sets of Jaus Messages for 
//				use by the system. It will queue sets of messages until an end message is
//				recieved and then create a single Jaus Message from that collection

#include <stdlib.h>
#include "nodeManagerInterface/nodeManagerInterface.h"

LargeMessageHandler lmHandlerCreate(void)
{
	LargeMessageHandler lmh = (LargeMessageHandler)malloc( sizeof(LargeMessageHandlerStruct) );
	if(lmh)
	{
		lmh->messageLists = jausArrayCreate();
		return lmh;
	}
	else
	{
		return NULL;
	}
}

void lmHandlerDestroy(LargeMessageHandler lmh)
{
	jausArrayDestroy(lmh->messageLists, (void *)lmListDestroy);
	free(lmh);
}

LargeMessageList lmListCreate(void)
{
	LargeMessageList msgList = (LargeMessageList)malloc(sizeof(LargeMessageListStruct));
	msgList->commandCode = 0;
	msgList->source = jausAddressCreate();
	msgList->messages = jausArrayCreate();
	return msgList;
}

void lmListDestroy(LargeMessageList msgList)
{
	jausArrayDestroy(msgList->messages, (void *)jausMessageDestroy);
	jausAddressDestroy(msgList->source);
	free(msgList);
}

void lmHandlerReceiveLargeMessage(NodeManagerInterface nmi, JausMessage message)
{
	LargeMessageList msgList;
	JausMessage tempMessage;
	JausMessage outMessage;
	int i;
	unsigned long sequenceNumber;
	unsigned long index;
	JausUnsignedInteger newDataSize = 0;
	JausUnsignedInteger bufferIndex = 0;
	char address[128] = {0};
	
	switch(message->dataFlag)
	{
		case JAUS_FIRST_DATA_PACKET:
			// Check for valid SeqNumber(0), else Error
			if(message->sequenceNumber) 
			{
				//cError("LargeMessageHandler: Received First Data Packet with invalid Sequence Number(%d)\n", message->sequenceNumber);
				jausMessageDestroy(message);
				return;
			}
			
			// Check if LargeMessageList exists (same CC & Source)
			msgList = lmHandlerGetMessageList(nmi->lmh, message);
			if(msgList)
			{
				// Destroy the list and all messages
				jausArrayRemove(nmi->lmh->messageLists, msgList, (void *)lmHandlerMessageListEqual);
				lmListDestroy(msgList);		
			}

			// create LargeMessageList
			msgList = lmListCreate();
			jausArrayAdd(nmi->lmh->messageLists, msgList);
				
			// add message to LargeMessageList at first position
			msgList->commandCode = message->commandCode;
			jausAddressCopy(msgList->source, message->source);
			jausArrayAdd(msgList->messages, message);
			break;
		
		case JAUS_NORMAL_DATA_PACKET:
			// Check if LargeMessageList exists, error if not
			msgList = lmHandlerGetMessageList(nmi->lmh, message);
			if(msgList)
			{
				// Check if item exists in LargeMessageList with seqNumber
				if(jausArrayContains(msgList->messages, message, (void *)lmHandlerLargeMessageCheck) != -1)
				{
					//cError("LargeMessageHandler: Received duplicate NORMAL_DATA_PACKET\n");
					jausMessageDestroy(message);
				}
				else
				{
					// insert to jausArray
					jausArrayAdd(msgList->messages, message);
				}
			}
			else
			{
				// Destroy Message
				//cError("LargeMessageHandler: Received NORMAL_DATA_PACKET (0x%4X) for unknown Large Message Set (never received JAUS_FIRST_DATA_PACKET)\n", message->commandCode);
				jausMessageDestroy(message);
			}
			break;
			
		case JAUS_RETRANSMITTED_DATA_PACKET:
			// Check if LargeMessageList exists, error if not
			msgList = lmHandlerGetMessageList(nmi->lmh, message);
			if(msgList)
			{
				// Check if item exists in LargeMessageList with seqNumber
				if(jausArrayContains(msgList->messages, message, (void *)lmHandlerLargeMessageCheck) != -1)
				{
					tempMessage = (JausMessage) jausArrayRemove(msgList->messages, message, (void *)lmHandlerLargeMessageCheck);
					jausMessageDestroy(tempMessage);
				}
				// insert to jausArray
				jausArrayAdd(msgList->messages, message);
			}
			else
			{
				//cError("LargeMessageHandler: Received RETRANSMITTED_DATA_PACKET for unknown Large Message Set (never received JAUS_FIRST_DATA_PACKET)\n");
				jausMessageDestroy(message);				
			}
			break;
		
		case JAUS_LAST_DATA_PACKET:
			// Check if LargeMessageList exists, error if not
			msgList = lmHandlerGetMessageList(nmi->lmh, message);
			if(msgList && msgList->messages->elementCount > 0)
			{
				// insert message to end of list
				jausArrayAdd(msgList->messages, message);

				// Create JausMessage object
				outMessage = jausMessageCreate();

				// Calculate new message size
				newDataSize = 0;
				tempMessage = NULL;
				
				for(i = 0; i < msgList->messages->elementCount; i++)
				{
					tempMessage = (JausMessage)msgList->messages->elementData[i];
					newDataSize += tempMessage->dataSize;
				}

				// Setup Header and Data Buffer
				outMessage->properties = tempMessage->properties;
				outMessage->commandCode = tempMessage->commandCode;
				jausAddressCopy(outMessage->destination, tempMessage->destination);
				jausAddressCopy(outMessage->source, tempMessage->source);
				outMessage->dataSize = tempMessage->dataSize;
				outMessage->dataFlag = tempMessage->dataFlag;
				outMessage->sequenceNumber = tempMessage->sequenceNumber;
				outMessage->data = (unsigned char *) malloc(newDataSize);
				
				// Populate new message
				sequenceNumber = 0;
				bufferIndex = 0;
				while(sequenceNumber <= message->sequenceNumber)
				{
					index = 0;
					do
					{
						tempMessage = (JausMessage)msgList->messages->elementData[index];
						index++;
						if(index > (unsigned int) msgList->messages->elementCount)
						{
							// Invalid set of messages
							//TODO: Here is when you would request a retransmittal if ACK/NAK is set and ask the sending component to resend some packets
	
							// Received LAST_DATA_PACKET, but do not have proper sequence of messages
							// Destroy the list and all messages
							jausArrayRemove(nmi->lmh->messageLists, msgList, (void *)lmHandlerMessageListEqual);
							lmListDestroy(msgList);
						
							//cError("LargeMessageHandler: Received LAST_DATA_PACKET, but do not have proper sequence of messages\n");						
							jausMessageDestroy(outMessage);
							return;
						}
					}
					while(tempMessage->sequenceNumber != sequenceNumber);
					
					// Move data from tempMessage to outMessage
					memcpy(outMessage->data + bufferIndex, tempMessage->data, tempMessage->dataSize);
					bufferIndex += tempMessage->dataSize;
					sequenceNumber++; // TOM: switched from i++
				}

				// Set DataSize
				// Set proper header flags (dataFlag JAUS_SINGLE_DATA_PACKET)
				outMessage->dataSize = newDataSize;
				outMessage->dataFlag = JAUS_SINGLE_DATA_PACKET;

				if(outMessage->properties.scFlag)
				{
					scManagerReceiveMessage(nmi, outMessage);
				}
				else
				{
					queuePush(nmi->receiveQueue, (void *)outMessage);
				}
				
				// Destroy LargeMessageList
				jausArrayRemove(nmi->lmh->messageLists, msgList, (void *)lmHandlerMessageListEqual);
				lmListDestroy(msgList);
			}
			else
			{
				//cError("LargeMessageHandler: Received LAST_DATA_PACKET for unknown Large Message Set (never received JAUS_FIRST_DATA_PACKET)\n");
				jausMessageDestroy(message);				
			}
			break;
		
		default:
			jausAddressToString(message->source, address);
			//cError("lmHandler: Received (%s) with improper dataFlag (%d) from %s\n", jausMessageCommandCodeString(message), message->dataFlag, address);
			jausMessageDestroy(message);
			break;
	}
}

int lmHandlerMessageListEqual(LargeMessageList listOne, LargeMessageList listTwo)
{
	if(listOne->commandCode == listTwo->commandCode && jausAddressEqual(listOne->source, listTwo->source))
	{
		return 1;
	}
	else
	{
		return 0;
	}	
}

LargeMessageList lmHandlerGetMessageList(LargeMessageHandler lmh, JausMessage message)
{
	int i;
	LargeMessageList msgList;
	
	// Look for the matching LargeMessageList object in the jausArray array
	for(i = 0; i < lmh->messageLists->elementCount; i++)
	{
		msgList = (LargeMessageList) lmh->messageLists->elementData[i];

		// Command Code and Source uniquely identify a message list
		if(msgList->commandCode == message->commandCode && jausAddressEqual(msgList->source, message->source))
			return msgList;
	}
	return NULL;
}

int lmHandlerLargeMessageCheck(JausMessage messageOne, JausMessage messageTwo)
{
	// Check if two messages are equal for LargeMessageSet purposes
	// This only applies to large message set tests
	if(messageOne->commandCode == messageTwo->commandCode && 
	   jausAddressEqual(messageOne->source, messageTwo->source) &&
	   messageOne->sequenceNumber == messageTwo->sequenceNumber)
	{
		return JAUS_TRUE;
	}
	else
	{
		return JAUS_FALSE;
	}		
}

// Note: It is left to the user to destroy the inMessage after a call to this function
int lmHandlerSendLargeMessage(NodeManagerInterface nmi, JausMessage inMessage)
{
	unsigned int bytesRemaining = inMessage->dataSize;
	unsigned int bufferIndex = 0;
	int messageCount = 0;
	int result = -1;
	JausMessage outMessage;
	
	if(inMessage->dataSize > JAUS_MAX_DATA_SIZE_BYTES)
	{
		while(bytesRemaining > 	JAUS_MAX_DATA_SIZE_BYTES)
		{
			outMessage = jausMessageCreate();
	
			// Copy Header
			outMessage->properties = inMessage->properties;
			outMessage->commandCode = inMessage->commandCode;
			jausAddressCopy(outMessage->destination, inMessage->destination);
			jausAddressCopy(outMessage->source, inMessage->source);
			outMessage->dataSize = inMessage->dataSize;
			outMessage->dataFlag = inMessage->dataFlag;
			outMessage->sequenceNumber = messageCount++;

			if(messageCount == 1)
			{
				// First Message
				outMessage->dataFlag = JAUS_FIRST_DATA_PACKET;
			}
			else
			{
				// Normal Message
				outMessage->dataFlag = JAUS_NORMAL_DATA_PACKET;
			}
			
			// Copy Data to new memory location
			outMessage->data = (JausByte *)malloc(JAUS_MAX_DATA_SIZE_BYTES);
			outMessage->dataSize = JAUS_MAX_DATA_SIZE_BYTES;

			memcpy(outMessage->data, inMessage->data + bufferIndex, JAUS_MAX_DATA_SIZE_BYTES);
			bufferIndex += JAUS_MAX_DATA_SIZE_BYTES;
			bytesRemaining -= JAUS_MAX_DATA_SIZE_BYTES;

			result = nodeManagerSendSingleMessage(nmi, outMessage);
			jausMessageDestroy(outMessage);
		}
		
		// Last Message
		outMessage = jausMessageCreate();

		// Copy Header
		outMessage->properties = inMessage->properties;
		outMessage->commandCode = inMessage->commandCode;
		jausAddressCopy(outMessage->destination, inMessage->destination);
		jausAddressCopy(outMessage->source, inMessage->source);
		outMessage->dataSize = inMessage->dataSize;
		outMessage->dataFlag = inMessage->dataFlag;
		outMessage->sequenceNumber = messageCount++;
		
		outMessage->dataFlag = JAUS_LAST_DATA_PACKET;
		outMessage->dataSize = bytesRemaining;
		
		// Copy data to new memory location
		outMessage->data = (JausByte *)malloc(bytesRemaining);
		memcpy(outMessage->data, inMessage->data + bufferIndex, bytesRemaining);
		
		result = nodeManagerSendSingleMessage(nmi, outMessage);
		jausMessageDestroy(outMessage);
	}
	else
	{
		// Message in Large Message Handler which is NOT a large message
		// Pass through
		result = nodeManagerSendSingleMessage(nmi, inMessage);
	}
	
	return result;
}
