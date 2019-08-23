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

#ifndef FRAMEWORK_PARCEL_H
#define FRAMEWORK_PARCEL_H

#include "TML/inc/framework_Interface.h"

void framework_ParcelCreate(void **parcel);
void framework_ParcelDelete(void *parcel);
uint8_t framework_ParcelReadByte(void *parcel);
void framework_ParcelWriteByte(void *parcel,uint8_t i);
uint16_t framework_ParcelReadShort(void *parcel);
void framework_ParcelWriteShort(void *parcel,uint16_t i);
uint32_t framework_ParcelReadInt32(void *parcel);
void framework_ParcelWriteInt32(void *parcel,uint32_t i);
uint64_t framework_ParcelReadInt64(void *parcel);
void framework_ParcelWriteInt64(void *parcel,uint64_t i);
float framework_ParcelReadFloat(void *parcel);
void framework_ParcelWriteFloat(void *parcel,float f);
double framework_ParcelReadDouble(void *parcel);
void framework_ParcelWriteDouble(void *parcel,double d);
const char* framework_ParcelReadString(void *parcel);
void framework_ParcelWriteString(void *parcel,const char* s);
void framework_ParcelReadRaw(void *parcel,void* outBuffer, uint32_t len);
void framework_ParcelWriteRaw(void *parcel,const void* buffer, uint32_t len);
const uint8_t* framework_ParcelDataAtCurrentPosition(void *parcel);
uint32_t framework_ParcelGetRemainingDataSize(void *parcel);
const uint8_t *framework_ParcelGetData(void *parcel);
uint32_t framework_ParcelGetSize(void *parcel);
void framework_ParcelSetData(void *parcel,const uint8_t* data,uint32_t size);
void framework_ParcelRewind(void *parcel);
void framework_ParcelForward(void *parcel,uint32_t nbBytes);
void framework_ParcelClear(void *parcel);
void framework_ParcelSerialize(void *dstParcel,void *srcParcel);
void framework_ParcelDeserialize(void *dstParcel,void *srcParcel);

#endif // ndef FRAMEWORK_PARCEL_H
