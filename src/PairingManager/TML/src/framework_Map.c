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
 
#include "TML/inc/framework_Map.h"
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

typedef struct map_element
{
	void* id;
	void* object;
	struct map_element_t* next;
}map_element_t;

typedef struct map
{
	char start[14];
	map_element_t* elements;
	char end[12];
}map_t;

STATUS map_isValid(map_t* map);

STATUS map_create(void ** map)
{
	STATUS lStatus = SUCCESS;
	map_t* lMap = NULL;

	*map = MEM_ALLOCATOR(sizeof(map_t));
	if (NULL == *map)
	{
		lStatus = FAILED;
	}
	else
	{
		lMap = (map_t*)*map;
		lMap->elements = NULL;
		STR_CPY(lMap->start, START_OK, sizeof(START_OK));
		STR_CPY(lMap->end, END_OK, sizeof(END_OK));
	}
	return lStatus;
}

STATUS map_destroy(void* map)
{
	STATUS lStatus = SUCCESS;
	map_t* lMap = (map_t*)map;

	lStatus = map_isValid(lMap);

	if (SUCCESS == lStatus)
	{
		while (NULL != lMap->elements)
		{
			map_remove(map, lMap->elements->id);
		}

		STR_CPY(lMap->start, START_KO, sizeof(START_KO));
		STR_CPY(lMap->end, END_KO, sizeof(END_KO));

		MEM_DEALLOCATOR(lMap);
		lMap = NULL;
	}
	return lStatus;
}

STATUS map_add(void * map, void* id, void* object)
{
	STATUS lStatus = SUCCESS;
	map_t* lMap = (map_t*)map;
	void* lObject = NULL;
	map_element_t* lElement = NULL;
	map_element_t* lElementTmp = NULL;

	lStatus = map_isValid(lMap);

	if (SUCCESS == lStatus)
	{
		lStatus = map_get(map, id, &lObject);
		if (SUCCESS == lStatus)
		{
			lStatus = ALREADY_EXISTS;
		}
		else
		{
			lElement = (map_element_t*)MEM_ALLOCATOR(sizeof(map_element_t));
			if (NULL == lElement)
			{
				lStatus = FAILED;
			}
			else
			{
				lElement->id = id;
				lElement->next = NULL;
				lElement->object = object;
				lElementTmp = lMap->elements;
				if (NULL == lElementTmp)
				{
					lMap->elements = lElement;
				}
				else
				{
					while (NULL != lElementTmp->next)
					{
						lElementTmp = (map_element_t *) lElementTmp->next;
					}
					lElementTmp->next = (struct map_element_t *) lElement;
				}
				lStatus = SUCCESS;
			}
		}
	}

	return lStatus;

}

STATUS map_getAll(void* map, void ** elements, int * lenght)
{
	STATUS lStatus = SUCCESS;
	map_t* lMap = (map_t*)map;
	map_element_t* lElement = NULL;
    int i;

	lStatus = map_isValid(lMap);

	if (SUCCESS == lStatus)
	{
	    if (0x00 == *lenght || NULL == elements)
	    {
		*lenght = 0x00;
		lElement = lMap->elements;

		while (NULL != lElement)
		{
		    (*lenght)++;
		    lElement = (map_element_t *) lElement->next;
		}
	    }
	    else
	    {
	    	lElement = lMap->elements;
	    	for (i = 0x00; i < *lenght; i++)
	    	{
	    		elements[i] = lElement->object;
	    	}
	    }
	}
	return lStatus;
}

STATUS map_remove(void* map, void* id)
{
	STATUS lStatus = SUCCESS;
	map_t* lMap = (map_t*)map;
	map_element_t* lElementTmp = NULL;
	map_element_t* lElement = NULL;
	int lElementFound = 0x00;

	lStatus = map_isValid(map);

	if (SUCCESS == lStatus)
	{
		lElement = lMap->elements;
		lElementTmp = NULL;
		while (NULL != lElement)
		{
			if (lElement->id == id)
			{
				lElementFound = 0x01;

				if (lMap->elements == lElement)
				{
					lMap->elements = (map_element_t *) lElement->next;
				}
				else
				{
					lElementTmp->next = lElement->next;
				}
				break;
			}
			lElementTmp = lElement;
			lElement = (map_element_t *) lElement->next;
		}

		if (0x00 == lElementFound)
		{
			lStatus = NOT_FOUND;
		}
		else
		{
			MEM_DEALLOCATOR(lElement);
		}
	}

	return lStatus;
}

STATUS map_get(void* map, void* id, void ** object)
{
	STATUS lStatus = SUCCESS;
	map_t* lMap = (map_t*)map;
	map_element_t* lElement = NULL;

	lStatus = map_isValid(lMap);

	if (NULL == object)
	{
		lStatus = INVALID_PARAM;
	}

	if (SUCCESS == lStatus)
	{
		*object = NULL;
		if (NULL != lMap->elements)
		{
			lElement = lMap->elements;

			while (NULL != lElement)
			{
				if (lElement->id == id)
				{
					*object = lElement->object;
					break;
				}
				lElement = (map_element_t *) lElement->next;
			}

			if (NULL == *object)
			{
				lStatus = NOT_FOUND;
			}
		}
		else
		{
			lStatus = NOT_FOUND;
		}
	}

	return lStatus;
}

STATUS map_isValid(map_t* map)
{
	STATUS lStatus = SUCCESS;
	if (NULL == map)
	{
		lStatus = INVALID_PARAM;
	}
	if (SUCCESS == lStatus)
	{
		if ((0x00 != STR_CMP(START_OK, map->start, sizeof(START_OK))) || (0x00 != STR_CMP(END_OK, map->end, sizeof(END_OK))))
		{
			lStatus = INVALID_MAP;
		}
	}
	return lStatus;
}
