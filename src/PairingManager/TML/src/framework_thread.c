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

#include <pthread.h>
#include <errno.h>
#include <unistd.h>

// **************************************** OS Specific functions

typedef struct tLinuxThread
{
	pthread_t thread;
	void* ctx;
	void* (*threadedFunc)(void *);
	void* mutexCanDelete;
}tLinuxThread_t;

void* thread_object_func(void* obj)
{
	tLinuxThread_t *linuxThread = (tLinuxThread_t *)obj;
	void *res = NULL;
	framework_LockMutex(linuxThread->mutexCanDelete);
	res = linuxThread->threadedFunc(linuxThread->ctx);
	framework_UnlockMutex(linuxThread->mutexCanDelete);

	return res;
}


eResult framework_CreateThread(void** threadHandle, void * (* threadedFunc)(void *) , void * ctx)
{
	tLinuxThread_t *linuxThread = (tLinuxThread_t *)framework_AllocMem(sizeof(tLinuxThread_t));
	
	linuxThread->ctx = ctx;
	linuxThread->threadedFunc = threadedFunc;
	framework_CreateMutex(&(linuxThread->mutexCanDelete));
	
	if (pthread_create(&(linuxThread->thread), NULL, thread_object_func, linuxThread))
	{
		framework_Error("Cannot create Thread\n");
		framework_DeleteMutex(linuxThread->mutexCanDelete);
		framework_FreeMem(linuxThread);
		
		return FRAMEWORK_FAILED;
	}
	pthread_detach(linuxThread->thread);
	
	*threadHandle = linuxThread;
	
	return FRAMEWORK_SUCCESS;
}

void framework_JoinThread(void * threadHandle)
{
	tLinuxThread_t *linuxThread = (tLinuxThread_t*)threadHandle;
	if (pthread_self() != linuxThread->thread)
	{
		// Will cause block if thread still running !!!
		framework_LockMutex(linuxThread->mutexCanDelete);
		framework_UnlockMutex(linuxThread->mutexCanDelete);
		// Thread now just ends up !
	}
}


void framework_DeleteThread(void * threadHandle)
{
	tLinuxThread_t *linuxThread = (tLinuxThread_t*)threadHandle;
	framework_DeleteMutex(linuxThread->mutexCanDelete);
	framework_FreeMem(linuxThread);
}

void * framework_GetCurrentThreadId()
{
	return (void*)pthread_self();
}

void * framework_GetThreadId(void * threadHandle)
{
	tLinuxThread_t *linuxThread = (tLinuxThread_t*)threadHandle;
	return (void*)linuxThread->thread;
}

void framework_MilliSleep(uint32_t ms)
{
	usleep(1000*ms);
}
