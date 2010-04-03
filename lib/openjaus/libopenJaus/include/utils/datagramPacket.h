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
// File Name: datagramPacket.h
//
// Written By: Tom Galluzzo (galluzzo AT gmail DOT com) and Danny Kent
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file describes the functionality associated with a DatagramPacket object. 
// Inspired by the class of the same name in the JAVA language.

#ifndef DATAGRAM_PACKET_H
#define DATAGRAM_PACKET_H

#ifdef WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
    #ifndef socklen_t
        typedef int socklen_t;
    #endif
	#define CLOSE_SOCKET closesocket
	typedef unsigned int size_t;
#elif defined(__linux) || defined(linux) || defined(__linux__) || defined(__APPLE__)
	#include <unistd.h>
	#include <sys/socket.h>
	#include <netdb.h>
	#include <arpa/inet.h>
	#include <sys/time.h>
	#include <netinet/in.h>
	#include <sys/types.h>
	#define CLOSE_SOCKET close
#else
	#error "No Socket implementation defined for this platform."
#endif

#include "utils/inetAddress.h"

#ifdef WIN32
	#define JAUS_EXPORT	__declspec(dllexport)
#else
	#define JAUS_EXPORT
#endif

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
	unsigned char *buffer;

#if defined(__linux) || defined(linux) || defined(__linux__) || defined(__APPLE__)
	size_t bufferSizeBytes;
#else
	int bufferSizeBytes;
#endif

	unsigned short port;
	InetAddress address;
}DatagramPacketStruct;

typedef DatagramPacketStruct *DatagramPacket;

JAUS_EXPORT DatagramPacket datagramPacketCreate(void);
JAUS_EXPORT void datagramPacketDestroy(DatagramPacket);

#ifdef __cplusplus
}
#endif

#endif // DATAGRAM_PACKET_H
