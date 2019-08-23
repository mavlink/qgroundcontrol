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

#include "TML/inc/framework_Parcel.h"
#include "TML/inc/framework_Allocator.h"

#include <stdlib.h>
#include <string.h>

#define INC_COUNTER(p,x) \
{ \
uint8_t seeSomewhereElseCompilator = 0; \
do \
{ \
    p->m_curPosition += x; \
    if (p->m_curLength < p->m_curPosition) \
    { \
        p->m_curLength   += x; \
    } \
}while (seeSomewhereElseCompilator); \
}
typedef struct _tParcel
{
    uint8_t  *m_data;
    uint32_t  m_dataLength;
    uint32_t  m_curLength;
    uint32_t  m_curPosition;
}tParcel;

static void growData(tParcel *parcel,uint32_t len)
{
    uint32_t newSize;
    uint8_t *newBuffer;
    
    if ((parcel->m_curPosition+len) < parcel->m_dataLength)
        return;

    newSize = parcel->m_dataLength + len;

    newBuffer = (uint8_t*)framework_AllocMem(newSize);

    memcpy(newBuffer,parcel->m_data, parcel->m_dataLength);

    framework_FreeMem(parcel->m_data);

    parcel->m_data = newBuffer;
    parcel->m_dataLength = newSize;
}

void framework_ParcelCreate(void** parcel)
{
    tParcel *p = NULL;
    
    if (parcel == NULL)
    {
        return;
    }
    
    p                = (tParcel*)framework_AllocMem(sizeof(tParcel));
    p->m_data        = (uint8_t*)framework_AllocMem(32);
    p->m_dataLength  = 32;
    p->m_curPosition = 0;
    p->m_curLength   = 0;

    memset(p->m_data, 0, 32);
    
    *parcel = p;
}

void framework_ParcelDelete(void* parcel)
{
    tParcel *p = (tParcel*)parcel;
    if (parcel)
    {
        framework_FreeMem(p->m_data);
        framework_FreeMem(p);
    }
    
}

void framework_ParcelClear(void* parcel)
{
    tParcel *p = (tParcel*)parcel;
    
    p->m_curPosition = 0;
    p->m_curLength   = 0;
    memset(p->m_data, 0, p->m_dataLength);
}

const uint8_t* framework_ParcelDataAtCurrentPosition(void* parcel)
{
    tParcel *p = (tParcel*)parcel;
    return (p->m_data+p->m_curPosition);
}
void framework_ParcelDeserialize(void* dstParcel, void* srcParcel)
{
    tParcel *pDest = (tParcel*)dstParcel;
    tParcel *pSrc  = (tParcel*)srcParcel;
    uint8_t *data  = NULL;
    
    uint32_t size  = framework_ParcelReadInt32(pSrc);
    
    if (size > 0)
    {
        data = (uint8_t*)framework_AllocMem(size);
        framework_ParcelReadRaw(pSrc,data,size);
        framework_ParcelSetData(pDest,data,size);
        framework_FreeMem(data);
        framework_ParcelRewind(pDest);
    }else
    {
        framework_ParcelClear(pDest);
    }
    
}
void framework_ParcelForward(void* parcel, uint32_t nbBytes)
{
    tParcel *p = (tParcel*)parcel;
    
    if (p->m_curPosition+nbBytes <= p->m_curLength)
    {
        p->m_curPosition+=nbBytes;
    }
}
const uint8_t* framework_ParcelGetData(void* parcel)
{
    tParcel *p = (tParcel*)parcel;
    return p->m_data;
}
uint32_t framework_ParcelGetRemainingDataSize(void* parcel)
{
    tParcel *p = (tParcel*)parcel;
    return p->m_curLength-p->m_curPosition;
}
uint32_t framework_ParcelGetSize(void* parcel)
{
    tParcel *p = (tParcel*)parcel;
    return p->m_curLength;
}
uint8_t framework_ParcelReadByte(void* parcel)
{
    tParcel *p = (tParcel*)parcel;
    
    if (p->m_curPosition < p->m_curLength)
    {
        uint8_t result = p->m_data[p->m_curPosition];
        p->m_curPosition += sizeof(uint8_t);
        return result;
    }
    return 0;
}
double framework_ParcelReadDouble(void* parcel)
{
    tParcel *p = (tParcel*)parcel;
    
    if (p->m_curPosition < p->m_curLength)
    {
        void* result = p->m_data+p->m_curPosition;
        p->m_curPosition += sizeof(double);
        return *((double*)result);
    }
    return 0;
}
float framework_ParcelReadFloat(void* parcel)
{
    tParcel *p = (tParcel*)parcel;
    
    if (p->m_curPosition < p->m_curLength)
    {
        void* result = p->m_data+p->m_curPosition;
        p->m_curPosition += sizeof(float);
        return *((float*)result);
    }
    return 0;
}
uint32_t framework_ParcelReadInt32(void* parcel)
{
    tParcel *p = (tParcel*)parcel;
    
    if (p->m_curPosition < p->m_curLength)
    {
        void* result = p->m_data+p->m_curPosition;
        p->m_curPosition += sizeof(uint32_t);
        return *((uint32_t*)result);
    }
    return 0;
    
}
uint64_t framework_ParcelReadInt64(void* parcel)
{
    tParcel *p = (tParcel*)parcel;
    
    if (p->m_curPosition < p->m_curLength)
    {
        void* result = p->m_data+p->m_curPosition;
        p->m_curPosition += sizeof(uint64_t);
        return *((uint64_t*)result);
    }
    return 0;
    
}
void framework_ParcelReadRaw(void* parcel, void* outBuffer, uint32_t len)
{
    tParcel *p = (tParcel*)parcel;
    
    if (!outBuffer)
    {
        return;
    }

    if (p->m_curPosition < p->m_curLength)
    {
        uint8_t *start =p->m_data+p->m_curPosition;
        memcpy(outBuffer,start,len);
        p->m_curPosition += len;
    }
}
uint16_t framework_ParcelReadShort(void* parcel)
{
    tParcel *p = (tParcel*)parcel;
    
    if (p->m_curPosition < p->m_curLength)
    {
        void* result = p->m_data+p->m_curPosition;
        p->m_curPosition += sizeof(uint16_t);
        return *((uint16_t*)result);
    }
    return 0;
}
const char* framework_ParcelReadString(void* parcel)
{
    tParcel *p = (tParcel*)parcel;
    
    if (p->m_curPosition < p->m_curLength)
    {
        char* result =(char*)(p->m_data+p->m_curPosition);
        p->m_curPosition += strlen(result)+1;
        return result;
    }
    return NULL;
}
void framework_ParcelRewind(void* parcel)
{
    tParcel *p = (tParcel*)parcel;
    p->m_curPosition = 0;
}
void framework_ParcelSerialize(void* dstParcel, void* srcParcel)
{
    tParcel *pDest = (tParcel*)dstParcel;
    tParcel *pSrc  = (tParcel*)srcParcel;
    
    framework_ParcelWriteInt32(pDest,pSrc->m_curLength);
    if (pSrc->m_curLength>0)
    {
        framework_ParcelWriteRaw(dstParcel,pSrc->m_data,pSrc->m_curLength);
    }
}
void framework_ParcelSetData(void* parcel, const uint8_t* data, uint32_t size)
{
    tParcel *p = (tParcel*)parcel;
    
    framework_FreeMem(p->m_data);
    if (size > 0)
    {
        p->m_data = (uint8_t*)framework_AllocMem(size);
        memcpy(p->m_data,data,size);
        p->m_dataLength = size;
    }else
    {
        p->m_data = (uint8_t*)framework_AllocMem(32);
        memset(p->m_data,0,32);
        p->m_dataLength = 32;
    }
    
    p->m_curLength = size;

    p->m_curPosition = 0;
    
}
void framework_ParcelWriteByte(void* parcel, uint8_t i)
{
    tParcel *p = (tParcel*)parcel;
    
    growData(p,sizeof(uint8_t));
    p->m_data[p->m_curPosition] = i;
    INC_COUNTER(p,sizeof(uint8_t));
    
}
void framework_ParcelWriteDouble(void* parcel, double d)
{
    tParcel *p = (tParcel*)parcel;
    
    growData(p,sizeof(double));
    *((double*)(p->m_data+p->m_curPosition)) = d;
    INC_COUNTER(p,sizeof(double));
}
void framework_ParcelWriteFloat(void* parcel, float f)
{
    tParcel *p = (tParcel*)parcel;
    
    growData(p,sizeof(float));
    *((float*)(p->m_data+p->m_curPosition)) = f;
    INC_COUNTER(p,sizeof(float));
}
void framework_ParcelWriteInt32(void* parcel, uint32_t i)
{
    tParcel *p = (tParcel*)parcel;
    
    growData(p,sizeof(uint32_t));
    *((uint32_t*)(p->m_data+p->m_curPosition)) = i;
    INC_COUNTER(p,sizeof(uint32_t));
    
}
void framework_ParcelWriteInt64(void* parcel, uint64_t i)
{
    tParcel *p = (tParcel*)parcel;
    growData(p,sizeof(uint64_t));
    *((uint64_t*)(p->m_data+p->m_curPosition)) = i;
    INC_COUNTER(p,sizeof(uint64_t));
}
void framework_ParcelWriteRaw(void* parcel, const void* buffer, uint32_t len)
{
    tParcel *p = (tParcel*)parcel;
    growData(p,len);
    memcpy((p->m_data+p->m_curPosition),buffer,len);
    INC_COUNTER(p,len);
}
void framework_ParcelWriteShort(void* parcel, uint16_t i)
{
    tParcel *p = (tParcel*)parcel;
    growData(p,sizeof(uint16_t));
    *((uint16_t*)(p->m_data+p->m_curPosition)) = i;
    INC_COUNTER(p,sizeof(uint16_t));
}
void framework_ParcelWriteString(void* parcel, const char* s)
{
    tParcel *p = (tParcel*)parcel;
    
    growData(p,strlen(s)+1); // Trailling 0
    strcpy((char*)(p->m_data+p->m_curPosition),s);
    INC_COUNTER(p,strlen(s)+1);
}
