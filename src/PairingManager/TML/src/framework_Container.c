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

#include "TML/inc/framework_Container.h"
#include <stdlib.h>
#include <string.h>

#define MEM_ALLOCATOR(x) malloc(x)
#define MEM_DEALLOCATOR(x) free(x)

#define START_OK "valid_start"
#define START_KO "deleted_start"

#define END_OK "valid_end"
#define END_KO "deleted_end"

#define STR_CMP(x, y, z) memcmp(x, y, z)
#define STR_CPY(x, y, z) memcpy(x, y, z)
 
 typedef struct Container
 {
    char         start[14];
    void       **m_data;
    uint32_t     m_size_data;
    uint32_t     m_alloc_size;
    char         end[12];
 }Container_h;
 
void container_check_size(Container_h* pContainer);
void container_rebuild(Container_h* pContainer, uint32_t index);
CONTAINER_STATUS container_isValid(Container_h* pContainer);


CONTAINER_STATUS container_create(void** pContainer, uint32_t size)
{
    CONTAINER_STATUS lStatus = CONTAINER_SUCCESS;
    Container_h *lContainer = NULL;
    
    if (NULL == pContainer)
    {
        lStatus = CONTAINER_INVALID_PARAM;
    }
    
    if(CONTAINER_SUCCESS == lStatus)
    {
        *pContainer = MEM_ALLOCATOR(sizeof(Container_h));
        if (NULL == *pContainer)
        {
            lStatus = CONTAINER_FAILED;
        }
        else
        {
            lContainer = (Container_h*)*pContainer;
            lContainer->m_data=(void**)MEM_ALLOCATOR(size*sizeof(void*));
            memset(lContainer->m_data,0,size*sizeof(void*));
            lContainer->m_size_data=0;
            lContainer->m_alloc_size=size;
            STR_CPY(lContainer->start, START_OK, sizeof(START_OK));
            STR_CPY(lContainer->end, END_OK, sizeof(END_OK));
        }
    }
    
    return lStatus;
}

CONTAINER_STATUS container_delete(void* pContainer)
{
    CONTAINER_STATUS lStatus = CONTAINER_SUCCESS;
    Container_h *lContainer = (Container_h *) pContainer;
    
    lStatus = container_isValid(lContainer);
    
    if(CONTAINER_SUCCESS == lStatus)
    {
        if(NULL != lContainer->m_data)
        {
            MEM_DEALLOCATOR(lContainer->m_data);
            lContainer->m_data = NULL;
            
            STR_CPY(lContainer->start, START_KO, sizeof(START_OK));
            STR_CPY(lContainer->end, END_KO, sizeof(END_OK));
        }
    }
    
    return lStatus;
}


CONTAINER_STATUS container_add(void* pContainer, void* _ptr)
{
    CONTAINER_STATUS lStatus = CONTAINER_SUCCESS;
    Container_h *lContainer = (Container_h *) pContainer;
    
    lStatus = container_isValid(lContainer);
    
    if(CONTAINER_SUCCESS == lStatus)
    {
        container_check_size(lContainer);
        lContainer->m_data[lContainer->m_size_data] = _ptr;
        lContainer->m_size_data++;
    }
    
    return lStatus;
}


CONTAINER_STATUS container_set(void* pContainer, uint32_t index,void*_ptr, void** _old)
{
    CONTAINER_STATUS lStatus = CONTAINER_SUCCESS;
    Container_h *lContainer = (Container_h *) pContainer;
    
    lStatus = container_isValid(lContainer);
    
    
    *_old = NULL;
    if(CONTAINER_SUCCESS == lStatus)
    {
        if (index > lContainer->m_size_data)
        {
            container_add(pContainer, _ptr);
        }
        else
        {
            *_old = lContainer->m_data[index];
            lContainer->m_data[index] = _ptr;
        }
    }
    
    return lStatus;
}

CONTAINER_STATUS container_remove(void* pContainer, uint32_t index, void** _old)
{
    CONTAINER_STATUS lStatus = CONTAINER_SUCCESS;
    Container_h *lContainer = (Container_h *) pContainer;
    
    lStatus = container_isValid(lContainer);
    
    *_old = NULL;
    if(CONTAINER_SUCCESS == lStatus)
    {
        if (index < lContainer->m_size_data)
        {
            *_old = lContainer->m_data[index];
            lContainer->m_data[index] = NULL;
            
            // this is not the last one
            if (index != (lContainer->m_size_data - 1))
            {
                container_rebuild(lContainer, index);
            }
            lContainer->m_size_data--;
        }
    }
    return lStatus;
}

CONTAINER_STATUS container_get(void* pContainer, uint32_t index, void** _out)
{
    CONTAINER_STATUS lStatus = CONTAINER_SUCCESS;
    Container_h *lContainer = (Container_h *) pContainer;
    
    lStatus = container_isValid(lContainer);
    
    *_out = NULL;
    if(CONTAINER_SUCCESS == lStatus)
    {
        if (index < lContainer->m_size_data)
        {
            *_out = lContainer->m_data[index];
        }
    }
    
    return lStatus;
}

CONTAINER_STATUS container_size(void* pContainer, uint32_t* size)
{
    CONTAINER_STATUS lStatus = CONTAINER_SUCCESS;
    Container_h *lContainer = (Container_h *) pContainer;
    
    lStatus = container_isValid(lContainer);
    
    if(CONTAINER_SUCCESS == lStatus)
    {
        *size =  lContainer->m_size_data;
    }
    
    return lStatus;
}

void container_check_size(Container_h* pContainer)
{
    void** m_new_data;

    if (pContainer->m_size_data == pContainer->m_alloc_size)
    {
        pContainer->m_alloc_size = pContainer->m_alloc_size * 2;

        m_new_data = (void**) MEM_ALLOCATOR(sizeof(void*) * pContainer->m_alloc_size);
        memmove(m_new_data, pContainer->m_data, pContainer->m_size_data * sizeof(void*));
        MEM_DEALLOCATOR(pContainer->m_data);
        pContainer->m_data = m_new_data;
    }
}

void container_rebuild(Container_h* pContainer, uint32_t index)
{
    void** _zero = pContainer->m_data + index;
    void** _start_cpy = _zero + 1;
    int32_t size = (pContainer->m_size_data - 1) - index;
    if (size <= 0) return;
    memmove(_zero, _start_cpy, size * sizeof(void*));
}

CONTAINER_STATUS container_clear(void* pContainer)
{
    CONTAINER_STATUS lStatus = CONTAINER_SUCCESS;
    Container_h *lContainer = (Container_h *) pContainer;
    
    lStatus = container_isValid(lContainer);
    
    if(CONTAINER_SUCCESS == lStatus)
    {
        memset(lContainer->m_data, 0, lContainer->m_size_data);
        lContainer->m_size_data = 0;
    }
    
    return lStatus;
}


CONTAINER_STATUS container_flushMallocedContent(void* pContainer)
{
    CONTAINER_STATUS lStatus = CONTAINER_SUCCESS;
    Container_h *lContainer = (Container_h *) pContainer;
    uint32_t sz, i;
    void *old;

    lStatus = container_isValid(lContainer);
    
    if(CONTAINER_SUCCESS == lStatus)
    {
        old = NULL;
        sz = 0x00;
        container_size(pContainer, &sz);
        for (i = 0 ; i < sz;i++)
        {
            container_get(pContainer, i, &old);
            MEM_DEALLOCATOR(old);
        }
        container_clear(pContainer);
    }
    
    return lStatus;
}

CONTAINER_STATUS container_removePtr(void* pContainer, void* ref, void** out)
{
    CONTAINER_STATUS lStatus = CONTAINER_SUCCESS;
    Container_h *lContainer = (Container_h *) pContainer;
    uint32_t sz, i;
    void *scan;

    lStatus = container_isValid(lContainer);
    
    if(CONTAINER_SUCCESS == lStatus)
    {
        scan = NULL;
        sz = 0x00;
        container_size(pContainer, &sz);
        
        *out = NULL;
        for (i = 0; i < sz; i++)
        {
            container_get(pContainer, i, &scan);
            if (scan == ref)
            {
                container_remove(pContainer, i, out);
            }
        }
    }
    return lStatus;
}




CONTAINER_STATUS container_isValid(Container_h* pContainer)
{
    CONTAINER_STATUS lStatus = CONTAINER_SUCCESS;
    if (NULL == pContainer)
    {
        lStatus = CONTAINER_INVALID_PARAM;
    }
    if (CONTAINER_SUCCESS == lStatus)
    {
        if ((0x00 != STR_CMP(START_OK, pContainer->start, sizeof(START_OK))) || (0x00 != STR_CMP(END_OK, pContainer->end, sizeof(END_OK))))
        {
            lStatus = CONTAINER_INVALID_CONTAINER;
        }
    }
    return lStatus;
}
 
 
