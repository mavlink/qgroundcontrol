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
// File Name: jausNode.c
//
// Written By: Danny Kent (jaus AT dannykent DOT com), Tom Galluzzo (galluzzo AT gmail DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: 

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "jaus.h"

JausNode jausNodeCreate(void)
{
	JausNode node;
	
	node = (JausNode)malloc( sizeof(JausNodeStruct) );
	if(node == NULL)
	{
		return NULL;
	}
	
	//Init Values
	node->identification = NULL;
	node->id = 0;
	node->components = jausArrayCreate();
	node->subsystem = NULL;

	jausNodeUpdateTimestamp(node);

	return node;
}

void jausNodeDestroy(JausNode node)
{
	jausArrayDestroy(node->components, (void *)jausComponentDestroy);
	if(node->identification) free(node->identification);
	free(node);
}

JausNode jausNodeClone(JausNode node)
{
	JausNode clone;
	JausComponent tempCmpt;
	int i;
	size_t stringLength;
	
	clone = (JausNode)malloc( sizeof(JausNodeStruct) );
	if(clone == NULL)
	{
		return NULL;
	}
	
	//Init Values
	if(node->identification)
	{
		stringLength = strlen(node->identification) + 1;
		clone->identification = (char *) malloc(stringLength);
		sprintf(clone->identification, "%s", node->identification);
	}
	else
	{
		clone->identification = NULL;
	}

	clone->id = node->id;
	clone->components = jausArrayCreate();
	for( i = 0; i < node->components->elementCount; i++)
	{
		tempCmpt = jausComponentClone(node->components->elementData[i]);
		jausArrayAdd(clone->components, tempCmpt);
	}
	
	clone->subsystem = node->subsystem;

	jausNodeUpdateTimestamp(clone);

	return clone;
}

JausBoolean jausNodeFreeMemory(JausNode node)
{
	if(node->components == NULL) return JAUS_FALSE;
	
	free(node->components);
	return JAUS_TRUE;
}

void jausNodeUpdateTimestamp(JausNode node)
{
	time(&node->timeStampSec);  
}

JausBoolean jausNodeIsTimedOut(JausNode node)
{
	time_t now;
	time(&now);

	return difftime(now, node->timeStampSec) > NODE_TIMEOUT_SEC? JAUS_TRUE : JAUS_FALSE;
}

int jausNodeToString(JausNode node, char *buf)
{	
	if(node->identification)
	{
		return sprintf(buf, "%s-%d", node->identification, node->id);
	}
	else
	{
		return sprintf(buf, "JausNode-%d", node->id);
	}
}

JausBoolean jausNodeHasIdentification(JausNode node)
{
	if(node && node->identification)
	{
		return JAUS_TRUE;
	}
	else
	{
		return JAUS_FALSE;
	}
}

JausBoolean jausNodeHasConfiguration(JausNode node)
{
	if(node && node->components->elementCount > 0)
	{
		return JAUS_TRUE;
	}
	else
	{
		return JAUS_FALSE;
	}
}
