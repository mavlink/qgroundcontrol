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
// File Name: jausAddress.c
//
// Written By: Danny Kent (jaus AT dannykent DOT com), Tom Galluzzo (galluzzo AT gmail DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file defines the basic functions for use with the JausAddress of each component

#include <stdio.h>
#include <stdlib.h>
#include "jaus.h"

JausAddress jausAddressCreate(void)
{
	JausAddress address;
	
	address = (JausAddress)malloc( sizeof(struct JausAddressStruct) );
	if(address == NULL)
	{
		return NULL;
	}
	
	// Init Values
	address->instance = 0;
	address->component = 0;
	address->node = 0;
	address->subsystem = 0;
	
	address->next = NULL;
	
	return address;
}

void jausAddressDestroy(JausAddress address)
{
	free(address);
}

JausAddress jausAddressClone(JausAddress input)
{
	JausAddress address;
	
	address = (JausAddress)malloc( sizeof(struct JausAddressStruct) );
	if(address == NULL)
	{
		return NULL;
	}

	address->instance = input->instance;
	address->component = input->component;
	address->node = input->node;
	address->subsystem = input->subsystem;
	
	address->next = input->next;

	return address;
}

JausBoolean jausAddressCopy(JausAddress dst, JausAddress src)
{
	if(dst && src)
	{
		dst->subsystem = src->subsystem;
		dst->node = src->node;
		dst->component = src->component;
		dst->instance = src->instance;
		return JAUS_TRUE;
	}
	else
	{
		return JAUS_FALSE;
	}
}

JausBoolean jausAddressEqual(JausAddress first, JausAddress second)
{
	return (JausBoolean) (	first->subsystem == second->subsystem &&
							first->node == second->node &&
							first->component == second->component &&
							first->instance == second->instance );
}


int jausAddressToString(JausAddress address, char *buf)
{
	if(address)
	{
		return sprintf(buf, "%d.%d.%d.%d", address->subsystem, address->node, address->component, address->instance);
	}
	else
	{
		return sprintf(buf, "Invalid JAUS Address");
	}
}


JausBoolean jausAddressIsValid(JausAddress address)
{
	return (address &&
			address->subsystem	!= JAUS_INVALID_SUBSYSTEM_ID &&
			address->node		!= JAUS_INVALID_NODE_ID &&
			address->component	!= JAUS_INVALID_COMPONENT_ID &&
			address->instance	!= JAUS_INVALID_INSTANCE_ID);
}

int jausAddressHash(JausAddress address)
{
	int hash = (address->subsystem << 24) | (address->node << 16) | (address->component << 8) | (address->instance);
	return hash;
}

