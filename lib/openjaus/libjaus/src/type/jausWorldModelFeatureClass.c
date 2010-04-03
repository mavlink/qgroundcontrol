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
// File Name: jausWorldModelFeatureClass.c
//
// Written By: Danny Kent (jaus AT dannykent DOT com), Tom Galluzzo (galluzzo AT gmail DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description:

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "jaus.h"

// JausWorldModelFeatureClass Constructor
JausWorldModelFeatureClass featureClassCreate(void)
{
	JausWorldModelFeatureClass fcClass;
	fcClass = (JausWorldModelFeatureClass) malloc(sizeof(JausWorldModelFeatureClassStruct));
	if(fcClass)
	{
		fcClass->id = newJausUnsignedShort(0);							// Enumeration, defined by system
		memset(fcClass->metaData, 0, JAUS_WM_FC_METADATA_STRING_LENGTH);	// String of Metadata information defined for this Feature Class
		fcClass->attribute = featureClassAttributeCreate();				// Feature Class Attribute data type and value
		return fcClass;
	}
	else
	{
		return NULL;
	}
}

// JausWorldModelFeatureClass Constructor (from Buffer)
JausWorldModelFeatureClass featureClassFromBuffer(unsigned char *buffer, unsigned int bufferSizeBytes)
{
	unsigned int index = 0;
	JausWorldModelFeatureClass fcClass;
	
	fcClass = (JausWorldModelFeatureClass) malloc(sizeof(JausWorldModelFeatureClassStruct));
	if(fcClass)
	{
		if(!jausUnsignedShortFromBuffer(&fcClass->id, buffer+index, bufferSizeBytes-index))
		{
			free(fcClass);
			return NULL;
		}
		index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
	
		fcClass->attribute = featureClassAttributeFromBuffer(buffer+index, bufferSizeBytes-index);
		if(!fcClass->attribute)
		{
			free(fcClass);
			return NULL;
		}

		return fcClass;
	}
	else
	{
		return NULL;
	}
}

// JausWorldModelFeatureClass To Buffer
JausBoolean featureClassToBuffer(JausWorldModelFeatureClass fcClass, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	unsigned int index = 0;
	
	if(fcClass)
	{
		// FC Id
		if(!jausUnsignedShortToBuffer(fcClass->id, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
		
		// FC Attribute Data Type & Value
		if(!featureClassAttributeToBuffer(fcClass->attribute, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		return JAUS_TRUE;
	}
	return JAUS_FALSE;
}

// JausWorldModelFeatureClass Destructor
void featureClassDestroy(JausWorldModelFeatureClass fcClass)
{
	if(fcClass)
	{
		featureClassAttributeDestroy(fcClass->attribute);
		free(fcClass);
		fcClass = NULL;
	}
}

// JausWorldModelFeatureClass Buffer Size
unsigned int featureClassSizeBytes(JausWorldModelFeatureClass fcClass)
{
	unsigned int size = 0;
	
	if(fcClass)
	{
		size += JAUS_UNSIGNED_SHORT_SIZE_BYTES;	// Class Id
		size += featureClassAttributeSizeBytes(fcClass->attribute);	// Size of attribute & data type
	}
	return size;
}

// JausWorldModelFeatureClass Copy
JausWorldModelFeatureClass featureClassCopy(JausWorldModelFeatureClass fcClass)
{
	JausWorldModelFeatureClass classCopy;
	
	classCopy = (JausWorldModelFeatureClass) malloc(sizeof(JausWorldModelFeatureClassStruct));
	if(classCopy)
	{
		memcpy(classCopy, fcClass, sizeof(JausWorldModelFeatureClassStruct));
		classCopy->attribute = featureClassAttributeCopy(fcClass->attribute);
		if(!classCopy->attribute)
		{
			free(classCopy);
			return NULL;
		}
		return classCopy;
	}
	else
	{
		return NULL;
	}
}

// JausWorldModelFeatureClassAttribute Constructor
JausWorldModelFeatureClassAttribute featureClassAttributeCreate(void)
{
	JausWorldModelFeatureClassAttribute attribute;
	attribute = (JausWorldModelFeatureClassAttribute) malloc(sizeof(JausWorldModelFeatureClassAttributeStruct));
	if(attribute)
	{
		memset(attribute, 0, sizeof(JausWorldModelFeatureClassAttributeStruct));
		attribute->dataType = newJausByte(JAUS_WM_OBJECT_DEFAULT_DATA);
		return attribute;
	}
	else
	{
		return NULL;
	}
}

// JausWorldModelFeatureClassAttribute Destructor
void featureClassAttributeDestroy(JausWorldModelFeatureClassAttribute attribute)
{
	if(attribute)
	{
		free(attribute);
		attribute = NULL;
	}
}

// JausWorldModelFeatureClassAttribute Constructor (from Buffer)
JausWorldModelFeatureClassAttribute featureClassAttributeFromBuffer(unsigned char *buffer, unsigned int bufferSizeBytes)
{
	unsigned int index = 0;
	JausWorldModelFeatureClassAttribute attribute;

	attribute = (JausWorldModelFeatureClassAttribute) malloc(sizeof(JausWorldModelFeatureClassAttributeStruct));
	if(attribute)
	{
		// Data Type
		if(!jausByteFromBuffer(&attribute->dataType, buffer+index, bufferSizeBytes-index))
		{
			free(attribute);
			return NULL;
		}
		index += JAUS_BYTE_SIZE_BYTES;
		
		switch(attribute->dataType)
		{
			case JAUS_WM_OBJECT_BYTE_DATA:
				if(!jausByteFromBuffer(&attribute->data.byteValue, buffer+index, bufferSizeBytes-index))
				{
					free(attribute);
					return NULL;
				}
				return attribute;
				
			case JAUS_WM_OBJECT_SHORT_DATA:
				if(!jausShortFromBuffer(&attribute->data.shortValue, buffer+index, bufferSizeBytes-index))
				{
					free(attribute);
					return NULL;
				}
				return attribute;

			case JAUS_WM_OBJECT_INTEGER_DATA:
				if(!jausIntegerFromBuffer(&attribute->data.integerValue, buffer+index, bufferSizeBytes-index))
				{
					free(attribute);
					return NULL;
				}
				return attribute;

			case JAUS_WM_OBJECT_LONG_DATA:
				if(!jausLongFromBuffer(&attribute->data.longValue, buffer+index, bufferSizeBytes-index))
				{
					free(attribute);
					return NULL;
				}
				return attribute;

			case JAUS_WM_OBJECT_UNSIGNED_SHORT_DATA:
				if(!jausUnsignedShortFromBuffer(&attribute->data.unsignedShortValue, buffer+index, bufferSizeBytes-index))
				{
					free(attribute);
					return NULL;
				}
				return attribute;

			case JAUS_WM_OBJECT_UNSIGNED_INTEGER_DATA:
				if(!jausUnsignedIntegerFromBuffer(&attribute->data.unsignedIntegerValue, buffer+index, bufferSizeBytes-index))
				{
					free(attribute);
					return NULL;
				}
				return attribute;

			case JAUS_WM_OBJECT_UNSIGNED_LONG_DATA:
				if(!jausUnsignedLongFromBuffer(&attribute->data.unsignedLongValue, buffer+index, bufferSizeBytes-index))
				{
					free(attribute);
					return NULL;
				}
				return attribute;

			case JAUS_WM_OBJECT_FLOAT_DATA:
				if(!jausFloatFromBuffer(&attribute->data.floatValue, buffer+index, bufferSizeBytes-index))
				{
					free(attribute);
					return NULL;
				}
				return attribute;

			case JAUS_WM_OBJECT_DOUBLE_DATA:
				if(!jausDoubleFromBuffer(&attribute->data.doubleValue, buffer+index, bufferSizeBytes-index))
				{
					free(attribute);
					return NULL;
				}
				return attribute;

				break;

			case JAUS_WM_OBJECT_RGB_DATA:
				if(!jausByteFromBuffer(&attribute->data.rgb.redValue, buffer+index, bufferSizeBytes-index))
				{
					free(attribute);
					return NULL;
				}
				index += JAUS_BYTE_SIZE_BYTES;

				if(!jausByteFromBuffer(&attribute->data.rgb.greenValue, buffer+index, bufferSizeBytes-index))
				{
					free(attribute);
					return NULL;
				}
				index += JAUS_BYTE_SIZE_BYTES;

				if(!jausByteFromBuffer(&attribute->data.rgb.blueValue, buffer+index, bufferSizeBytes-index))
				{
					free(attribute);
					return NULL;
				}
				index += JAUS_BYTE_SIZE_BYTES;

				return attribute;

			default:
				free(attribute);
				return NULL;
		}
	}
	else
	{
		return NULL;
	}
}

// JausWorldModelFeatureClassAttribute To Buffer
JausBoolean featureClassAttributeToBuffer(JausWorldModelFeatureClassAttribute attribute, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	unsigned int index = 0;
	if(attribute)
	{
		// Data Type
		if(!jausByteToBuffer(attribute->dataType, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_BYTE_SIZE_BYTES;
		
		// Value
		switch(attribute->dataType)
		{
			case JAUS_WM_OBJECT_BYTE_DATA:
				if(!jausByteToBuffer(attribute->data.byteValue, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				return JAUS_TRUE;
								
			case JAUS_WM_OBJECT_SHORT_DATA:
				if(!jausShortToBuffer(attribute->data.shortValue, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				return JAUS_TRUE;

			case JAUS_WM_OBJECT_INTEGER_DATA:
				if(!jausIntegerToBuffer(attribute->data.integerValue, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				return JAUS_TRUE;

			case JAUS_WM_OBJECT_LONG_DATA:
				if(!jausLongToBuffer(attribute->data.longValue, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				return JAUS_TRUE;

			case JAUS_WM_OBJECT_UNSIGNED_SHORT_DATA:
				if(!jausUnsignedShortToBuffer(attribute->data.unsignedShortValue, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				return JAUS_TRUE;

			case JAUS_WM_OBJECT_UNSIGNED_INTEGER_DATA:
				if(!jausUnsignedIntegerToBuffer(attribute->data.unsignedIntegerValue, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				return JAUS_TRUE;

			case JAUS_WM_OBJECT_UNSIGNED_LONG_DATA:
				if(!jausUnsignedLongToBuffer(attribute->data.unsignedLongValue, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				return JAUS_TRUE;

			case JAUS_WM_OBJECT_FLOAT_DATA:
				if(!jausFloatToBuffer(attribute->data.floatValue, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				return JAUS_TRUE;

			case JAUS_WM_OBJECT_DOUBLE_DATA:
				if(!jausDoubleToBuffer(attribute->data.doubleValue, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				return JAUS_TRUE;

			case JAUS_WM_OBJECT_RGB_DATA:
				if(!jausByteToBuffer(attribute->data.rgb.redValue, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				index += JAUS_BYTE_SIZE_BYTES;

				if(!jausByteToBuffer(attribute->data.rgb.greenValue, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				index += JAUS_BYTE_SIZE_BYTES;

				if(!jausByteToBuffer(attribute->data.rgb.blueValue, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				return JAUS_TRUE;

			default:
				return JAUS_TRUE;
		}
	}
	return JAUS_FALSE;
}

// JausWorldModelFeatureClassAttribute Buffer Size
unsigned int featureClassAttributeSizeBytes(JausWorldModelFeatureClassAttribute attribute)
{
	unsigned int size = 0;
	
	if(attribute)
	{
		size += JAUS_BYTE_SIZE_BYTES;
		switch(attribute->dataType)
		{
			case JAUS_WM_OBJECT_BYTE_DATA:
				size += JAUS_BYTE_SIZE_BYTES;
				return size;
				break;
				
			case JAUS_WM_OBJECT_SHORT_DATA:
				size += JAUS_SHORT_SIZE_BYTES;
				return size;
				break;

			case JAUS_WM_OBJECT_INTEGER_DATA:
				size += JAUS_INTEGER_SIZE_BYTES;
				return size;
				break;

			case JAUS_WM_OBJECT_LONG_DATA:
				size += JAUS_LONG_SIZE_BYTES;
				return size;
				break;

			case JAUS_WM_OBJECT_UNSIGNED_SHORT_DATA:
				size += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
				return size;
				break;

			case JAUS_WM_OBJECT_UNSIGNED_INTEGER_DATA:
				size += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
				return size;
				break;

			case JAUS_WM_OBJECT_UNSIGNED_LONG_DATA:
				size += JAUS_UNSIGNED_LONG_SIZE_BYTES;
				return size;
				break;

			case JAUS_WM_OBJECT_FLOAT_DATA:
				size += JAUS_FLOAT_SIZE_BYTES;
				return size;
				break;

			case JAUS_WM_OBJECT_DOUBLE_DATA:
				size += JAUS_DOUBLE_SIZE_BYTES;
				return size;
				break;

			case JAUS_WM_OBJECT_RGB_DATA:
				size += JAUS_BYTE_SIZE_BYTES;	// Red
				size += JAUS_BYTE_SIZE_BYTES;	// Green
				size += JAUS_BYTE_SIZE_BYTES;	// Blue
				return size;
				break;

			default:
				return 0;			
		}
	}
	return size;
}

// JausWorldModelFeatureClassAttribute Copy
JausWorldModelFeatureClassAttribute featureClassAttributeCopy(JausWorldModelFeatureClassAttribute attribute)
{
	JausWorldModelFeatureClassAttribute copy;
	
	copy = (JausWorldModelFeatureClassAttribute) malloc(sizeof(JausWorldModelFeatureClassAttributeStruct));
	if(copy)
	{
		memcpy(copy, attribute, sizeof(JausWorldModelFeatureClassAttributeStruct));
		return copy;
	}
	else
	{
		return NULL;
	}
}

// JausWorldModelFeatureClassAttribute ToString
int featureClassAttributeToString(JausWorldModelFeatureClassAttribute attribute, char *string, size_t stringLength)
{
	int retVal = 0;

	switch(attribute->dataType)
	{
		case JAUS_WM_OBJECT_BYTE_DATA:
			retVal = sprintf(string, "Byte Attribute (%d)", attribute->data.byteValue);
			return retVal;
						
		case JAUS_WM_OBJECT_SHORT_DATA:
			retVal = sprintf(string, "Short Attribute (%d)", attribute->data.shortValue);
			return retVal;

		case JAUS_WM_OBJECT_INTEGER_DATA:
			retVal = sprintf(string, "Integer Attribute (%d)", attribute->data.integerValue);
			return retVal;

		case JAUS_WM_OBJECT_LONG_DATA:
			retVal = sprintf(string, "Long Attribute (%lld)", attribute->data.longValue);
			return retVal;

		case JAUS_WM_OBJECT_UNSIGNED_SHORT_DATA:
			retVal = sprintf(string, "Unsigned Short Attribute (%d)", attribute->data.unsignedShortValue);
			return retVal;

		case JAUS_WM_OBJECT_UNSIGNED_INTEGER_DATA:
			retVal = sprintf(string, "Unsigned Integer Attribute (%d)", attribute->data.unsignedIntegerValue);
			return retVal;

		case JAUS_WM_OBJECT_UNSIGNED_LONG_DATA:
			retVal = sprintf(string, "Unsigned Long Attribute (%lld)", attribute->data.unsignedLongValue);
			return retVal;

		case JAUS_WM_OBJECT_FLOAT_DATA:
			retVal = sprintf(string, "Float Attribute (%f)", attribute->data.floatValue);
			return retVal;

		case JAUS_WM_OBJECT_DOUBLE_DATA:
			retVal = sprintf(string, "Double Attribute (%lf)", attribute->data.doubleValue);
			return retVal;

		case JAUS_WM_OBJECT_RGB_DATA:
			retVal = sprintf(string, "RGB Attribute Red(%d) Green(%d) Blue(%d)", attribute->data.rgb.redValue, attribute->data.rgb.greenValue, attribute->data.rgb.blueValue);
			return retVal;

		default:
			return 0;
	}
}

// JausWorldModelFeatureClassAttribute GetValue
JausUnsignedLong featureClassAttributeGetValue(JausWorldModelFeatureClassAttribute attribute)
{
	JausUnsignedLong tempULong = 0;
	
	switch(attribute->dataType)
	{
		case JAUS_WM_OBJECT_BYTE_DATA:
			return (JausUnsignedLong)attribute->data.byteValue;
						
		case JAUS_WM_OBJECT_SHORT_DATA:
			return (JausUnsignedLong)attribute->data.shortValue;

		case JAUS_WM_OBJECT_INTEGER_DATA:
			return (JausUnsignedLong)attribute->data.integerValue;

		case JAUS_WM_OBJECT_LONG_DATA:
			return (JausUnsignedLong)attribute->data.longValue;

		case JAUS_WM_OBJECT_UNSIGNED_SHORT_DATA:
			return (JausUnsignedLong)attribute->data.shortValue;

		case JAUS_WM_OBJECT_UNSIGNED_INTEGER_DATA:
			return (JausUnsignedLong)attribute->data.integerValue;

		case JAUS_WM_OBJECT_UNSIGNED_LONG_DATA:
			return (JausUnsignedLong)attribute->data.unsignedLongValue;

		case JAUS_WM_OBJECT_FLOAT_DATA:
			return (JausUnsignedLong)attribute->data.floatValue;

		case JAUS_WM_OBJECT_DOUBLE_DATA:
			return (JausUnsignedLong)attribute->data.doubleValue;

		case JAUS_WM_OBJECT_RGB_DATA:
			tempULong = attribute->data.rgb.redValue << 16;
			tempULong += attribute->data.rgb.greenValue << 8;
			tempULong += attribute->data.rgb.blueValue;
			return tempULong;

		default:
			return 0;
	}
}
