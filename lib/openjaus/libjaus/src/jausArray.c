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
// File Name: jausArray.c
//
// Written By: Tom Galluzzo (galluzzo AT gmail DOT com) and Danny Kent (jaus AT dannykent DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file defines the attributes of a JausArray object

#include <stdio.h>
#include <stdlib.h>
#include "jaus.h"

int jausArrayIncrement(JausArray);

JausArray jausArrayCreate(void)
{
	JausArray jausArray;
	
	jausArray = (JausArray)malloc( sizeof(JausArrayStruct) );
	if(jausArray == NULL)
	{
		return NULL;
	}
	
	jausArray->capacity = 8;
	jausArray->capacityIncrement = 8;
	
	jausArray->elementCount = 0;
	jausArray->elementData = (void **)malloc( sizeof(void *) * jausArray->capacity);
	if(jausArray->elementData == NULL)
	{
		free(jausArray);
		return NULL;
	}
	
	return jausArray;
}

void jausArrayDestroy(JausArray jausArray, void (*elementDestroy)(void *))
{
	int i;
	
	if(jausArray == NULL)
	{
		return;
	}
	
	if(elementDestroy != NULL)
	{
		i = jausArray->elementCount;
		while(i--)
		{
			elementDestroy(jausArray->elementData[i]);
		}
	}

	free(jausArray->elementData);
	free(jausArray);
}

void jausArrayAdd(JausArray jausArray, void *element)
{
	jausArray->elementCount++;
	if(jausArray->elementCount > jausArray->capacity)
	{
		jausArrayIncrement(jausArray);
	}
	
	jausArray->elementData[jausArray->elementCount - 1] = element;
}

int jausArrayIncrement(JausArray jausArray)
{
	int newCapacity = jausArray->capacity + jausArray->capacityIncrement;
	void ** newElementData;

	newElementData = (void **)realloc((void *)jausArray->elementData,  sizeof(void *) * newCapacity);
	
	if(newElementData == NULL)
	{
		return -1;
	}
	else
	{
		jausArray->elementData = newElementData;
		jausArray->capacity = newCapacity;
		return 0;
	}
}

int jausArrayContains(JausArray jausArray, void *testElement, int (*equals)(void *, void *))
{
	int i;

	for(i = 0; i < jausArray->elementCount; i++)
	{
		if(equals(jausArray->elementData[i], testElement))
		{
			return i;
		}
	}
	
	return -1;
}

// Returns the element removed
void *jausArrayRemoveAt(JausArray jausArray, int index)
{
	int i;
//	int newCapacity = jausArray->capacity - jausArray->capacityIncrement;
	void *retValue;
	
	if(jausArray->elementCount <= index)
	{
		return NULL;
	}
	
	retValue = jausArray->elementData[index];
	for(i = index; i < jausArray->elementCount - 1; i++)
	{
		jausArray->elementData[i] = jausArray->elementData[i+1];
	}
	
	// TODO: Check to see if we should shrink capacity
	// jausArray->elementData = (void **)realloc((void *)jausArray->elementData,  sizeof(void *) * newCapacity);
	jausArray->elementCount--;
	// jausArray->capacity = newCapacity;

	return retValue;
}

// returns the element removed
void *jausArrayRemove(JausArray jausArray, void *testElement, int (*equals)(void *, void *))
{
	int i;
	
	for(i = 0; i < jausArray->elementCount; i++)
	{
		if(equals(jausArray->elementData[i], testElement))
		{
			return jausArrayRemoveAt(jausArray, i);
		}
	}
	
	return NULL;
}

void jausArrayRemoveAll(JausArray jausArray, void (*elementDestroy)(void *))
{
	int i;

	if(elementDestroy != NULL)
	{
		i = jausArray->elementCount;
		while(i--)
		{
			elementDestroy(jausArray->elementData[i]);
		}
		jausArray->elementCount = 0;
	}
}
