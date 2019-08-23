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

#include "TML/inc/framework_linux.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// **************************************** Log functions
void framework_Log(const char* format, ...)
{
    char buffer[1024];
	va_list varg;
	va_start(varg, format);
    strcpy(buffer, "LOG     : ");
    vsprintf(&buffer[10], format, varg);
	va_end(varg);
    logmsg(buffer);
}

void framework_Warn(const char* format, ...)
{
    char buffer[1024];
    va_list varg;
	va_start(varg, format);
    strcpy(buffer, "WARNING : ");
    vsprintf(&buffer[10], format, varg);
    va_end(varg);
    logmsg(buffer);
}

void framework_Error(const char* format, ...)
{
    char buffer[1024];
    va_list varg;
	va_start(varg, format);
    strcpy(buffer, "ERROR   : ");
    vsprintf(&buffer[10], format, varg);
    va_end(varg);
    logmsg(buffer);
}

void framework_HexDump(const void * buffer, uint32_t size)
{
	HexDump(buffer,size);
}

// Internal function
void HexDump(const void * buffer, uint32_t size)
{
	char      tmpBuff[128];
	uint8_t  *dataBuffer = (uint8_t*)buffer;
	uint32_t  i         = 0;
	uint32_t  buffIter  = 0;
	uint32_t  tmpI      = 0;
	while (i < size)
	{
		buffIter = 0;
		tmpI     = i;
		while ((tmpI < size) && (buffIter < 24))
		{
			sprintf(tmpBuff + buffIter, "%.2X ", dataBuffer[tmpI]);
			buffIter += 3;
			tmpI++;
		}

		tmpI = i;
		while ((tmpI < size) && (buffIter < 32))
		{
			if (dataBuffer[tmpI] == '%')
			{
				tmpBuff[buffIter++] = '%';
				tmpBuff[buffIter++] = '%';
			}else if (dataBuffer[tmpI] >= 32)
			{
				tmpBuff[buffIter++] = dataBuffer[tmpI];
			}
			else
			{
				tmpBuff[buffIter++] = '.';
			}
			tmpI++;
		}
		i = tmpI;
		tmpBuff[buffIter++] = '\n';
		tmpBuff[buffIter++] = 0;

		framework_Log(tmpBuff);
	}
}
