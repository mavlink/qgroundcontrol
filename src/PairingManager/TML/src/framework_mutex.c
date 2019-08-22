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

#include "TML/inc/framework_Allocator.h"

// **************************************** OS Specific functions

#include <pthread.h>
#include <errno.h>
#include <string.h>

typedef struct tLinuxMutex
{
	pthread_mutex_t *lock;
	pthread_cond_t  *cond;
}tLinuxMutex_t;

eResult framework_CreateMutex(void ** mutexHandle)
{
	tLinuxMutex_t *mutex = (tLinuxMutex_t *)framework_AllocMem(sizeof(tLinuxMutex_t));
	
	mutex->lock = (pthread_mutex_t*)framework_AllocMem(sizeof(pthread_mutex_t));
	mutex->cond = (pthread_cond_t*)framework_AllocMem(sizeof(pthread_cond_t));
	
	pthread_mutex_init(mutex->lock,NULL);
	pthread_cond_init(mutex->cond,NULL);
	
	*mutexHandle = mutex;
	
	return FRAMEWORK_SUCCESS;
}

void framework_LockMutex(void * mutexHandle)
{
	tLinuxMutex_t *mutex = (tLinuxMutex_t*)mutexHandle;
	
	int res = pthread_mutex_lock(mutex->lock);
	if (res)
	{
		framework_Error("lock() failed errno %i\n",strerror (errno));
	}
}

void framework_UnlockMutex(void * mutexHandle)
{
	tLinuxMutex_t *mutex = (tLinuxMutex_t*)mutexHandle;
	int res = pthread_mutex_unlock(mutex->lock);
	if (res)
	{
		framework_Error("unlock() failed %s\n",strerror (errno));
	}
}

void framework_WaitMutex(void * mutexHandle, uint8_t needLock)
{
	tLinuxMutex_t *mutex = (tLinuxMutex_t*)mutexHandle;
	
	if (needLock)
	{
		framework_LockMutex(mutexHandle);
	}

	pthread_cond_wait(mutex->cond,mutex->lock);
	
	if (needLock)
	{
		framework_UnlockMutex(mutexHandle);
	}
	
}

void framework_NotifyMutex(void * mutexHandle, uint8_t needLock)
{
	tLinuxMutex_t *mutex = (tLinuxMutex_t*)mutexHandle;
	
	if (needLock)
	{
		framework_LockMutex(mutexHandle);
	}

	pthread_cond_broadcast(mutex->cond);
	
	if (needLock)
	{
		framework_UnlockMutex(mutexHandle);
	}
}

void framework_DeleteMutex(void * mutexHandle)
{
	tLinuxMutex_t *mutex = (tLinuxMutex_t*)mutexHandle;
	
	pthread_mutex_destroy(mutex->lock);
	pthread_cond_destroy(mutex->cond);
	
	framework_FreeMem(mutex);
}
