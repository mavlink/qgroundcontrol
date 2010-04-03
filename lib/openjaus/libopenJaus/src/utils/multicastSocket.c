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
// File Name: multicastSocket.c
//
// Written By: Tom Galluzzo (galluzzo AT gmail DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description:	This file describes the functionality associated with a MulicastSocket object.
// Inspired by the class of the same name in the JAVA language.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils/multicastSocket.h"

MulticastSocket multicastSocketCreate(short port, InetAddress ipAddress)
{
	MulticastSocket multicastSocket;
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

	multicastSocket = (MulticastSocket)malloc( sizeof(MulticastSocketStruct) );
	if(multicastSocket == NULL)
	{
		return NULL;
	}
	multicastSocket->address = inetAddressCreate();
	multicastSocket->unicastSocketDescriptor = -1;
	multicastSocket->multicastSocketDescriptor = -1;
	
	// Open a socket with: Protocol Family (PF) IPv4, of Datagram Socket Type, and using UDP IP protocol explicitly
	multicastSocket->unicastSocketDescriptor = (int) socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP); 
	if(multicastSocket->unicastSocketDescriptor  == -1)
	{
		multicastSocketDestroy(multicastSocket);
		return NULL;
	}

	memset(&address, 0, sizeof(address));			// Clear the data structure to zero
	address.sin_family = AF_INET;					// Set Internet Socket (sin), Family to: Address Family (AF) IPv4 (INET)
	address.sin_addr.s_addr = ipAddress->value;		// Set Internet Socket (sin), Address to: The ipAddressString argument
	address.sin_port = htons(port);					// Set Internet Socket (sin), Port to: the port argument
	
	// Bind our open socket to a free port on the given interface, with our defined ipAddress
	if(bind(multicastSocket->unicastSocketDescriptor, (struct sockaddr *)&address, sizeof(address)))
	{
		multicastSocketDestroy(multicastSocket);
		return NULL;
	}

	if(getsockname(multicastSocket->unicastSocketDescriptor, (struct sockaddr *)&address, &addressLength))
	{
		multicastSocketDestroy(multicastSocket);
		return NULL;
	}

	// Tell the kernel to send multicast packets from this interface
	if(setsockopt(multicastSocket->unicastSocketDescriptor, IPPROTO_IP, IP_MULTICAST_IF, (char *)&ipAddress->value, sizeof(ipAddress->value)))
	{
		multicastSocketDestroy(multicastSocket);
		return NULL;
	}

	ipAddress->value = address.sin_addr.s_addr;
	multicastSocket->address->value = ipAddress->value;
	multicastSocket->port = ntohs(address.sin_port);
	multicastSocket->timeout.tv_sec = 0;
	multicastSocket->timeout.tv_usec = 0;
	multicastSocket->blocking = 1;
	multicastSocket->multicastSocketDescriptor = -1;
	
	return multicastSocket;
}

void multicastSocketDestroy(MulticastSocket multicastSocket)
{
	if(multicastSocket->unicastSocketDescriptor != -1)
	{
		CLOSE_SOCKET(multicastSocket->unicastSocketDescriptor);
	}
	
	if(multicastSocket->multicastSocketDescriptor != -1)
	{
		CLOSE_SOCKET(multicastSocket->multicastSocketDescriptor);
	}
	
	inetAddressDestroy(multicastSocket->address);	
	free(multicastSocket);

#ifdef WIN32	
	// Initialize the socket subsystem
	WSACleanup();
#endif

}

int multicastSocketJoinGroup(MulticastSocket multicastSocket, InetAddress groupIpAddress)
{
	struct ip_mreq multicastRequest;
	
	if(multicastSocket == NULL)
	{
		return -1;
	}

	if(groupIpAddress == NULL)
	{
		return -1;
	}

	multicastRequest.imr_multiaddr.s_addr = groupIpAddress->value;
	multicastRequest.imr_interface.s_addr = multicastSocket->address->value;

#if defined(__linux) || defined(linux) || defined(__linux__) || defined(__APPLE__)
	// Open a socket with: Protocol Family (PF) IPv4, of Datagram Socket Type, and using UDP IP protocol explicitly
	multicastSocket->multicastSocketDescriptor = (int) socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP); 
	if(multicastSocket->multicastSocketDescriptor  == -1)
	{
		return -1;
	}

	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));				// Clear the data structure to zero
	address.sin_family = AF_INET;						// Set Internet Socket (sin), Family to: Address Family (AF) IPv4 (INET)
	address.sin_addr.s_addr = groupIpAddress->value;	// Set Internet Socket (sin), Address to: The ipAddressString argument
	address.sin_port = htons(multicastSocket->port);	// Set Internet Socket (sin), Port to: the port argument

	// Bind our open socket to a free port on the given interface, with our defined ipAddress
	if(bind(multicastSocket->multicastSocketDescriptor, (struct sockaddr *)&address, sizeof(address)))
	{
		printf("Error: Cannot bind multicast socket to %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
		CLOSE_SOCKET(multicastSocket->multicastSocketDescriptor);
		return -1;
	}
	
	return setsockopt(multicastSocket->multicastSocketDescriptor, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&multicastRequest, sizeof(multicastRequest)); 
#else
	return setsockopt(multicastSocket->unicastSocketDescriptor, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&multicastRequest, sizeof(multicastRequest)); 
#endif

}

int multicastSocketSend(MulticastSocket multicastSocket, DatagramPacket packet)
{
	struct sockaddr_in toAddress;
	int bytesSent = 0;

	memset(&toAddress, 0, sizeof(toAddress));
	toAddress.sin_family = AF_INET;							// Set Internet Socket (sin), Family to: Address Family (AF) IPv4 (INET)
	toAddress.sin_addr.s_addr = packet->address->value;		// Set Internet Socket (sin), Address to: The packet ipAddressString
	toAddress.sin_port = htons(packet->port);				// Set Internet Socket (sin), Port to: the packet port
	
	bytesSent = sendto(multicastSocket->unicastSocketDescriptor, (void *)packet->buffer, packet->bufferSizeBytes, 0, (struct sockaddr *)&toAddress, sizeof(toAddress));

	return bytesSent;
}

int multicastSocketReceive(MulticastSocket multicastSocket, DatagramPacket packet)
{
	struct timeval timeout;
	struct timeval *timeoutPtr = NULL;
	fd_set readSet;
	struct sockaddr_in fromAddress;
	socklen_t fromAddressLength;
	int bytesReceived = 0;
	int count = 0;
	int socket = 0;
	
	memset(&fromAddress, 0, sizeof(fromAddress));
	
	if(!multicastSocket->blocking)
	{
		timeout = multicastSocket->timeout;
		timeoutPtr = &timeout;
	}

	FD_ZERO(&readSet);
	FD_SET(multicastSocket->unicastSocketDescriptor, &readSet);
	socket = multicastSocket->unicastSocketDescriptor;
	
	if(multicastSocket->multicastSocketDescriptor != -1)
	{
		FD_SET(multicastSocket->multicastSocketDescriptor, &readSet);
		socket = multicastSocket->multicastSocketDescriptor;
	}

	count = select(socket + 1, &readSet, NULL, NULL, timeoutPtr);
	if(count > 0)
	{
		fromAddressLength = sizeof(fromAddress);

		if(FD_ISSET(multicastSocket->unicastSocketDescriptor, &readSet))
		{
			bytesReceived = recvfrom(multicastSocket->unicastSocketDescriptor, packet->buffer, packet->bufferSizeBytes, 0, (struct sockaddr*)&fromAddress, &fromAddressLength);
		}
		if(multicastSocket->multicastSocketDescriptor != -1 && FD_ISSET(multicastSocket->multicastSocketDescriptor, &readSet))
		{
			bytesReceived = recvfrom(multicastSocket->multicastSocketDescriptor, packet->buffer, packet->bufferSizeBytes, 0, (struct sockaddr*)&fromAddress, &fromAddressLength);
		}
	}
	else
	{
		return -1;
	}
	
	if(bytesReceived != -1)
	{
		packet->port = ntohs(fromAddress.sin_port);
		packet->address->value = fromAddress.sin_addr.s_addr;
		return bytesReceived;
	}
	else
	{
		return -1;
	}
}

void multicastSocketSetTimeout(MulticastSocket multicastSocket, double timeoutSec)
{
	long sec, usec;
	
	sec = (long)timeoutSec;
	usec = (long)((timeoutSec - (double)sec) * 1.0e6);
			
	multicastSocket->timeout.tv_sec = sec;
	multicastSocket->timeout.tv_usec = usec;
	multicastSocket->blocking = 0;
}

int multicastSocketSetTTL(MulticastSocket multicastSocket, unsigned int ttl)
{
	if(ttl > 255)
	{
		ttl = 255;
	}

	if(ttl < 1)
	{
		ttl = 1;
	}

	if(multicastSocket->multicastSocketDescriptor != -1)
	{
		return (setsockopt(multicastSocket->multicastSocketDescriptor, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&ttl, sizeof(unsigned int)) &&
				setsockopt(multicastSocket->unicastSocketDescriptor, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&ttl, sizeof(unsigned int)) );
	}
	else
	{
		return setsockopt(multicastSocket->unicastSocketDescriptor, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&ttl, sizeof(unsigned int));
	}
}

int multicastSocketSetLoopback(MulticastSocket multicastSocket, unsigned int loop)
{
		return setsockopt(multicastSocket->unicastSocketDescriptor, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loop, sizeof(unsigned int));
}

