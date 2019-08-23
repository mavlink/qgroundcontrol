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

#include "TML/inc/framework_Timer.h"
#include "TML/inc/framework_Interface.h"

#include <stdlib.h>
#include <signal.h>
#include <time.h>

#define NSECS 1000000
#define MAX_NO_TIMERS 16

struct framework_Timer
{
   timer_t handle;
   framework_TimerCallBack *callback;
   void* pContext;
   int nIsStopped;
};

static struct framework_Timer timers[MAX_NO_TIMERS] =
{
   {0, NULL, NULL
     , 0
   },
};

static void framework_TimerExpired(union sigval sv)
{
	uint32_t timerid = (uint32_t)(sv.sival_int);

	if((timerid < MAX_NO_TIMERS) && (timers[timerid].nIsStopped == 1))
	{
		return;
	}

	if(timerid < MAX_NO_TIMERS)
	{
		framework_TimerStop(&timerid);
	    if (timers[timerid].callback)
	    {
	    	timers[timerid].callback(timers[timerid].pContext);
	    }
	}
}

static void framework_TimerDummyCb(void *pContext) {}

void framework_TimerCreate(void** timer)
{
	   uint32_t timerid;
	   struct sigevent se;

	   se.sigev_notify = SIGEV_THREAD;
	   se.sigev_notify_function = framework_TimerExpired;
	   se.sigev_notify_attributes = NULL;

	   /* Look for available timer slot */
	   for(timerid=0; timerid<MAX_NO_TIMERS; timerid++) {
	      if(timers[timerid].callback == NULL) break;
	   }
	   if(timerid == MAX_NO_TIMERS) {
		   *timer = NULL;
		   return;
	   }

	   se.sigev_value.sival_int = (int)timerid;

	   /* Create POSIX timer */
	   if(timer_create(CLOCK_REALTIME, &se, &(timers[timerid].handle)) == -1) {
		   *timer = NULL;
		   return;
	   }

	   timers[timerid].callback = framework_TimerDummyCb;
	   *timer = &timerid;
}

void framework_TimerDelete(void* timer)
{
	   uint32_t TimerId = *((uint32_t *) timer);
	   if(TimerId >= MAX_NO_TIMERS)
	      return;
	   if(timers[TimerId].callback == NULL)
	      return;

	   timer_delete(timers[TimerId].handle);

	   timers[TimerId].callback = NULL;
	   timers[TimerId].pContext = NULL;
}


void framework_TimerStart(void* timer,uint32_t delay, framework_TimerCallBack *cb,void *usercontext)
{
	   struct itimerspec its;
	   uint32_t TimerId = *((uint32_t *) timer);

	   if(TimerId >= MAX_NO_TIMERS)
	      return;
	   if(cb == NULL)
	      return;
	   if(timers[TimerId].callback == NULL)
	      return;

	   its.it_interval.tv_sec  = 0;
	   its.it_interval.tv_nsec = 0;
	   its.it_value.tv_sec     = delay / 1000;
	   its.it_value.tv_nsec    = 1000000 * (delay % 1000);
	   if(its.it_value.tv_sec == 0 && its.it_value.tv_nsec == 0)
	   {
	     // this would inadvertently stop the timer
	     its.it_value.tv_nsec = 1;
	   }

	   timers[TimerId].callback = cb;
	   timers[TimerId].pContext = usercontext;
	   timers[TimerId].nIsStopped = 0;

	   timer_settime(timers[TimerId].handle, 0, &its, NULL);
}


void framework_TimerStop(void* timer)
{
	   struct itimerspec its = {{0, 0}, {0, 0}};
	   uint32_t TimerId = *((uint32_t *) timer);

	   if(TimerId >= MAX_NO_TIMERS)
	      return;
	   if(timers[TimerId].callback == NULL)
	      return;
	   if(timers[TimerId].nIsStopped == 1)
	      return;

	   timers[TimerId].nIsStopped = 1;
	   timer_settime(timers[TimerId].handle, 0, &its, NULL);
}

