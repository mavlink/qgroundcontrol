/**************************************************************************
 * Copyright (C) 2015 Eff'Innov Technologies 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * Developped by Eff'Innov Technologies : contact@effinnov.com
 * 
 **************************************************************************/

#include "TML/inc/framework_Allocator.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct sMemInfo
{
	uint32_t magic;
	size_t size;
} sMemInfo_t;

typedef struct sMemInfoEnd
{
	uint32_t magicEnd;
} sMemInfoEnd_t;


void* framework_AllocMem(size_t size)
{
	sMemInfo_t *info = NULL;
	sMemInfoEnd_t *infoEnd = NULL;
	uint8_t * pMem = (uint8_t *) malloc(size+sizeof(sMemInfo_t)+sizeof(sMemInfoEnd_t));
	info = (sMemInfo_t*)pMem;
	
	info->magic = 0xDEADC0DE;
	info->size  = size;

	pMem = pMem+sizeof(sMemInfo_t);

	memset(pMem,0xAB,size);

	infoEnd = (sMemInfoEnd_t*)(pMem+size);

	infoEnd->magicEnd = 0xDEADC0DE;
	return pMem;
}

void framework_FreeMem(void *ptr)
{
	if(NULL !=  ptr)
	{
		sMemInfoEnd_t *infoEnd = NULL;
		uint8_t *memInfo = (uint8_t*)ptr;
		sMemInfo_t *info = (sMemInfo_t*)(memInfo - sizeof(sMemInfo_t));

		infoEnd = (sMemInfoEnd_t*)(memInfo+info->size);

		if ((info->magic != 0xDEADC0DE)||(infoEnd->magicEnd != 0xDEADC0DE))
		{
			// Call Debugger
			*(int *)(uintptr_t)0xbbadbeef = 0;
		}else
		{
			memset(info,0x14,info->size+sizeof(sMemInfo_t)+sizeof(sMemInfoEnd_t));
		}
			
		free(info);
	}
}

