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

#ifndef FRAMEWORK_TIMER_H
#define FRAMEWORK_TIMER_H

#include "TML/inc/framework_Interface.h"

typedef void (framework_TimerCallBack)(void*);

void framework_TimerCreate(void **timer);
void framework_TimerStart(void *timer,uint32_t delay,framework_TimerCallBack *cb,void *usercontext);
void framework_TimerStop(void *timer);
void framework_TimerDelete(void *timer);



#endif // ndef FRAMEWORK_TIMER_H

