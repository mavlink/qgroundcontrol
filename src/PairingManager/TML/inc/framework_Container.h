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

#ifndef FRAMEWORK_CONTAINER_H
#define FRAMEWORK_CONTAINER_H

#include "TML/inc/framework_Interface.h"

typedef enum CONTAINER_STATUS
{
    CONTAINER_SUCCESS,
    CONTAINER_FAILED,
    CONTAINER_INVALID_PARAM,
    CONTAINER_INVALID_CONTAINER
}CONTAINER_STATUS;

CONTAINER_STATUS container_create(void** lContainer, uint32_t size);

CONTAINER_STATUS container_delete(void* lContainer);

CONTAINER_STATUS container_add(void* pContainer, void* _ptr);

CONTAINER_STATUS container_set(void* pContainer, uint32_t index,void*_ptr, void** _old);

CONTAINER_STATUS container_remove(void* pContainer, uint32_t index, void** _old);

CONTAINER_STATUS container_get(void* pContainer, uint32_t index, void** _out);

CONTAINER_STATUS container_size(void* pContainer, uint32_t* size);

CONTAINER_STATUS container_clear(void* pContainer);

CONTAINER_STATUS container_flushMallocedContent(void* pContainer);

CONTAINER_STATUS container_removePtr(void* pContainer, void* ref, void** out);

#endif /* FRAMEWORK_CONTAINER_H*/
