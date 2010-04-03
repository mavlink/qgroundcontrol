/*****************************************************************************
 *  Copyright (c) 2008, University of Florida
 *  All rights reserved.
 *  
 *  This file is part of OpenJAUS.  OpenJAUS is distributed under the BSD 
 *  license.  See the LICENSE file for details.
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of the University of Florida nor the names of its 
 *       contributors may be used to endorse or promote products derived from 
 *       this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ****************************************************************************/
// File Name: LocalComponent.cpp
//
// Written By: Danny Kent (jaus AT dannykent DOT com) 
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: Defines the common fuctionality of local components. 
//				Used by the NodeManagerComponent and CommunicatorComponent classes. 

#include "nodeManager/LocalComponent.h"
#include "utils/timeval.h"

void LocalComponent::run()
{
	struct timespec timeout;
	struct timeval now;
		
	if(!this->cmpt)
	{
		// ERROR: This cannot be called if we haven't yet constructed our component
		// TODO: Throw an exception? Log an error.
		return;
	}

	this->startupState();
	this->cmpt->state = JAUS_INITIALIZE_STATE;

	// Lock our mutex
	pthread_mutex_lock(&this->threadMutex);
	
	// prepare new timeout value.
	// Note that we need an absolute time.
	gettimeofday(&now, NULL);
	timeout.tv_sec = now.tv_sec;

	// timeval uses micro-seconds.
	// timespec uses nano-seconds.
	// 1 micro-second = 1000 nano-seconds.
	timeout.tv_nsec = (now.tv_usec * 1000) + (long)(1e9 / this->cmptRateHz);

	while(this->running)
	{
		while(timeout.tv_nsec > 1e9)
		{
			timeout.tv_nsec -= (long)1e9;
			timeout.tv_sec++;
		}
	
		int rc = pthread_cond_timedwait(&this->threadConditional, &this->threadMutex, &timeout);
		switch(rc)
		{
			case 0: // Conditional Signal
				// Check the send queue
				while(!this->queue.isEmpty())
				{
					// Pop a packet off the queue and send it off
					processMessage(queue.pop());

					// Check if we need to run our state thread!
					gettimeofday(&now, NULL);
					if( now.tv_sec > timeout.tv_sec || (now.tv_usec*1000) > timeout.tv_nsec)
					{
						break;
					}
				}
				break;
			
			case ETIMEDOUT: // our time is up
				// Capture time now, that way we get a constant deltaTime from beginning to beginning
				gettimeofday(&now, NULL);

				switch(cmpt->state)
				{
					case JAUS_INITIALIZE_STATE:
						intializeState();
						break;

					case JAUS_STANDBY_STATE:
						standbyState();
						break;

					case JAUS_READY_STATE:
						readyState();
						break;

					case JAUS_EMERGENCY_STATE:
						emergencyState();
						break;

					case JAUS_FAILURE_STATE:
						failureState();
						break;
					
					default:
						//do nothing
						break;
				}
				allState();

				// prepare new timeout value.
				// Note that we need an absolute time.
				timeout.tv_sec = now.tv_sec;

				// timeval uses micro-seconds.
				// timespec uses nano-seconds.
				// 1 micro-second = 1000 nano-seconds.
				timeout.tv_nsec = (now.tv_usec * 1000) + (long)(1e9 / this->cmptRateHz);

				while(!this->queue.isEmpty())
				{
					// Pop a packet off the queue and send it off
					processMessage(queue.pop());
					
					// Check if we need to run our state thread!
					gettimeofday(&now, NULL);
					if( now.tv_sec > timeout.tv_sec || (now.tv_usec*1000) > timeout.tv_nsec)
					{
						break;
					}
				}
				break;

			default:
				// Some other error occured
				// TODO: Log error.
				gettimeofday(&now, NULL);

				// prepare new timeout value.
				// Note that we need an absolute time.
				timeout.tv_sec = now.tv_sec;

				// timeval uses micro-seconds.
				// timespec uses nano-seconds.
				// 1 micro-second = 1000 nano-seconds.
				timeout.tv_nsec = (now.tv_usec * 1000) + (long)(1e9 / this->cmptRateHz);
				break;
		}
	}
	
	while(!this->queue.isEmpty())
	{
		// Pop a packet off the queue and send it off
		processMessage(queue.pop());
	}
	
	pthread_mutex_unlock(&this->threadMutex);
	
	shutdownState();
}

