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
// File Name: jausWorldModelVectorObject.h
//
// Written By: Danny Kent (jaus AT dannykent DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file describes all the functionality associated with a JausWorldModelVectorObject. 
// JausWorldModelVectorObjects are used to support the storage and transfer of vector objects using the World Model message set.

#ifndef JAUS_WM_VECTOR_OBJECT_H
#define JAUS_WM_VECTOR_OBJECT_H

#include "jaus.h"
#include "string.h"

// Vector Object types as defined in WMVKS Document v1.3
#ifndef JAUS_WM_OBJECT_TYPES
#define JAUS_WM_OBJECT_TYPES
#define JAUS_WM_UNKNOWN_TYPE	255
#define JAUS_WM_POINT_TYPE		0
#define JAUS_WM_LINE_TYPE		1
#define JAUS_WM_POLYGON_TYPE	2
#endif

// Attribute Data Types as defined in WMVKS Document v1.3
#ifndef JAUS_WM_OBJECT_DATA_TYPE
#define JAUS_WM_OBJECT_DATA_TYPE
#define JAUS_WM_OBJECT_DEFAULT_DATA				255
#define JAUS_WM_OBJECT_BYTE_DATA				0
#define JAUS_WM_OBJECT_SHORT_DATA				1
#define JAUS_WM_OBJECT_INTEGER_DATA				2
#define JAUS_WM_OBJECT_LONG_DATA				3
#define JAUS_WM_OBJECT_UNSIGNED_SHORT_DATA		4
#define JAUS_WM_OBJECT_UNSIGNED_INTEGER_DATA	5
#define JAUS_WM_OBJECT_UNSIGNED_LONG_DATA		6
#define JAUS_WM_OBJECT_FLOAT_DATA				7
#define JAUS_WM_OBJECT_DOUBLE_DATA				8
#define JAUS_WM_OBJECT_RGB_DATA					9
#endif

// ************************************************************************************************************************************
//			JausWorldModelVectorObject
// ************************************************************************************************************************************
typedef struct
{
	JausUnsignedShort id;		// Unique Object Id
	JausByte type;				// Enumeration, see above
	JausFloat bufferMeters;		// Buffer Size in meters
	JausArray featureClasses;		// Dynamic Array of FeatureClass data
	JausArray dataPoints;			// Dynamic Array of PointLla data
}JausWorldModelVectorObjectStruct;
typedef JausWorldModelVectorObjectStruct *JausWorldModelVectorObject;

// JausWorldModelVectorObject Constructor
JAUS_EXPORT JausWorldModelVectorObject vectorObjectCreate(void);

// JausWorldModelVectorObject Constructor (from Buffer)
JAUS_EXPORT JausWorldModelVectorObject vectorObjectFromBuffer(unsigned char *buffer, unsigned int bufferSizeBytes, JausBoolean objectBuffered);

// JausWorldModelVectorObject To Buffer
JAUS_EXPORT JausBoolean vectorObjectToBuffer(JausWorldModelVectorObject object, unsigned char *buffer, unsigned int bufferSizeBytes, JausBoolean objectBuffered);

// JausWorldModelVectorObject Destructor
JAUS_EXPORT void vectorObjectDestroy(JausWorldModelVectorObject object);

// JausWorldModelVectorObject Buffer Size
JAUS_EXPORT unsigned int vectorObjectSizeBytes(JausWorldModelVectorObject object, JausBoolean objectBuffered);

// JausWorldModelVectorObject To String
JAUS_EXPORT int vectorObjectTypeToString(JausWorldModelVectorObject object, char *string, size_t stringLength);

// JausWorldModelVectorObject Copy
JAUS_EXPORT JausWorldModelVectorObject vectorObjectCopy(JausWorldModelVectorObject input);

#endif // VECTOROBJECT_H
