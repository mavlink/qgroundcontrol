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
// File Name: jausPayloadInterface.c
//
// Written By: Bob Touchton
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file provides the general support functionality ReportPayloadInterfacesMessage
// NOTE WELL: this message will also be used for general purpose information exchange

#include <stdlib.h>
#include <string.h>
#include "jaus.h"

#define CORE_MESSAGE_SUPPORT 0
#define NO_PRESENCE_VECTOR 0

//********************************************************//
//				Report PayloadInterfaces Support
//********************************************************//

// Command Interface Constructor
JausCommandInterface jausCommandInterfaceCreate(void)
{
	JausCommandInterface commandInterface;
	
	commandInterface = (JausCommandInterface) malloc(sizeof(JausCommandInterfaceStruct));
	if(commandInterface)
	{
		commandInterface->commandIdentifier = NULL;		// null-terminated string
		commandInterface->typeCode = 0;					// per TYPE CODE table
		commandInterface->units = 0;					// per UNITS table
		commandInterface->blockingFlag = 0;				// bit-0 = 0 for non-blocking, 1 for non-blocking
		commandInterface->minValue.longValue = 0;					// minimum acceptable value
		commandInterface->defaultValue.longValue = 0;				// default value
		commandInterface->maxValue.longValue = 0;					// maximum acceptable value
		commandInterface->enumerationLength = NO_ENUM; 	// length of NO_ENUM (=0) means no enumeration
		commandInterface->enumeration = NULL;			// enumeration content, comma delimited, NULL terminated
		commandInterface->hmiRecommendation = 0;		// per HMI table
		commandInterface->hmiRecommendedPositionXPixels = 0;	
		commandInterface->hmiRecommendedPositionYPixels = 0;
		commandInterface->hmiRecommendedPositionWidthPixels = 0;
		commandInterface->hmiRecommendedPositionHeightPixels = 0;
		commandInterface->currentValue.longValue = 0;
		return commandInterface;
	}
	else
	{
		return NULL;
	}
}

// Information Interface Constructor
JausInformationInterface jausInformationInterfaceCreate(void)
{
	JausInformationInterface informationInterface;
	
	informationInterface = (JausInformationInterface) malloc(sizeof(JausInformationInterfaceStruct));
	if(informationInterface)
	{
		informationInterface->informationIdentifier = NULL;	// null-terminated string
		informationInterface->commandInterfaceAssociation = 0; // index number of the assoc. command interface, 0 if none
		informationInterface->typeCode = 0;				// per TYPE CODE table
		informationInterface->units = 0;					// per UNITS table
		informationInterface->minValue.longValue = 0;					// minimum acceptable value
		informationInterface->defaultValue.longValue = 0;				// default value
		informationInterface->maxValue.longValue = 0;					// maximum acceptable value
		informationInterface->enumerationLength = NO_ENUM; // length of NO_ENUM (=0) means no enumeration
		informationInterface->enumeration = NULL;				// enumeration content, comma delimited, NULL terminated
		informationInterface->hmiRecommendation = 0;		// per HMI table
		informationInterface->hmiRecommendedPositionXPixels = 0;	
		informationInterface->hmiRecommendedPositionYPixels = 0;
		informationInterface->hmiRecommendedPositionWidthPixels = 0;
		informationInterface->hmiRecommendedPositionHeightPixels = 0;
		informationInterface->currentValue.longValue = 0;
		return informationInterface;
	}
	else
	{
		return NULL;
	}
}

// Command Interface Destructor
void jausCommandInterfaceDestroy(JausCommandInterface commandInterface)
{
	if(commandInterface)
	{
		free(commandInterface);
		commandInterface = NULL;
	}
}

// Information Interface Destructor
void jausInformationInterfaceDestroy(JausInformationInterface informationInterface)
{
	if(informationInterface)
	{
		free(informationInterface);
		informationInterface = NULL;
	}
}

JausCommandInterface jausCommandInterfaceRetrieve(JausPayloadInterface payloadInterface, char* identifier)
{
	JausCommandInterface commandInterface;
	int i = 0;	

	// Loop through all services
    for(i = 0; i < payloadInterface->jausCommandInterfaces->elementCount; i++)
	{
		commandInterface = (JausCommandInterface) payloadInterface->jausCommandInterfaces->elementData[i];
		if(!strcmp(commandInterface->commandIdentifier, identifier))
		{
			return commandInterface;
		}
	}
	return NULL;	
}

JausInformationInterface jausInformationInterfaceRetrieve(JausPayloadInterface payloadInterface, char* identifier)
{
	JausInformationInterface informationInterface;
	int i = 0;	
	
	// Loop through all services
    for(i = 0; i < payloadInterface->jausInformationInterfaces->elementCount; i++)
	{
		informationInterface = (JausInformationInterface)payloadInterface->jausInformationInterfaces->elementData[i];
		if(!strcmp(informationInterface->informationIdentifier, identifier))
		{
			return informationInterface;
		}
	}
	return NULL;	
}

// ************************************************************************************************************************************
//			Report PayloadInterfaces End User Functions
// ************************************************************************************************************************************

// jausPayloadInterface Constructor
JausPayloadInterface jausPayloadInterfaceCreate(void)
{
	JausPayloadInterface payloadInterface;
	
	payloadInterface = (JausPayloadInterface) malloc(sizeof(JausPayloadInterfaceStruct));
	if(payloadInterface)
	{
		payloadInterface->presenceVector = NO_HMI;						// defaults to no HMI elements in PV
		payloadInterface->jausCommandInterfaces = jausArrayCreate();		// Dynamic Array of Input Commands
		payloadInterface->jausInformationInterfaces = jausArrayCreate();	// Dynamic Array of Output Commands
		return payloadInterface;
	}
	else
	{
		return NULL;
	}
}

// jausPayloadInterface Destructor
void jausPayloadInterfaceDestroy(JausPayloadInterface payloadInterface)
{
	if(payloadInterface)
	{
		jausArrayDestroy(payloadInterface->jausCommandInterfaces, (void *)jausCommandInterfaceDestroy);
		jausArrayDestroy(payloadInterface->jausInformationInterfaces, (void *)jausInformationInterfaceDestroy);
		free(payloadInterface);
		payloadInterface = NULL;
	}
}

JausCommandInterface jausAddNewCommandInterface(JausPayloadInterface payloadInterface, char* identifier, JausByte typeCode)
{
	JausCommandInterface commandInterface;
	
	if(jausCommandInterfaceRetrieve(payloadInterface, identifier) )
	{
		////cError("%s:%d: command interface %s already exists\n", __FILE__, __LINE__, identifier);
		return NULL;
	}
	
	if( (commandInterface = jausCommandInterfaceCreate() ) )
	{
		commandInterface->commandIdentifier = identifier;		// null-terminated string
		commandInterface->typeCode = typeCode;					// per TYPE CODE table	
		jausArrayAdd(payloadInterface->jausCommandInterfaces, commandInterface);
		return commandInterface;
	}
	else
	{
		////cError("%s:%d: could not create command interface named %s\n", __FILE__, __LINE__, identifier);
		return NULL;
	}
}

JausInformationInterface jausAddNewInformationInterface(JausPayloadInterface payloadInterface, char* identifier, JausByte typeCode)
{
	JausInformationInterface informationInterface;
	
	if(jausInformationInterfaceRetrieve(payloadInterface, identifier) )
	{
		////cError("%s:%d: information interface %s already exists\n", __FILE__, __LINE__, identifier);
		return NULL;
	}
	
	if( (informationInterface = jausInformationInterfaceCreate() ) )
	{
		informationInterface->informationIdentifier = identifier;	// null-terminated string
		informationInterface->typeCode = typeCode;					// per TYPE CODE table
		jausArrayAdd(payloadInterface->jausInformationInterfaces, informationInterface);	
		return informationInterface;
	}
	else
	{
		////cError("%s:%d: could not create information interface named %s\n", __FILE__, __LINE__, identifier);
		return NULL;
	}
}

// NOTE: this function assumes end user is starting their index at 1, iaw the numbering scheme in the JAUS OPC ICD
char* jausGetCommandInterfaceIdentifierByIndex(JausPayloadInterface payloadInterface, int index, int* successFlag)
{
	JausCommandInterface commandInterface;

	if(payloadInterface->jausCommandInterfaces->elementCount >= index)
	{
		commandInterface = payloadInterface->jausCommandInterfaces->elementData[index - 1];
		return commandInterface->commandIdentifier; 
	}
	else
	{
		////cError("%s:%d: could not find a command interface with an index of %d\n", __FILE__, __LINE__, index);
		if(*successFlag) *successFlag = FAILURE;
		return "0";
	}
}

// NOTE: this function assumes end user is starting their index at 1, iaw the numbering scheme in the JAUS OPC ICD
char* jausGetInformationInterfaceIdentifierByIndex(JausPayloadInterface payloadInterface, int index, int* successFlag)
{
	JausInformationInterface informationInterface;

	if(payloadInterface->jausInformationInterfaces->elementCount >= index)
	{
		informationInterface = (JausInformationInterface) payloadInterface->jausInformationInterfaces->elementData[index - 1];
		return informationInterface->informationIdentifier; 
	}
	else
	{
		////cError("%s:%d: could not find an information interface with an index of %d\n", __FILE__, __LINE__, index);
		if(*successFlag) *successFlag = FAILURE;
		return "0";
	}
}

JausByte jausGetCommandInterfaceTypeCode(JausPayloadInterface payloadInterface, char* identifier, int* successFlag)
{
	JausCommandInterface commandInterface;

	if( (commandInterface = jausCommandInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		return commandInterface->typeCode;
	}
	else
	{
		////cError("%s:%d: could not find command interface named %s\n", __FILE__, __LINE__, identifier);
		if(*successFlag) *successFlag = FAILURE;
		return 0;
	}
}

JausByte jausGetInformationInterfaceTypeCode(JausPayloadInterface payloadInterface, char* identifier, int* successFlag)
{
	JausInformationInterface informationInterface;

	if( (informationInterface = jausInformationInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		return informationInterface->typeCode;
	}
	else
	{
		////cError("%s:%d: could not find information interface named %s\n", __FILE__, __LINE__, identifier);
		if(*successFlag) *successFlag = FAILURE;
		return 0;
	}
}

JausBoolean jausSetCommandInterfaceUnits(JausPayloadInterface payloadInterface, char* identifier, JausByte units)
{
	JausCommandInterface commandInterface;

	if( (commandInterface = jausCommandInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		commandInterface->units = units;
		return JAUS_TRUE;
	}
	else
	{
		////cError("%s:%d: could not find command interface named %s\n", __FILE__, __LINE__, identifier);
		return JAUS_FALSE;
	}
}

JausByte jausGetCommandInterfaceUnits(JausPayloadInterface payloadInterface, char* identifier, int* successFlag)
{
	JausCommandInterface commandInterface;

	if( (commandInterface = jausCommandInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		return commandInterface->units;
	}
	else
	{
		////cError("%s:%d: could not find command interface named %s\n", __FILE__, __LINE__, identifier);
		if(*successFlag) *successFlag = FAILURE;
		return 0;
	}
}

JausBoolean jausSetCommandInterfaceBlockingFlag(JausPayloadInterface payloadInterface, char* identifier)
{
	JausCommandInterface commandInterface;

	if( (commandInterface = jausCommandInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		commandInterface->blockingFlag = commandInterface->blockingFlag | 0x01;
		return JAUS_TRUE;
	}
	else
	{
		////cError("%s:%d: could not find command interface named %s\n", __FILE__, __LINE__, identifier);
		return JAUS_FALSE;
	}
}

JausBoolean jausClearCommandInterfaceBlockingFlag(JausPayloadInterface payloadInterface, char* identifier)
{
	JausCommandInterface commandInterface;

	if( (commandInterface = jausCommandInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		commandInterface->blockingFlag = commandInterface->blockingFlag & 0xFE;
		return JAUS_TRUE;
	}
	else
	{
		////cError("%s:%d: could not find command interface named %s\n", __FILE__, __LINE__, identifier);
		return JAUS_FALSE;
	}
}

// returns True if blocking flag is set to "blocking", else returns false
JausBoolean jausGetCommandInterfaceBlockingFlag(JausPayloadInterface payloadInterface, char* identifier, int* successFlag)
{
	JausCommandInterface commandInterface;

	if( (commandInterface = jausCommandInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		if(commandInterface->blockingFlag & 0x01) return JAUS_TRUE;
		else return JAUS_FALSE;
	}
	else
	{
		////cError("%s:%d: could not find command interface named %s\n", __FILE__, __LINE__, identifier);
		if(*successFlag) *successFlag = FAILURE;
		return JAUS_FALSE;
	}
}

JausBoolean jausSetCommandInterfaceMinMax(JausPayloadInterface payloadInterface, char* identifier, JausTypeCode minValue, JausTypeCode maxValue)
{
	JausCommandInterface commandInterface;

	if( (commandInterface = jausCommandInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		commandInterface->minValue = minValue;
		commandInterface->maxValue = maxValue;
		return JAUS_TRUE;
	}
	else
	{
		////cError("%s:%d: could not find command interface named %s\n", __FILE__, __LINE__, identifier);
		return JAUS_FALSE;
	}
}

JausTypeCode jausGetCommandInterfaceMin(JausPayloadInterface payloadInterface, char* identifier, int* successFlag)
{
	JausTypeCode failCode = {0};
	JausCommandInterface commandInterface;

	if( (commandInterface = jausCommandInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		return commandInterface->minValue;
	}
	else
	{
		////cError("%s:%d: could not find command interface named %s\n", __FILE__, __LINE__, identifier);
		if(*successFlag) *successFlag = FAILURE;
		return failCode;
	}
}

JausTypeCode jausGetCommandInterfaceMax(JausPayloadInterface payloadInterface, char* identifier, int* successFlag)
{
	JausTypeCode failCode = {0};
	JausCommandInterface commandInterface;

	if( (commandInterface = jausCommandInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		return commandInterface->maxValue;
	}
	else
	{
		////cError("%s:%d: could not find command interface named %s\n", __FILE__, __LINE__, identifier);
		if(*successFlag) *successFlag = FAILURE;
		return failCode;
	}
}

JausBoolean jausSetCommandInterfaceDefault(JausPayloadInterface payloadInterface, char* identifier, JausTypeCode defaultValue)
{
	JausCommandInterface commandInterface;

	if( (commandInterface = jausCommandInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		commandInterface->defaultValue = defaultValue;
		return JAUS_TRUE;
	}
	else
	{
		////cError("%s:%d: could not find command interface named %s\n", __FILE__, __LINE__, identifier);
		return JAUS_FALSE;
	}
}

JausTypeCode jausGetCommandInterfaceDefault(JausPayloadInterface payloadInterface, char* identifier, int* successFlag)
{
	JausTypeCode failCode = {0};
	JausCommandInterface commandInterface;

	if( (commandInterface = jausCommandInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		return commandInterface->defaultValue;
	}
	else
	{
		////cError("%s:%d: could not find command interface named %s\n", __FILE__, __LINE__, identifier);
		if(*successFlag) *successFlag = FAILURE;
		return failCode;
	}
}

JausBoolean jausSetCommandInterfaceEnumeration(JausPayloadInterface payloadInterface, char* identifier, char* enumeration)
{
	JausCommandInterface commandInterface;

	if( (commandInterface = jausCommandInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		commandInterface->enumeration = enumeration; // no validation on enum formatting
		commandInterface->enumerationLength = (int)strlen(enumeration) + 1; // include the NULL character
		return JAUS_TRUE;
	}
	else
	{
		////cError("%s:%d: could not find command interface named %s\n", __FILE__, __LINE__, identifier);
		return JAUS_FALSE;
	}
}

char* jausGetCommandInterfaceEnumeration(JausPayloadInterface payloadInterface, char* identifier, int* successFlag)
{
	JausCommandInterface commandInterface;

	if( (commandInterface = jausCommandInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		return commandInterface->enumeration; // no validation on enum formatting
	}
	else
	{
		////cError("%s:%d: could not find command interface named %s\n", __FILE__, __LINE__, identifier);
		if(*successFlag) *successFlag = FAILURE;
		return "0";
	}
}

JausUnsignedShort jausGetCommandInterfaceEnumerationLength(JausPayloadInterface payloadInterface, char* identifier, int* successFlag)
{
	JausCommandInterface commandInterface;

	if( (commandInterface = jausCommandInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		return commandInterface->enumerationLength; // includes the NULL character
	}
	else
	{
		////cError("%s:%d: could not find command interface named %s\n", __FILE__, __LINE__, identifier);
		if(*successFlag) *successFlag = FAILURE;
		return 0;
	}
}

JausBoolean jausSetCommandInterfaceHmiRecommendation(JausPayloadInterface payloadInterface, char* identifier, JausByte hmiRecommendation)
{
	JausCommandInterface commandInterface;

	if( (commandInterface = jausCommandInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		commandInterface->hmiRecommendation = hmiRecommendation;
		jausByteSetBit(&payloadInterface->presenceVector, JAUS_PAYLOAD_INTERFACE_PV_HMI_RECOMMENDATION_BIT);//update PV
		return JAUS_TRUE;
	}
	else
	{
		////cError("%s:%d: could not find command interface named %s\n", __FILE__, __LINE__, identifier);
		return JAUS_FALSE;
	}
}

JausByte jausGetCommandInterfaceHmiRecommendation(JausPayloadInterface payloadInterface, char* identifier, int* successFlag)
{
	JausCommandInterface commandInterface;

	if( (commandInterface = jausCommandInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		return commandInterface->hmiRecommendation;
	}
	else
	{
		////cError("%s:%d: could not find command interface named %s\n", __FILE__, __LINE__, identifier);
		if(*successFlag) *successFlag = FAILURE;
		return 0;
	}
}

// NOTE: recommended HMI positioning must be set as a group
JausBoolean jausSetCommandInterfaceHmiParameters(JausPayloadInterface payloadInterface, char* identifier, JausUnsignedShort xPixels, JausUnsignedShort yPixels, JausUnsignedShort widthPixels, JausUnsignedShort heightPixels)
{
	JausCommandInterface commandInterface;

	if( (commandInterface = jausCommandInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		commandInterface->hmiRecommendedPositionXPixels = xPixels;
		commandInterface->hmiRecommendedPositionYPixels = yPixels;
		commandInterface->hmiRecommendedPositionWidthPixels = widthPixels;
		commandInterface->hmiRecommendedPositionHeightPixels = heightPixels;
		//update PV
		jausByteSetBit(&payloadInterface->presenceVector, JAUS_PAYLOAD_INTERFACE_PV_HMI_XPOS_BIT);
		jausByteSetBit(&payloadInterface->presenceVector, JAUS_PAYLOAD_INTERFACE_PV_HMI_YPOS_BIT);
		jausByteSetBit(&payloadInterface->presenceVector, JAUS_PAYLOAD_INTERFACE_PV_HMI_WIDTH_BIT);
		jausByteSetBit(&payloadInterface->presenceVector, JAUS_PAYLOAD_INTERFACE_PV_HMI_HEIGHT_BIT);
		return JAUS_TRUE;
	}
	else
	{
		////cError("%s:%d: could not find command interface named %s\n", __FILE__, __LINE__, identifier);
		return JAUS_FALSE;
	}
}

JausUnsignedShort jausGetCommandInterfaceHmiXPositionPixels(JausPayloadInterface payloadInterface, char* identifier, int* successFlag)
{
	JausCommandInterface commandInterface;

	if( (commandInterface = jausCommandInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		return commandInterface->hmiRecommendedPositionXPixels;
	}
	else
	{
		////cError("%s:%d: could not find command interface named %s\n", __FILE__, __LINE__, identifier);
		if(*successFlag) *successFlag = FAILURE;
		return 0;
	}
}

JausUnsignedShort jausGetCommandInterfaceHmiYPositionPixels(JausPayloadInterface payloadInterface, char* identifier, int* successFlag)
{
	JausCommandInterface commandInterface;

	if( (commandInterface = jausCommandInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		return commandInterface->hmiRecommendedPositionYPixels;
	}
	else
	{
		////cError("%s:%d: could not find command interface named %s\n", __FILE__, __LINE__, identifier);
		if(*successFlag) *successFlag = FAILURE;
		return 0;
	}
}

JausUnsignedShort jausGetCommandInterfaceHmiWidthPixels(JausPayloadInterface payloadInterface, char* identifier, int* successFlag)
{
	JausCommandInterface commandInterface;

	if( (commandInterface = jausCommandInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		return commandInterface->hmiRecommendedPositionWidthPixels;
	}
	else
	{
		////cError("%s:%d: could not find command interface named %s\n", __FILE__, __LINE__, identifier);
		if(*successFlag) *successFlag = FAILURE;
		return 0;
	}
}

JausUnsignedShort jausGetCommandInterfaceHmiHeightPixels(JausPayloadInterface payloadInterface, char* identifier, int* successFlag)
{
	JausCommandInterface commandInterface;

	if( (commandInterface = jausCommandInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		return commandInterface->hmiRecommendedPositionHeightPixels;
	}
	else
	{
		////cError("%s:%d: could not find command interface named %s\n", __FILE__, __LINE__, identifier);
		if(*successFlag) *successFlag = FAILURE;
		return 0;
	}
}

JausBoolean jausSetInformationInterfaceUnits(JausPayloadInterface payloadInterface, char* identifier, JausByte units)
{
	JausInformationInterface informationInterface;

	if( (informationInterface = jausInformationInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		informationInterface->units = units;
		return JAUS_TRUE;
	}
	else
	{
		////cError("%s:%d: could not find information interface named %s\n", __FILE__, __LINE__, identifier);
		return JAUS_FALSE;
	}
}

JausByte jausGetInformationInterfaceUnits(JausPayloadInterface payloadInterface, char* identifier, int* successFlag)
{
	JausInformationInterface informationInterface;

	if( (informationInterface = jausInformationInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		return informationInterface->units;
	}
	else
	{
		////cError("%s:%d: could not find information interface named %s\n", __FILE__, __LINE__, identifier);
		if(*successFlag) *successFlag = FAILURE;
		return 0;
	}
}

JausBoolean jausSetInformationInterfaceCommandInterfaceAssoc(JausPayloadInterface payloadInterface, char* identifier, JausByte commandInterfaceAssociation)
{
	JausInformationInterface informationInterface;

	if( (informationInterface = jausInformationInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		informationInterface->commandInterfaceAssociation = commandInterfaceAssociation; // no validation
		return JAUS_TRUE;
	}
	else
	{
		////cError("%s:%d: could not find information interface named %s\n", __FILE__, __LINE__, identifier);
		return JAUS_FALSE;
	}
}

JausByte jausGetInformationInterfaceCommandInterfaceAssoc(JausPayloadInterface payloadInterface, char* identifier, int* successFlag)
{
	JausInformationInterface informationInterface;

	if( (informationInterface = jausInformationInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		return informationInterface->commandInterfaceAssociation; // no validation
	}
	else
	{
		////cError("%s:%d: could not find information interface named %s\n", __FILE__, __LINE__, identifier);
		if(*successFlag) *successFlag = FAILURE;
		return 0;
	}
}

JausBoolean jausSetInformationInterfaceMinMax(JausPayloadInterface payloadInterface, char* identifier, JausTypeCode minValue, JausTypeCode maxValue)
{
	JausInformationInterface informationInterface;

	if( (informationInterface = jausInformationInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		informationInterface->minValue = minValue;
		informationInterface->maxValue = maxValue;
		return JAUS_TRUE;
	}
	else
	{
		////cError("%s:%d: could not find information interface named %s\n", __FILE__, __LINE__, identifier);
		return JAUS_FALSE;
	}
}

JausTypeCode jausGetInformationInterfaceMin(JausPayloadInterface payloadInterface, char* identifier, int* successFlag)
{
	JausTypeCode failCode = {0};
	JausInformationInterface informationInterface;

	if( (informationInterface = jausInformationInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		return informationInterface->minValue;
	}
	else
	{
		////cError("%s:%d: could not find information interface named %s\n", __FILE__, __LINE__, identifier);
		if(*successFlag) *successFlag = FAILURE;
		return failCode;
	}
}

JausTypeCode jausGetInformationInterfaceMax(JausPayloadInterface payloadInterface, char* identifier, int* successFlag)
{
	JausTypeCode failCode = {0};
	JausInformationInterface informationInterface;

	if( (informationInterface = jausInformationInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		return informationInterface->maxValue;
	}
	else
	{
		////cError("%s:%d: could not find information interface named %s\n", __FILE__, __LINE__, identifier);
		if(*successFlag) *successFlag = FAILURE;
		return failCode;
	}
}

JausBoolean jausSetInformationInterfaceDefault(JausPayloadInterface payloadInterface, char* identifier, JausTypeCode defaultValue)
{
	JausInformationInterface informationInterface;

	if( (informationInterface = jausInformationInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		informationInterface->defaultValue = defaultValue;
		return JAUS_TRUE;
	}
	else
	{
		////cError("%s:%d: could not find information interface named %s\n", __FILE__, __LINE__, identifier);
		return JAUS_FALSE;
	}
}

JausTypeCode jausGetInformationInterfaceDefault(JausPayloadInterface payloadInterface, char* identifier, int* successFlag)
{
	JausTypeCode failCode = {0};
	JausInformationInterface informationInterface;

	if( (informationInterface = jausInformationInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		return informationInterface->defaultValue;
	}
	else
	{
		////cError("%s:%d: could not find information interface named %s\n", __FILE__, __LINE__, identifier);
		if(*successFlag) *successFlag = FAILURE;
		return failCode;
	}
}

JausBoolean jausSetInformationInterfaceEnumeration(JausPayloadInterface payloadInterface, char* identifier, char* enumeration)
{
	JausInformationInterface informationInterface;

	if( (informationInterface = jausInformationInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		informationInterface->enumeration = enumeration; // no validation on enum formatting
		informationInterface->enumerationLength = (int)strlen(enumeration) + 1; // include the NULL character
		return JAUS_TRUE;
	}
	else
	{
		////cError("%s:%d: could not find information interface named %s\n", __FILE__, __LINE__, identifier);
		return JAUS_FALSE;
	}
}

char* jausGetInformationInterfaceEnumeration(JausPayloadInterface payloadInterface, char* identifier, int* successFlag)
{
	JausInformationInterface informationInterface;

	if( (informationInterface = jausInformationInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		return informationInterface->enumeration; // no validation on enum formatting
	}
	else
	{
		////cError("%s:%d: could not find information interface named %s\n", __FILE__, __LINE__, identifier);
		if(*successFlag) *successFlag = FAILURE;
		return "0";
	}
}

JausUnsignedShort jausGetInformationInterfaceEnumerationLength(JausPayloadInterface payloadInterface, char* identifier, int* successFlag)
{
	JausInformationInterface informationInterface;

	if( (informationInterface = jausInformationInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		return informationInterface->enumerationLength; // includes the NULL character
	}
	else
	{
		////cError("%s:%d: could not find information interface named %s\n", __FILE__, __LINE__, identifier);
		if(*successFlag) *successFlag = FAILURE;
		return 0;
	}
}

JausBoolean jausSetInformationInterfaceHmiRecommendation(JausPayloadInterface payloadInterface, char* identifier, JausByte hmiRecommendation)
{
	JausInformationInterface informationInterface;

	if( (informationInterface = jausInformationInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		informationInterface->hmiRecommendation = hmiRecommendation;
		jausByteSetBit(&payloadInterface->presenceVector, JAUS_PAYLOAD_INTERFACE_PV_HMI_RECOMMENDATION_BIT);//update PV
		return JAUS_TRUE;
	}
	else
	{
		////cError("%s:%d: could not find information interface named %s\n", __FILE__, __LINE__, identifier);
		return JAUS_FALSE;
	}
}

JausByte jausGetInformationInterfaceHmiRecommendation(JausPayloadInterface payloadInterface, char* identifier, int* successFlag)
{
	JausInformationInterface informationInterface;

	if( (informationInterface = jausInformationInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		return informationInterface->hmiRecommendation;
	}
	else
	{
		////cError("%s:%d: could not find information interface named %s\n", __FILE__, __LINE__, identifier);
		if(*successFlag) *successFlag = FAILURE;
		return 0;
	}
}

// NOTE: recommended HMI positioning must be set as a group
JausBoolean jausSetInformationInterfaceHmiParameters(JausPayloadInterface payloadInterface, char* identifier, JausUnsignedShort xPixels, JausUnsignedShort yPixels, JausUnsignedShort widthPixels, JausUnsignedShort heightPixels)
{
	JausInformationInterface informationInterface;

	if( (informationInterface = jausInformationInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		informationInterface->hmiRecommendedPositionXPixels = xPixels;
		informationInterface->hmiRecommendedPositionYPixels = yPixels;
		informationInterface->hmiRecommendedPositionWidthPixels = widthPixels;
		informationInterface->hmiRecommendedPositionHeightPixels = heightPixels;
		//update PV
		jausByteSetBit(&payloadInterface->presenceVector, JAUS_PAYLOAD_INTERFACE_PV_HMI_XPOS_BIT);
		jausByteSetBit(&payloadInterface->presenceVector, JAUS_PAYLOAD_INTERFACE_PV_HMI_YPOS_BIT);
		jausByteSetBit(&payloadInterface->presenceVector, JAUS_PAYLOAD_INTERFACE_PV_HMI_WIDTH_BIT);
		jausByteSetBit(&payloadInterface->presenceVector, JAUS_PAYLOAD_INTERFACE_PV_HMI_HEIGHT_BIT);
		return JAUS_TRUE;
	}
	else
	{
		////cError("%s:%d: could not find information interface named %s\n", __FILE__, __LINE__, identifier);
		return JAUS_FALSE;
	}
}

JausUnsignedShort jausGetInformationInterfaceHmiXPositionPixels(JausPayloadInterface payloadInterface, char* identifier, int* successFlag)
{
	JausInformationInterface informationInterface;

	if( (informationInterface = jausInformationInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		return informationInterface->hmiRecommendedPositionXPixels;
	}
	else
	{
		////cError("%s:%d: could not find information interface named %s\n", __FILE__, __LINE__, identifier);
		if(*successFlag) *successFlag = FAILURE;
		return 0;
	}
}

JausUnsignedShort jausGetInformationInterfaceHmiYPositionPixels(JausPayloadInterface payloadInterface, char* identifier, int* successFlag)
{
	JausInformationInterface informationInterface;

	if( (informationInterface = jausInformationInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		return informationInterface->hmiRecommendedPositionYPixels;
	}
	else
	{
		////cError("%s:%d: could not find information interface named %s\n", __FILE__, __LINE__, identifier);
		if(*successFlag) *successFlag = FAILURE;
		return 0;
	}
}

JausUnsignedShort jausGetInformationInterfaceHmiWidthPixels(JausPayloadInterface payloadInterface, char* identifier, int* successFlag)
{
	JausInformationInterface informationInterface;

	if( (informationInterface = jausInformationInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		return informationInterface->hmiRecommendedPositionWidthPixels;
	}
	else
	{
		////cError("%s:%d: could not find information interface named %s\n", __FILE__, __LINE__, identifier);
		if(*successFlag) *successFlag = FAILURE;
		return 0;
	}
}

JausUnsignedShort jausGetInformationInterfaceHmiHeightPixels(JausPayloadInterface payloadInterface, char* identifier, int* successFlag)
{
	JausInformationInterface informationInterface;

	if( (informationInterface = jausInformationInterfaceRetrieve(payloadInterface, identifier) ) )
	{
		return informationInterface->hmiRecommendedPositionHeightPixels;
	}
	else
	{
		////cError("%s:%d: could not find information interface named %s\n", __FILE__, __LINE__, identifier);
		if(*successFlag) *successFlag = FAILURE;
		return 0;
	}
}

// decodes the Type Code value and returns the number of bytes required for Min, Max, and Default fields
int jausMinMaxDefaultSizeBytes(JausByte typeCodeEnum)
{
	switch((int) typeCodeEnum)
	{
		case TYPE_CODE_SHORT:
			return JAUS_SHORT_SIZE_BYTES;
			break;
			
		case TYPE_CODE_INTEGER:
			return JAUS_INTEGER_SIZE_BYTES;
			break;
			
		case TYPE_CODE_LONG:
			return JAUS_LONG_SIZE_BYTES;
			break;
			
		case TYPE_CODE_BYTE:
			return JAUS_BYTE_SIZE_BYTES;
			break;

		case TYPE_CODE_U_SHORT:
			return JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			break;
		
		case TYPE_CODE_U_INTEGER:
			return JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
			break;
		
		case TYPE_CODE_U_LONG:
			return JAUS_UNSIGNED_LONG_SIZE_BYTES;
			break;
		
		case TYPE_CODE_FLOAT:
			return JAUS_FLOAT_SIZE_BYTES;
			break;
		
		case TYPE_CODE_DOUBLE:
			return JAUS_DOUBLE_SIZE_BYTES;
			break;
		
		case TYPE_CODE_SCALED_U_BYTE:	// min, max, defaut uses float for scaled fields
			return JAUS_FLOAT_SIZE_BYTES;
			break;

		case TYPE_CODE_SCALED_SHORT:	// min, max, defaut uses float for scaled fields
			return JAUS_FLOAT_SIZE_BYTES;
			break;
		
		case TYPE_CODE_SCALED_U_SHORT:	// min, max, defaut uses float for scaled fields
			return JAUS_FLOAT_SIZE_BYTES;
			break;
		
		case TYPE_CODE_SCALED_INTEGER:	// min, max, defaut uses float for scaled fields
			return JAUS_FLOAT_SIZE_BYTES;
			break;
		
		case TYPE_CODE_SCALED_U_INTEGER:	// min, max, defaut uses float for scaled fields
			return JAUS_FLOAT_SIZE_BYTES;
			break;
			
		case TYPE_CODE_SCALED_LONG:	// min, max, defaut uses float for scaled fields
			return JAUS_FLOAT_SIZE_BYTES;
			break;
			
		case TYPE_CODE_SCALED_U_LONG:	// min, max, defaut uses float for scaled fields
			return JAUS_FLOAT_SIZE_BYTES;
			break;
			
		case TYPE_CODE_ENUM:
			return JAUS_BYTE_SIZE_BYTES;
			break;
		
		case TYPE_CODE_BOOLEAN:
			return JAUS_BYTE_SIZE_BYTES;// booleans are treated as a byte
			break;
			
		case TYPE_CODE_STRING:	// min, max, default uses a byte for a string field
			return JAUS_BYTE_SIZE_BYTES;
			break;
			
		case TYPE_CODE_U_BYTE_TUPLE:
			return 2 * JAUS_BYTE_SIZE_BYTES;
			break;
			
		case TYPE_CODE_U_SHORT_TUPLE:
			return 2 * JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			break;
			
		case TYPE_CODE_U_INTEGER_TUPLE:
			return 2 * JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
			break;
			
		default:
			return 0;	
			break;
	}
}

// determines how to pack fields that have a JausTypeCode data type
int jausMinMaxDefaultToBuffer(JausTypeCode field, unsigned char* buffer, unsigned int bufferSizeBytes, JausByte typeCodeEnum)
{
	switch((int) typeCodeEnum)
	{
		case TYPE_CODE_SHORT:
			if(jausShortToBuffer(field.shortValue, buffer, bufferSizeBytes) ) return JAUS_SHORT_SIZE_BYTES;
			else return 0;
			break;
			
		case TYPE_CODE_INTEGER:
			if(jausIntegerToBuffer(field.integerValue, buffer, bufferSizeBytes) ) return JAUS_INTEGER_SIZE_BYTES;
			else return 0;
			break;
			
		case TYPE_CODE_LONG:
			if(jausLongToBuffer(field.longValue, buffer, bufferSizeBytes) ) return JAUS_LONG_SIZE_BYTES;
			else return 0;
			break;
			
		case TYPE_CODE_BYTE:
			if(jausByteToBuffer(field.byteValue, buffer, bufferSizeBytes) ) return JAUS_BYTE_SIZE_BYTES;
			else return 0;
			break;

		case TYPE_CODE_U_SHORT:
			if(jausUnsignedShortToBuffer(field.uShortValue, buffer, bufferSizeBytes) ) return JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			else return 0;
			break;
		
		case TYPE_CODE_U_INTEGER:
			if(jausUnsignedIntegerToBuffer(field.uIntegerValue, buffer, bufferSizeBytes) ) return JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
			else return 0;
			break;
		
		case TYPE_CODE_U_LONG:
			if(jausUnsignedLongToBuffer(field.uLongValue, buffer, bufferSizeBytes) ) return JAUS_UNSIGNED_LONG_SIZE_BYTES;
			else return 0;
			break;
		
		case TYPE_CODE_FLOAT:
			if(jausFloatToBuffer(field.floatValue, buffer, bufferSizeBytes) ) return JAUS_FLOAT_SIZE_BYTES;
			else return 0;
			break;
		
		case TYPE_CODE_DOUBLE:
			if(jausDoubleToBuffer(field.longFloatValue, buffer, bufferSizeBytes) ) return JAUS_DOUBLE_SIZE_BYTES;
			else return 0;
			break;
		
		case TYPE_CODE_SCALED_U_BYTE:	// min, max, defaut uses float for scaled fields
			if(jausFloatToBuffer(field.floatValue, buffer, bufferSizeBytes) ) return JAUS_FLOAT_SIZE_BYTES;
			else return 0;
			break;

		case TYPE_CODE_SCALED_SHORT:	// min, max, defaut uses float for scaled fields
			if(jausFloatToBuffer(field.floatValue, buffer, bufferSizeBytes) ) return JAUS_FLOAT_SIZE_BYTES;
			else return 0;
			break;
		
		case TYPE_CODE_SCALED_U_SHORT:	// min, max, defaut uses float for scaled fields
			if(jausFloatToBuffer(field.floatValue, buffer, bufferSizeBytes) ) return JAUS_FLOAT_SIZE_BYTES;
			else return 0;
			break;
		
		case TYPE_CODE_SCALED_INTEGER:	// min, max, defaut uses float for scaled fields
			if(jausFloatToBuffer(field.floatValue, buffer, bufferSizeBytes) ) return JAUS_FLOAT_SIZE_BYTES;
			else return 0;
			break;
		
		case TYPE_CODE_SCALED_U_INTEGER:	// min, max, defaut uses float for scaled fields
			if(jausFloatToBuffer(field.floatValue, buffer, bufferSizeBytes) ) return JAUS_FLOAT_SIZE_BYTES;
			else return 0;
			break;
			
		case TYPE_CODE_SCALED_LONG:	// min, max, defaut uses float for scaled fields
			if(jausFloatToBuffer(field.floatValue, buffer, bufferSizeBytes) ) return JAUS_FLOAT_SIZE_BYTES;
			else return 0;
			break;
			
		case TYPE_CODE_SCALED_U_LONG:	// min, max, defaut uses float for scaled fields
			if(jausFloatToBuffer(field.floatValue, buffer, bufferSizeBytes) ) return JAUS_FLOAT_SIZE_BYTES;
			else return 0;
			break;
			
		case TYPE_CODE_ENUM:
			if(jausUnsignedShortToBuffer(field.uShortValue, buffer, bufferSizeBytes) ) return JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			else return 0;
			break;
		
		case TYPE_CODE_BOOLEAN:
			if(jausByteToBuffer(field.booleanValue, buffer, bufferSizeBytes) ) return JAUS_BYTE_SIZE_BYTES;
			else return 0;
			break;
			
		case TYPE_CODE_STRING:	// min, max, default uses a byte for a string field
			if(jausByteToBuffer(field.byteValue, buffer, bufferSizeBytes) ) return JAUS_BYTE_SIZE_BYTES;
			else return 0;
			break;
			
		case TYPE_CODE_U_BYTE_TUPLE:
			if(!jausByteToBuffer(field.byteTuple.one, buffer, bufferSizeBytes)) return 0;
			buffer += JAUS_BYTE_SIZE_BYTES;
			if(jausByteToBuffer(field.byteTuple.two, buffer, bufferSizeBytes) ) return 2 * JAUS_BYTE_SIZE_BYTES;
			else return 0;
			break;
			
		case TYPE_CODE_U_SHORT_TUPLE:
			if(!jausUnsignedShortToBuffer(field.shortTuple.one, buffer, bufferSizeBytes)) return 0;
			buffer += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			if(jausUnsignedShortToBuffer(field.shortTuple.two, buffer, bufferSizeBytes) ) return 2 * JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			else return 0;
			break;
			
		case TYPE_CODE_U_INTEGER_TUPLE:
			if(!jausUnsignedIntegerToBuffer(field.integerTuple.one, buffer, bufferSizeBytes)) return 0;
			buffer += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
			if(jausUnsignedIntegerToBuffer(field.integerTuple.two, buffer, bufferSizeBytes) ) return 2 * JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
			else return 0;
			break;
			
		default:
			return 0;	
			break;
	}
	
}

// determines how to unpack fields that have a JausTypeCode data type
JausBoolean jausMinMaxDefaultFromBuffer(JausTypeCode *field, unsigned char* buffer, unsigned int bufferSizeBytes, JausByte typeCodeEnum)
{
	JausByte tempByte;
	switch((int) typeCodeEnum)
	{
		case TYPE_CODE_SHORT:
			if(jausShortFromBuffer(&field->shortValue, buffer, bufferSizeBytes) )
			{

				return JAUS_TRUE;
			}
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_INTEGER:
			if(jausIntegerFromBuffer(&field->integerValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_LONG:
			if(jausLongFromBuffer(&field->longValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_BYTE:
			if(jausByteFromBuffer(&field->byteValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;

		case TYPE_CODE_U_SHORT:
			if(jausUnsignedShortFromBuffer(&field->uShortValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
		
		case TYPE_CODE_U_INTEGER:
			if(jausUnsignedIntegerFromBuffer(&field->uIntegerValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
		
		case TYPE_CODE_U_LONG:
			if(jausUnsignedLongFromBuffer(&field->uLongValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
		
		case TYPE_CODE_FLOAT:
			if(jausFloatFromBuffer(&field->floatValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
		
		case TYPE_CODE_DOUBLE:
			if(jausDoubleFromBuffer(&field->longFloatValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
		
		case TYPE_CODE_SCALED_U_BYTE:	// min, max, defaut uses float for scaled fields
			if(jausFloatFromBuffer(&field->floatValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;

		case TYPE_CODE_SCALED_SHORT:	// min, max, defaut uses float for scaled fields
			if(jausFloatFromBuffer(&field->floatValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
		
		case TYPE_CODE_SCALED_U_SHORT:	// min, max, defaut uses float for scaled fields
			if(jausFloatFromBuffer(&field->floatValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
		
		case TYPE_CODE_SCALED_INTEGER:	// min, max, defaut uses float for scaled fields
			if(jausFloatFromBuffer(&field->floatValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
		
		case TYPE_CODE_SCALED_U_INTEGER:	// min, max, defaut uses float for scaled fields
			if(jausFloatFromBuffer(&field->floatValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_SCALED_LONG:	// min, max, defaut uses float for scaled fields
			if(jausFloatFromBuffer(&field->floatValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_SCALED_U_LONG:	// min, max, defaut uses float for scaled fields
			if(jausFloatFromBuffer(&field->floatValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_ENUM:
			if(jausUnsignedShortFromBuffer(&field->uShortValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
		
		case TYPE_CODE_BOOLEAN:
			if(jausByteFromBuffer(&tempByte, buffer, bufferSizeBytes) ) 
			{
				if(tempByte) 
				{
					field->booleanValue = JAUS_TRUE;
				}
				else 
				{
					field->booleanValue = JAUS_FALSE;
				}
				return JAUS_TRUE;
			}
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_STRING:	// min, max, default uses a byte for a string field
			if(jausByteFromBuffer(&field->byteValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_U_BYTE_TUPLE:
			if(!jausByteFromBuffer(&field->byteTuple.one, buffer, bufferSizeBytes)) return JAUS_FALSE;
			buffer += JAUS_BYTE_SIZE_BYTES;
			if(jausByteFromBuffer(&field->byteTuple.two, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_U_SHORT_TUPLE:
			if(!jausUnsignedShortFromBuffer(&field->shortTuple.one, buffer, bufferSizeBytes)) return JAUS_FALSE;
			buffer += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			if(jausUnsignedShortFromBuffer(&field->shortTuple.two, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_U_INTEGER_TUPLE:
			if(!jausUnsignedIntegerFromBuffer(&field->integerTuple.one, buffer, bufferSizeBytes)) return JAUS_FALSE;
			buffer += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
			if(jausUnsignedIntegerFromBuffer(&field->integerTuple.two, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
			
		default:
			return JAUS_FALSE;	
			break;
	}
	
}

// determines how to pack current value field based on JausTypeCode; returns bytes packed 
int jausInformationValueToBuffer(JausInformationInterface informationInterface, unsigned char* buffer, unsigned int bufferSizeBytes)
{
	JausByte tempByte;
	JausUnsignedShort tempUShort;
	JausShort tempShort;
	JausUnsignedInteger tempUInt;
	JausInteger tempInt;
	JausUnsignedLong tempULong;
	JausLong tempLong;
	switch((int) informationInterface->typeCode)
	{
		case TYPE_CODE_SHORT:
			if(jausShortToBuffer(informationInterface->currentValue.shortValue, buffer, bufferSizeBytes) ) return JAUS_SHORT_SIZE_BYTES;
			else return 0;
			break;
			
		case TYPE_CODE_INTEGER:
			if(jausIntegerToBuffer(informationInterface->currentValue.integerValue, buffer, bufferSizeBytes) ) return JAUS_INTEGER_SIZE_BYTES;
			else return 0;
			break;
			
		case TYPE_CODE_LONG:
			if(jausLongToBuffer(informationInterface->currentValue.longValue, buffer, bufferSizeBytes) ) return JAUS_LONG_SIZE_BYTES;
			else return 0;
			break;
			
		case TYPE_CODE_BYTE:
			if(jausByteToBuffer(informationInterface->currentValue.byteValue, buffer, bufferSizeBytes) ) return JAUS_BYTE_SIZE_BYTES;
			else return 0;
			break;

		case TYPE_CODE_U_SHORT:
			if(jausUnsignedShortToBuffer(informationInterface->currentValue.uShortValue, buffer, bufferSizeBytes) ) return JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			else return 0;
			break;
		
		case TYPE_CODE_U_INTEGER:
			if(jausUnsignedIntegerToBuffer(informationInterface->currentValue.uIntegerValue, buffer, bufferSizeBytes) ) return JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
			else return 0;
			break;
		
		case TYPE_CODE_U_LONG:
			if(jausUnsignedLongToBuffer(informationInterface->currentValue.uLongValue, buffer, bufferSizeBytes) ) return JAUS_UNSIGNED_LONG_SIZE_BYTES;
			else return 0;
			break;
		
		case TYPE_CODE_FLOAT:
			if(jausFloatToBuffer(informationInterface->currentValue.floatValue, buffer, bufferSizeBytes) ) return JAUS_FLOAT_SIZE_BYTES;
			else return 0;
			break;
		
		case TYPE_CODE_DOUBLE:
			if(jausDoubleToBuffer(informationInterface->currentValue.longFloatValue, buffer, bufferSizeBytes) ) return JAUS_DOUBLE_SIZE_BYTES;
			else return 0;
			break;
		
		case TYPE_CODE_SCALED_U_BYTE:	
			tempByte = jausByteFromDouble(informationInterface->currentValue.floatValue, informationInterface->minValue.floatValue, informationInterface->maxValue.floatValue);
			if(jausByteToBuffer(tempByte, buffer, bufferSizeBytes) ) return JAUS_BYTE_SIZE_BYTES;
			else return 0;
			break;

		case TYPE_CODE_SCALED_SHORT:
			tempShort = jausShortFromDouble(informationInterface->currentValue.floatValue, informationInterface->minValue.floatValue, informationInterface->maxValue.floatValue);
			if(jausShortToBuffer(tempShort, buffer, bufferSizeBytes) ) return JAUS_SHORT_SIZE_BYTES;
			else return 0;
			break;
		
		case TYPE_CODE_SCALED_U_SHORT:
			tempUShort = jausUnsignedShortFromDouble(informationInterface->currentValue.floatValue, informationInterface->minValue.floatValue, informationInterface->maxValue.floatValue);
			if(jausUnsignedShortToBuffer(tempUShort, buffer, bufferSizeBytes) ) return JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			else return 0;
			break;
		
		case TYPE_CODE_SCALED_INTEGER:
			tempInt = jausIntegerFromDouble(informationInterface->currentValue.floatValue, informationInterface->minValue.floatValue, informationInterface->maxValue.floatValue);
			if(jausIntegerToBuffer(tempInt, buffer, bufferSizeBytes) ) return JAUS_INTEGER_SIZE_BYTES;
			else return 0;
			break;
		
		case TYPE_CODE_SCALED_U_INTEGER:
			tempUInt = jausUnsignedIntegerFromDouble(informationInterface->currentValue.floatValue, informationInterface->minValue.floatValue, informationInterface->maxValue.floatValue);
			if(jausUnsignedIntegerToBuffer(tempUInt, buffer, bufferSizeBytes) ) return JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
			else return 0;
			break;
			
		case TYPE_CODE_SCALED_LONG:
			tempLong = jausLongFromDouble(informationInterface->currentValue.floatValue, informationInterface->minValue.floatValue, informationInterface->maxValue.floatValue);
			if(jausLongToBuffer(tempLong, buffer, bufferSizeBytes) ) return JAUS_LONG_SIZE_BYTES;
			else return 0;
			break;
			
		case TYPE_CODE_SCALED_U_LONG:
			tempULong = jausUnsignedIntegerFromDouble(informationInterface->currentValue.floatValue, informationInterface->minValue.floatValue, informationInterface->maxValue.floatValue);
			if(jausUnsignedLongToBuffer(tempULong, buffer, bufferSizeBytes) ) return JAUS_UNSIGNED_LONG_SIZE_BYTES;
			else return 0;
			break;
			
		case TYPE_CODE_ENUM:
			if(jausUnsignedShortToBuffer(informationInterface->currentValue.uShortValue, buffer, bufferSizeBytes) ) return JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			else return 0;
			break;
		
		case TYPE_CODE_BOOLEAN:
			if(informationInterface->currentValue.booleanValue == JAUS_TRUE)
			{
				if(jausByteToBuffer(0x01, buffer, bufferSizeBytes) ) return JAUS_BYTE_SIZE_BYTES;				
			}
			else if(jausByteToBuffer(0x00, buffer, bufferSizeBytes) )
			{
				return JAUS_BYTE_SIZE_BYTES;	
			}
			else return 0;
			break;

		case TYPE_CODE_STRING:
			if(jausUnsignedShortToBuffer(informationInterface->currentValue.string.length, buffer, bufferSizeBytes) )
			{
				memcpy(buffer, informationInterface->currentValue.string.value, informationInterface->currentValue.string.length);
				return JAUS_UNSIGNED_SHORT_SIZE_BYTES + informationInterface->currentValue.string.length;
			}
			else return 0;
			break;
			
		case TYPE_CODE_U_BYTE_TUPLE:
			if(!jausByteToBuffer(informationInterface->currentValue.byteTuple.one, buffer, bufferSizeBytes)) return 0;
			buffer += JAUS_BYTE_SIZE_BYTES;
			if(jausByteToBuffer(informationInterface->currentValue.byteTuple.two, buffer, bufferSizeBytes) ) return 2 * JAUS_BYTE_SIZE_BYTES;
			else return 0;
			break;
			
		case TYPE_CODE_U_SHORT_TUPLE:
			if(!jausUnsignedShortToBuffer(informationInterface->currentValue.shortTuple.one, buffer, bufferSizeBytes)) return 0;
			buffer += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			if(jausUnsignedShortToBuffer(informationInterface->currentValue.shortTuple.two, buffer, bufferSizeBytes) ) return 2 * JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			else return 0;
			break;
			
		case TYPE_CODE_U_INTEGER_TUPLE:
			if(!jausUnsignedIntegerToBuffer(informationInterface->currentValue.integerTuple.one, buffer, bufferSizeBytes)) return 0;
			buffer += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
			if(jausUnsignedIntegerToBuffer(informationInterface->currentValue.integerTuple.two, buffer, bufferSizeBytes) ) return 2 * JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
			else return 0;
			break;
			
		default:
			return 0;	
			break;
	}
	return 0;	
}

// determines how to pack current value field based on JausTypeCode; returns bytes packed 
int jausCommandValueToBuffer(JausCommandInterface commandInterface, unsigned char* buffer, unsigned int bufferSizeBytes)
{
	JausByte tempByte;
	JausUnsignedShort tempUShort;
	JausShort tempShort;
	JausUnsignedInteger tempUInt;
	JausInteger tempInt;
	JausUnsignedLong tempULong;
	JausLong tempLong;
	switch((int) commandInterface->typeCode)
	{
		case TYPE_CODE_SHORT:
			if(jausShortToBuffer(commandInterface->currentValue.shortValue, buffer, bufferSizeBytes) ) return JAUS_SHORT_SIZE_BYTES;
			else return 0;
			break;
			
		case TYPE_CODE_INTEGER:
			if(jausIntegerToBuffer(commandInterface->currentValue.integerValue, buffer, bufferSizeBytes) ) return JAUS_INTEGER_SIZE_BYTES;
			else return 0;
			break;
			
		case TYPE_CODE_LONG:
			if(jausLongToBuffer(commandInterface->currentValue.longValue, buffer, bufferSizeBytes) ) return JAUS_LONG_SIZE_BYTES;
			else return 0;
			break;
			
		case TYPE_CODE_BYTE:
			if(jausByteToBuffer(commandInterface->currentValue.byteValue, buffer, bufferSizeBytes) ) return JAUS_BYTE_SIZE_BYTES;
			else return 0;
			break;

		case TYPE_CODE_U_SHORT:
			if(jausUnsignedShortToBuffer(commandInterface->currentValue.uShortValue, buffer, bufferSizeBytes) ) return JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			else return 0;
			break;
		
		case TYPE_CODE_U_INTEGER:
			if(jausUnsignedIntegerToBuffer(commandInterface->currentValue.uIntegerValue, buffer, bufferSizeBytes) ) return JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
			else return 0;
			break;
		
		case TYPE_CODE_U_LONG:
			if(jausUnsignedLongToBuffer(commandInterface->currentValue.uLongValue, buffer, bufferSizeBytes) ) return JAUS_UNSIGNED_LONG_SIZE_BYTES;
			else return 0;
			break;
		
		case TYPE_CODE_FLOAT:
			if(jausFloatToBuffer(commandInterface->currentValue.floatValue, buffer, bufferSizeBytes) ) return JAUS_FLOAT_SIZE_BYTES;
			else return 0;
			break;
		
		case TYPE_CODE_DOUBLE:
			if(jausDoubleToBuffer(commandInterface->currentValue.longFloatValue, buffer, bufferSizeBytes) ) return JAUS_DOUBLE_SIZE_BYTES;
			else return 0;
			break;
		
		case TYPE_CODE_SCALED_U_BYTE:	
			tempByte = jausByteFromDouble(commandInterface->currentValue.floatValue, commandInterface->minValue.floatValue, commandInterface->maxValue.floatValue);
			if(jausByteToBuffer(tempByte, buffer, bufferSizeBytes) ) return JAUS_BYTE_SIZE_BYTES;
			else return 0;
			break;

		case TYPE_CODE_SCALED_SHORT:
			tempShort = jausShortFromDouble(commandInterface->currentValue.floatValue, commandInterface->minValue.floatValue, commandInterface->maxValue.floatValue);
			if(jausShortToBuffer(tempShort, buffer, bufferSizeBytes) ) return JAUS_SHORT_SIZE_BYTES;
			else return 0;
			break;
		
		case TYPE_CODE_SCALED_U_SHORT:
			tempUShort = jausUnsignedShortFromDouble(commandInterface->currentValue.floatValue, commandInterface->minValue.floatValue, commandInterface->maxValue.floatValue);
			if(jausUnsignedShortToBuffer(tempUShort, buffer, bufferSizeBytes) ) return JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			else return 0;
			break;
		
		case TYPE_CODE_SCALED_INTEGER:
			tempInt = jausIntegerFromDouble(commandInterface->currentValue.floatValue, commandInterface->minValue.floatValue, commandInterface->maxValue.floatValue);
			if(jausIntegerToBuffer(tempInt, buffer, bufferSizeBytes) ) return JAUS_INTEGER_SIZE_BYTES;
			else return 0;
			break;
		
		case TYPE_CODE_SCALED_U_INTEGER:
			tempUInt = jausUnsignedIntegerFromDouble(commandInterface->currentValue.floatValue, commandInterface->minValue.floatValue, commandInterface->maxValue.floatValue);
			if(jausUnsignedIntegerToBuffer(tempUInt, buffer, bufferSizeBytes) ) return JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
			else return 0;
			break;
			
		case TYPE_CODE_SCALED_LONG:
			tempLong = jausLongFromDouble(commandInterface->currentValue.floatValue, commandInterface->minValue.floatValue, commandInterface->maxValue.floatValue);
			if(jausLongToBuffer(tempLong, buffer, bufferSizeBytes) ) return JAUS_LONG_SIZE_BYTES;
			else return 0;
			break;
			
		case TYPE_CODE_SCALED_U_LONG:
			tempULong = jausUnsignedIntegerFromDouble(commandInterface->currentValue.floatValue, commandInterface->minValue.floatValue, commandInterface->maxValue.floatValue);
			if(jausUnsignedLongToBuffer(tempULong, buffer, bufferSizeBytes) ) return JAUS_UNSIGNED_LONG_SIZE_BYTES;
			else return 0;
			break;
			
		case TYPE_CODE_ENUM:
			if(jausUnsignedShortToBuffer(commandInterface->currentValue.uShortValue, buffer, bufferSizeBytes) ) return JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			else return 0;
			break;
		
		case TYPE_CODE_BOOLEAN:
			if(commandInterface->currentValue.booleanValue == JAUS_TRUE)
			{
				if(jausByteToBuffer(0x01, buffer, bufferSizeBytes) ) return JAUS_BYTE_SIZE_BYTES;				
			}
			else if(jausByteToBuffer(0x00, buffer, bufferSizeBytes) )
			{
				return JAUS_BYTE_SIZE_BYTES;	
			}
			else return 0;
			break;

		case TYPE_CODE_STRING:
			if(jausUnsignedShortToBuffer(commandInterface->currentValue.string.length, buffer, bufferSizeBytes) )
			{
				memcpy(buffer, commandInterface->currentValue.string.value, commandInterface->currentValue.string.length);
				return JAUS_UNSIGNED_SHORT_SIZE_BYTES + commandInterface->currentValue.string.length;
			}
			else return 0;
			break;
			
		case TYPE_CODE_U_BYTE_TUPLE:
			if(!jausByteToBuffer(commandInterface->currentValue.byteTuple.one, buffer, bufferSizeBytes)) return 0;
			buffer += JAUS_BYTE_SIZE_BYTES;
			if(jausByteToBuffer(commandInterface->currentValue.byteTuple.two, buffer, bufferSizeBytes) ) return 2 * JAUS_BYTE_SIZE_BYTES;
			else return 0;
			break;
			
		case TYPE_CODE_U_SHORT_TUPLE:
			if(!jausUnsignedShortToBuffer(commandInterface->currentValue.shortTuple.one, buffer, bufferSizeBytes)) return 0;
			buffer += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			if(jausUnsignedShortToBuffer(commandInterface->currentValue.shortTuple.two, buffer, bufferSizeBytes) ) return 2 * JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			else return 0;
			break;
			
		case TYPE_CODE_U_INTEGER_TUPLE:
			if(!jausUnsignedIntegerToBuffer(commandInterface->currentValue.integerTuple.one, buffer, bufferSizeBytes)) return 0;
			buffer += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
			if(jausUnsignedIntegerToBuffer(commandInterface->currentValue.integerTuple.two, buffer, bufferSizeBytes) ) return 2 * JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
			else return 0;
			break;
			
		default:
			return 0;	
			break;
	}
	return 0;	
}

// determines how to unpack current value field based on JausTypeCode 
JausBoolean jausInformationValueFromBuffer(JausInformationInterface informationInterface, unsigned char* buffer, unsigned int bufferSizeBytes)
{
	JausByte tempByte;
	JausUnsignedShort tempUShort;
	JausShort tempShort;
	JausUnsignedInteger tempUInt;
	JausInteger tempInt;
	JausUnsignedLong tempULong;
	JausLong tempLong;
	switch((int) informationInterface->typeCode)
	{
		case TYPE_CODE_SHORT:
			if(jausShortFromBuffer(&informationInterface->currentValue.shortValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_INTEGER:
			if(jausIntegerFromBuffer(&informationInterface->currentValue.integerValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_LONG:
			if(jausLongFromBuffer(&informationInterface->currentValue.longValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_BYTE:
			if(jausByteFromBuffer(&informationInterface->currentValue.byteValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;

		case TYPE_CODE_U_SHORT:
			if(jausUnsignedShortFromBuffer(&informationInterface->currentValue.uShortValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
		
		case TYPE_CODE_U_INTEGER:
			if(jausUnsignedIntegerFromBuffer(&informationInterface->currentValue.uIntegerValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
		
		case TYPE_CODE_U_LONG:
			if(jausUnsignedLongFromBuffer(&informationInterface->currentValue.uLongValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
		
		case TYPE_CODE_FLOAT:
			if(jausFloatFromBuffer(&informationInterface->currentValue.floatValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
		
		case TYPE_CODE_DOUBLE:
			if(jausDoubleFromBuffer(&informationInterface->currentValue.longFloatValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
		
		case TYPE_CODE_SCALED_U_BYTE:	
			if(jausByteFromBuffer(&tempByte, buffer, bufferSizeBytes) ) 
			{
				informationInterface->currentValue.floatValue = (float) jausByteToDouble(tempByte, informationInterface->minValue.floatValue, informationInterface->maxValue.floatValue);
				return JAUS_TRUE;
			}	
			else return JAUS_FALSE;
			break;

		case TYPE_CODE_SCALED_SHORT:	
			if(jausShortFromBuffer(&tempShort, buffer, bufferSizeBytes) ) 
			{
				informationInterface->currentValue.floatValue = (float) jausShortToDouble(tempShort, informationInterface->minValue.floatValue, informationInterface->maxValue.floatValue);
				return JAUS_TRUE;
			}
			else return JAUS_FALSE;
			break;
		
		case TYPE_CODE_SCALED_U_SHORT:	
			if(jausUnsignedShortFromBuffer(&tempUShort, buffer, bufferSizeBytes) ) 
			{
				informationInterface->currentValue.floatValue = (float) jausUnsignedShortToDouble(tempUShort, informationInterface->minValue.floatValue, informationInterface->maxValue.floatValue);
				return JAUS_TRUE;
			}
			else return JAUS_FALSE;
			break;
		
		case TYPE_CODE_SCALED_INTEGER:	
			if(jausIntegerFromBuffer(&tempInt, buffer, bufferSizeBytes) )
			{
				informationInterface->currentValue.floatValue = (float) jausIntegerToDouble(tempInt, informationInterface->minValue.floatValue, informationInterface->maxValue.floatValue);
				return JAUS_TRUE;
			}
			else return JAUS_FALSE;
			break;
		
		case TYPE_CODE_SCALED_U_INTEGER:	
			if(jausUnsignedIntegerFromBuffer(&tempUInt, buffer, bufferSizeBytes) )
			{
				informationInterface->currentValue.floatValue = (float) jausUnsignedIntegerToDouble(tempUInt, informationInterface->minValue.floatValue, informationInterface->maxValue.floatValue);
				return JAUS_TRUE;
			}
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_SCALED_LONG:	
			if(jausLongFromBuffer(&tempLong, buffer, bufferSizeBytes) )
			{
				informationInterface->currentValue.floatValue = (float) jausLongToDouble(tempLong, informationInterface->minValue.floatValue, informationInterface->maxValue.floatValue);
				return JAUS_TRUE;
			}
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_SCALED_U_LONG:
			if(jausUnsignedLongFromBuffer(&tempULong, buffer, bufferSizeBytes) )
			{
				informationInterface->currentValue.floatValue = (float) jausUnsignedLongToDouble(tempULong, informationInterface->minValue.floatValue, informationInterface->maxValue.floatValue);
				return JAUS_TRUE;
			}
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_ENUM:
			if(jausByteFromBuffer(&informationInterface->currentValue.enumValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
		
		case TYPE_CODE_BOOLEAN:
			if(jausByteFromBuffer(&tempByte, buffer, bufferSizeBytes) ) 
			{
				if(tempByte) 
				{
					informationInterface->currentValue.booleanValue = JAUS_TRUE;
				}
				else 
				{
					informationInterface->currentValue.booleanValue = JAUS_FALSE;
				}
				return JAUS_TRUE;
			}
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_STRING:	
			if(jausUnsignedShortFromBuffer(&informationInterface->currentValue.string.length, buffer, bufferSizeBytes) )
			{ 
				if(informationInterface->currentValue.string.length)
				{
					memcpy(informationInterface->currentValue.string.value, buffer, informationInterface->currentValue.string.length);
					return JAUS_TRUE;
				}
				else return JAUS_FALSE;
			}
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_U_BYTE_TUPLE:
			if(!jausByteFromBuffer(&informationInterface->currentValue.byteTuple.one, buffer, bufferSizeBytes)) return JAUS_FALSE;
			buffer += JAUS_BYTE_SIZE_BYTES;
			if(jausByteFromBuffer(&informationInterface->currentValue.byteTuple.two, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_U_SHORT_TUPLE:
			if(!jausUnsignedShortFromBuffer(&informationInterface->currentValue.shortTuple.one, buffer, bufferSizeBytes)) return JAUS_FALSE;
			buffer += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			if(jausUnsignedShortFromBuffer(&informationInterface->currentValue.shortTuple.two, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_U_INTEGER_TUPLE:
			if(!jausUnsignedIntegerFromBuffer(&informationInterface->currentValue.integerTuple.one, buffer, bufferSizeBytes)) return JAUS_FALSE;
			buffer += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
			if(jausUnsignedIntegerFromBuffer(&informationInterface->currentValue.integerTuple.two, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
			
		default:
			return JAUS_FALSE;	
			break;
	}
	
}

// determines how to unpack current value field based on JausTypeCode 
JausBoolean jausCommandValueFromBuffer(JausCommandInterface commandInterface, unsigned char* buffer, unsigned int bufferSizeBytes)
{
	JausByte tempByte;
	JausUnsignedShort tempUShort;
	JausShort tempShort;
	JausUnsignedInteger tempUInt;
	JausInteger tempInt;
	JausUnsignedLong tempULong;
	JausLong tempLong;
	switch((int) commandInterface->typeCode)
	{
		case TYPE_CODE_SHORT:
			if(jausShortFromBuffer(&commandInterface->currentValue.shortValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_INTEGER:
			if(jausIntegerFromBuffer(&commandInterface->currentValue.integerValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_LONG:
			if(jausLongFromBuffer(&commandInterface->currentValue.longValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_BYTE:
			if(jausByteFromBuffer(&commandInterface->currentValue.byteValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;

		case TYPE_CODE_U_SHORT:
			if(jausUnsignedShortFromBuffer(&commandInterface->currentValue.uShortValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
		
		case TYPE_CODE_U_INTEGER:
			if(jausUnsignedIntegerFromBuffer(&commandInterface->currentValue.uIntegerValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
		
		case TYPE_CODE_U_LONG:
			if(jausUnsignedLongFromBuffer(&commandInterface->currentValue.uLongValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
		
		case TYPE_CODE_FLOAT:
			if(jausFloatFromBuffer(&commandInterface->currentValue.floatValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
		
		case TYPE_CODE_DOUBLE:
			if(jausDoubleFromBuffer(&commandInterface->currentValue.longFloatValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
		
		case TYPE_CODE_SCALED_U_BYTE:	
			if(jausByteFromBuffer(&tempByte, buffer, bufferSizeBytes) ) 
			{
				commandInterface->currentValue.floatValue = (float) jausByteToDouble(tempByte, commandInterface->minValue.floatValue, commandInterface->maxValue.floatValue);
				return JAUS_TRUE;
			}	
			else return JAUS_FALSE;
			break;

		case TYPE_CODE_SCALED_SHORT:	
			if(jausShortFromBuffer(&tempShort, buffer, bufferSizeBytes) ) 
			{
				commandInterface->currentValue.floatValue = (float) jausShortToDouble(tempShort, commandInterface->minValue.floatValue, commandInterface->maxValue.floatValue);
				return JAUS_TRUE;
			}
			else return JAUS_FALSE;
			break;
		
		case TYPE_CODE_SCALED_U_SHORT:	
			if(jausUnsignedShortFromBuffer(&tempUShort, buffer, bufferSizeBytes) ) 
			{
				commandInterface->currentValue.floatValue = (float) jausUnsignedShortToDouble(tempUShort, commandInterface->minValue.floatValue, commandInterface->maxValue.floatValue);
				return JAUS_TRUE;
			}
			else return JAUS_FALSE;
			break;
		
		case TYPE_CODE_SCALED_INTEGER:	
			if(jausIntegerFromBuffer(&tempInt, buffer, bufferSizeBytes) )
			{
				commandInterface->currentValue.floatValue = (float) jausIntegerToDouble(tempInt, commandInterface->minValue.floatValue, commandInterface->maxValue.floatValue);
				return JAUS_TRUE;
			}
			else return JAUS_FALSE;
			break;
		
		case TYPE_CODE_SCALED_U_INTEGER:	
			if(jausUnsignedIntegerFromBuffer(&tempUInt, buffer, bufferSizeBytes) )
			{
				commandInterface->currentValue.floatValue = (float) jausUnsignedIntegerToDouble(tempUInt, commandInterface->minValue.floatValue, commandInterface->maxValue.floatValue);
				return JAUS_TRUE;
			}
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_SCALED_LONG:	
			if(jausLongFromBuffer(&tempLong, buffer, bufferSizeBytes) )
			{
				commandInterface->currentValue.floatValue = (float) jausLongToDouble(tempLong, commandInterface->minValue.floatValue, commandInterface->maxValue.floatValue);
				return JAUS_TRUE;
			}
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_SCALED_U_LONG:
			if(jausUnsignedLongFromBuffer(&tempULong, buffer, bufferSizeBytes) )
			{
				commandInterface->currentValue.floatValue = (float) jausUnsignedLongToDouble(tempULong, commandInterface->minValue.floatValue, commandInterface->maxValue.floatValue);
				return JAUS_TRUE;
			}
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_ENUM:
			if(jausByteFromBuffer(&commandInterface->currentValue.enumValue, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
		
		case TYPE_CODE_BOOLEAN:
			if(jausByteFromBuffer(&tempByte, buffer, bufferSizeBytes) ) 
			{
				if(tempByte) 
				{
					commandInterface->currentValue.booleanValue = JAUS_TRUE;
				}
				else 
				{
					commandInterface->currentValue.booleanValue = JAUS_FALSE;
				}
				return JAUS_TRUE;
			}
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_STRING:	
			if(jausUnsignedShortFromBuffer(&commandInterface->currentValue.string.length, buffer, bufferSizeBytes) )
			{ 
				if(commandInterface->currentValue.string.length)
				{
					memcpy(commandInterface->currentValue.string.value, buffer, commandInterface->currentValue.string.length);
					return JAUS_TRUE;
				}
				else return JAUS_FALSE;
			}
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_U_BYTE_TUPLE:
			if(!jausByteFromBuffer(&commandInterface->currentValue.byteTuple.one, buffer, bufferSizeBytes)) return JAUS_FALSE;
			buffer += JAUS_BYTE_SIZE_BYTES;
			if(jausByteFromBuffer(&commandInterface->currentValue.byteTuple.two, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_U_SHORT_TUPLE:
			if(!jausUnsignedShortFromBuffer(&commandInterface->currentValue.shortTuple.one, buffer, bufferSizeBytes)) return JAUS_FALSE;
			buffer += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			if(jausUnsignedShortFromBuffer(&commandInterface->currentValue.shortTuple.two, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
			
		case TYPE_CODE_U_INTEGER_TUPLE:
			if(!jausUnsignedIntegerFromBuffer(&commandInterface->currentValue.integerTuple.one, buffer, bufferSizeBytes)) return JAUS_FALSE;
			buffer += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
			if(jausUnsignedIntegerFromBuffer(&commandInterface->currentValue.integerTuple.two, buffer, bufferSizeBytes) ) return JAUS_TRUE;
			else return JAUS_FALSE;
			break;
			
		default:
			return JAUS_FALSE;	
			break;
	}
	
}

// decodes the Type Code value and returns the number of bytes required for currentValue field
int jausInformationValueSizeBytes(JausInformationInterface informationInterface)
{
	switch((int) informationInterface->typeCode)
	{
		case TYPE_CODE_SHORT:
			return JAUS_SHORT_SIZE_BYTES;
			break;
			
		case TYPE_CODE_INTEGER:
			return JAUS_INTEGER_SIZE_BYTES;
			break;
			
		case TYPE_CODE_LONG:
			return JAUS_LONG_SIZE_BYTES;
			break;
			
		case TYPE_CODE_BYTE:
			return JAUS_BYTE_SIZE_BYTES;
			break;

		case TYPE_CODE_U_SHORT:
			return JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			break;
		
		case TYPE_CODE_U_INTEGER:
			return JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
			break;
		
		case TYPE_CODE_U_LONG:
			return JAUS_UNSIGNED_LONG_SIZE_BYTES;
			break;
		
		case TYPE_CODE_FLOAT:
			return JAUS_FLOAT_SIZE_BYTES;
			break;
		
		case TYPE_CODE_DOUBLE:
			return JAUS_DOUBLE_SIZE_BYTES;
			break;
		
		case TYPE_CODE_SCALED_U_BYTE:	
			return JAUS_BYTE_SIZE_BYTES;
			break;

		case TYPE_CODE_SCALED_SHORT:	
			return JAUS_SHORT_SIZE_BYTES;
			break;
		
		case TYPE_CODE_SCALED_U_SHORT:	
			return JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			break;
		
		case TYPE_CODE_SCALED_INTEGER:
			return JAUS_INTEGER_SIZE_BYTES;
			break;
		
		case TYPE_CODE_SCALED_U_INTEGER:
			return JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
			break;
			
		case TYPE_CODE_SCALED_LONG:
			return JAUS_LONG_SIZE_BYTES;
			break;
			
		case TYPE_CODE_SCALED_U_LONG:
			return JAUS_UNSIGNED_LONG_SIZE_BYTES;
			break;
			
		case TYPE_CODE_ENUM:
			return JAUS_BYTE_SIZE_BYTES * 2; //enum is defined as 2 bytes 
			break;
		
		case TYPE_CODE_BOOLEAN:
			return JAUS_BYTE_SIZE_BYTES;// booleans are treated as a byte
			break;
			
		case TYPE_CODE_STRING:	// strings use an unsigned short followed by a null terminated string
			return JAUS_UNSIGNED_SHORT_SIZE_BYTES + informationInterface->currentValue.string.length;
			break;
			
		case TYPE_CODE_U_BYTE_TUPLE:
			return 2 * JAUS_BYTE_SIZE_BYTES;
			break;
			
		case TYPE_CODE_U_SHORT_TUPLE:
			return 2 * JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			break;
			
		case TYPE_CODE_U_INTEGER_TUPLE:
			return 2 * JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
			break;
			
		default:
			return 0;	
			break;
	}
}

// decodes the Type Code value and returns the number of bytes required for currentValue field
int jausCommandValueSizeBytes(JausCommandInterface commandInterface)
{
	switch((int) commandInterface->typeCode)
	{
		case TYPE_CODE_SHORT:
			return JAUS_SHORT_SIZE_BYTES;
			break;
			
		case TYPE_CODE_INTEGER:
			return JAUS_INTEGER_SIZE_BYTES;
			break;
			
		case TYPE_CODE_LONG:
			return JAUS_LONG_SIZE_BYTES;
			break;
			
		case TYPE_CODE_BYTE:
			return JAUS_BYTE_SIZE_BYTES;
			break;

		case TYPE_CODE_U_SHORT:
			return JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			break;
		
		case TYPE_CODE_U_INTEGER:
			return JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
			break;
		
		case TYPE_CODE_U_LONG:
			return JAUS_UNSIGNED_LONG_SIZE_BYTES;
			break;
		
		case TYPE_CODE_FLOAT:
			return JAUS_FLOAT_SIZE_BYTES;
			break;
		
		case TYPE_CODE_DOUBLE:
			return JAUS_DOUBLE_SIZE_BYTES;
			break;
		
		case TYPE_CODE_SCALED_U_BYTE:	
			return JAUS_BYTE_SIZE_BYTES;
			break;

		case TYPE_CODE_SCALED_SHORT:	
			return JAUS_SHORT_SIZE_BYTES;
			break;
		
		case TYPE_CODE_SCALED_U_SHORT:	
			return JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			break;
		
		case TYPE_CODE_SCALED_INTEGER:
			return JAUS_INTEGER_SIZE_BYTES;
			break;
		
		case TYPE_CODE_SCALED_U_INTEGER:
			return JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
			break;
			
		case TYPE_CODE_SCALED_LONG:
			return JAUS_LONG_SIZE_BYTES;
			break;
			
		case TYPE_CODE_SCALED_U_LONG:
			return JAUS_UNSIGNED_LONG_SIZE_BYTES;
			break;
			
		case TYPE_CODE_ENUM:
			return JAUS_BYTE_SIZE_BYTES * 2; //enum is defined as 2 bytes 
			break;
		
		case TYPE_CODE_BOOLEAN:
			return JAUS_BYTE_SIZE_BYTES;// booleans are treated as a byte
			break;
			
		case TYPE_CODE_STRING:	// strings use an unsigned short followed by a null terminated string
			return JAUS_UNSIGNED_SHORT_SIZE_BYTES + commandInterface->currentValue.string.length;
			break;
			
		case TYPE_CODE_U_BYTE_TUPLE:
			return 2 * JAUS_BYTE_SIZE_BYTES;
			break;
			
		case TYPE_CODE_U_SHORT_TUPLE:
			return 2 * JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			break;
			
		case TYPE_CODE_U_INTEGER_TUPLE:
			return 2 * JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
			break;
			
		default:
			return 0;	
			break;
	}
}

