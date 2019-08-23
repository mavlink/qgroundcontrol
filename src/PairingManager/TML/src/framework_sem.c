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

#include <TML/inc/framework_linux.h>
#include <TML/inc/framework_Allocator.h>

// ####################### Semaphore Generic implementation.

typedef struct tGenericSemaphore
{
	int8_t counter;
	void  *mutex;
}tGenericSemaphore_t;

eResult framework_CreateSemaphore(void ** semaphoreHandle)
{
	tGenericSemaphore_t *sem = (tGenericSemaphore_t *)framework_AllocMem(sizeof(tGenericSemaphore_t));

	sem->counter = 0;

	framework_CreateMutex(&(sem->mutex));

	*semaphoreHandle = sem;

	return FRAMEWORK_SUCCESS;
}

void framework_WaitSemaphore(void * semaphoreHandle)
{
	tGenericSemaphore_t *sem = (tGenericSemaphore_t *)semaphoreHandle;

	framework_LockMutex(sem->mutex);
	sem->counter++;
	if (sem->counter > 0)
	{
		framework_WaitMutex(sem->mutex, 0);
	}
	framework_UnlockMutex(sem->mutex);
}

void framework_PostSemaphore(void * semaphoreHandle)
{
	tGenericSemaphore_t *sem = (tGenericSemaphore_t *)semaphoreHandle;
	uint8_t needWakeUp = 0;

	framework_LockMutex(sem->mutex);

	if (sem->counter > 0)
	{
		needWakeUp = 1;
	}
	sem->counter--;

	if (needWakeUp)
	{
		framework_NotifyMutex(sem->mutex, 0);
	}
	framework_UnlockMutex(sem->mutex);
}

void framework_DeleteSemaphore(void * semaphoreHandle)
{
	tGenericSemaphore_t *sem = (tGenericSemaphore_t *)semaphoreHandle;
	framework_DeleteMutex(sem->mutex);
	framework_FreeMem(sem);
}
