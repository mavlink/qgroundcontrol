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
// File Name: jaus.h
//
// Written By: Danny Kent (jaus AT dannykent DOT com), Tom Galluzzo (galluzzo AT gmail DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file is a wrapper for all other headers in jaus and will provide the user with access 
// to the complete jaus library

#ifndef JAUS_H
#define JAUS_H

#ifdef __cplusplus
extern "C" 
{
#endif

#ifdef WIN32
	#define JAUS_EXPORT	__declspec(dllexport)
#else
	#define JAUS_EXPORT
#endif

#define JAUS_LIBRARY_VERSION	"3.3.0"

typedef enum
{
	JAUS_FALSE	= 0,
	JAUS_TRUE	= 1
}JausBoolean;

#define JAUS_IDENTIFICATION_LENGTH_BYTES	80
				
#define JAUS_PI				3.14159265358979323846
#define JAUS_HALF_PI		1.570796326794897
#define JAUS_DEG_PER_RAD    57.2957795131
#define JAUS_RAD_PER_DEG	1.745329251994328e-2

// Define Target System Endianess Here
#if defined(__i386__) || defined(i386) || defined(_M_IX86) || defined(_X86_) || defined(__arm__)
	#define JAUS_LITTLE_ENDIAN 1
#elif defined(__ppc__) || defined(__powerpc) || defined(__powerpc__) || defined(__POWERPC__) || defined(_M_PPC)
	#define JAUS_BIG_ENDIAN 1
#else
	#error "Please define system endianess in jaus.h. #define either JAUS_LITTLE_ENDIAN or JAUS_BIG_ENDIAN"
#endif

#include "jausArray.h"
#include "type/jausType.h"

#include "jausAddress.h"
#include "jausState.h"
#include "jausSubsystem.h"
#include "jausNode.h"
#include "jausComponent.h"
#include "jausService.h"
#include "jausPayloadInterface.h"

#include "message/jausMessageHeaders.h"

#ifdef __cplusplus
}
#endif

#endif // JAUS_H
