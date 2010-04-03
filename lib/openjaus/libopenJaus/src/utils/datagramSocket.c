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
// File Name: datagramSocket.c
//
// Written By: Tom Galluzzo (galluzzo AT gmail DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file describes the functionality associated with a DatagramSocket object. 
// Inspired by the class of the same name in the JAVA language.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils/datagramSocket.h"

DatagramSocket datagramSocketCreate(short port, InetAddress ipAddress)
{
	DatagramSocket datagramSocket;
	struct sockaddr_in address;
	socklen_t addressLength = sizeof(address);

#ifdef WIN32	
	// Initialize the socket subsystem
	WSADATA wsaData;
	int err;
	err = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if(err != 0)
	{
		return NULL;
	}
#endif

	datagramSocket = (DatagramSocket) malloc( sizeof(DatagramSocketStruct) );
	if( datagramSocket == NULL )
	{
		return NULL;
	}
	
	// Open a socket with: Protocol Family (PF) IPv4, of Datagram Socket Type, and using UDP IP protocol explicitly
	datagramSocket->descriptor = (int) socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP); 
	if(datagramSocket->descriptor == -1)
	{
		return NULL;
	}
	
	memset(&address, 0, sizeof(address));					// Clear the data structure to zero
	address.sin_family = AF_INET;							// Set Internet Socket (sin), Family to: Address Family (AF) IPv4 (INET)
	address.sin_addr.s_addr = ipAddress->value;	// Set Internet Socket (sin), Address to: The ipAddressString argument
	address.sin_port = htons(port);							// Set Internet Socket (sin), Port to: the port argument

	// Bind our open socket to a free port on the localhost, with our defined ipAddress
	if(bind(datagramSocket->descriptor, (struct sockaddr *)&address, sizeof(address)))
	{
		CLOSE_SOCKET(datagramSocket->descriptor);
		return NULL;
	}

	if(getsockname(datagramSocket->descriptor, (struct sockaddr *)&address, &addressLength))
	{
		CLOSE_SOCKET(datagramSocket->descriptor);
		return NULL;
	}

	datagramSocket->address = ipAddress;
	datagramSocket->port = ntohs(address.sin_port);
	datagramSocket->timeout.tv_sec = 0;
	datagramSocket->timeout.tv_usec = 0;
	datagramSocket->blocking = 1;
	
	return datagramSocket;
}

void datagramSocketDestroy(DatagramSocket datagramSocket)
{
	CLOSE_SOCKET(datagramSocket->descriptor);
	free(datagramSocket);
#ifdef WIN32	
	// Initialize the socket subsystem
	WSACleanup();
#endif
}

int datagramSocketSend(DatagramSocket datagramSocket, DatagramPacket packet)
{
	struct sockaddr_in toAddress;
	int bytesSent = 0;

	memset(&toAddress, 0, sizeof(toAddress));
	toAddress.sin_family = AF_INET;									// Set Internet Socket (sin), Family to: Address Family (AF) IPv4 (INET)
	toAddress.sin_addr.s_addr = packet->address->value;				// Set Internet Socket (sin), Address to: The packet ipAddressString
	toAddress.sin_port = htons(packet->port);						// Set Internet Socket (sin), Port to: the packet port
	
	bytesSent = sendto(datagramSocket->descriptor, (void *)packet->buffer, packet->bufferSizeBytes, 0, (struct sockaddr *)&toAddress, sizeof(toAddress));
	return bytesSent;
}

int datagramSocketReceive(DatagramSocket datagramSocket, DatagramPacket packet)
{
	struct timeval timeout;
	struct timeval *timeoutPtr = NULL;
	fd_set readSet;
	struct sockaddr_in fromAddress;
	socklen_t fromAddressLength;
	int bytesReceived = 0;
	int selcetReturnVal = -2;

	memset(&fromAddress, 0, sizeof(fromAddress));
	
	if(!datagramSocket->blocking)
	{
		timeout = datagramSocket->timeout;
		timeoutPtr = &timeout;
	}
	
	FD_ZERO(&readSet);
	FD_SET(datagramSocket->descriptor, &readSet);

	selcetReturnVal = select(datagramSocket->descriptor + 1, &readSet, NULL, NULL, timeoutPtr);
	if(selcetReturnVal > 0)
	{
		if(FD_ISSET(datagramSocket->descriptor, &readSet))
		{
			fromAddressLength = sizeof(fromAddress);
			bytesReceived = recvfrom(datagramSocket->descriptor, packet->buffer, packet->bufferSizeBytes, 0, (struct sockaddr*)&fromAddress, &fromAddressLength);
			
			if(bytesReceived != -1)
			{
				packet->port = ntohs(fromAddress.sin_port);
				packet->address->value = fromAddress.sin_addr.s_addr;
			}
			return bytesReceived;	
		}
		else
		{
			return -2;
		}
	}
	else
	{
		return selcetReturnVal;
	}
}

void datagramSocketSetTimeout(DatagramSocket datagramSocket, double timeoutSec)
{
	long sec, usec;
	
	sec = (long)timeoutSec;
	usec = (long)((timeoutSec - (double)sec) * 1.0e6);
			
	datagramSocket->timeout.tv_sec = sec;
	datagramSocket->timeout.tv_usec = usec;
	datagramSocket->blocking = 0;
}



