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
#include <jaus.h>			// JAUS message set (USER: JAUS libraries must be installed first)
#include <openJaus.h>
#include <pthread.h>			// Multi-threading functions (standard to unix)
#include <stdlib.h>	
//#include <unistd.h>				// Unix standard functions
#include <string.h>
#include "properties.h"
#include "vehicleSim.h"

#if defined (WIN32)
	#define _USE_MATH_DEFINES
	#include <math.h>
	#define CONFIG_DIRECTORY ".\\config\\"
#elif defined(__linux) || defined(linux) || defined(__linux__) || defined(__APPLE__)
	#define CONFIG_DIRECTORY "./config/"
#endif

#define MIN_TURNING_RADIUS_M		4.25

#define IDLE_FORCE_N				200.0

#define VEHICLE_MASS_KG 			1700.0

#define DRAG_COEFF_N_PER_MPS_SQ 	100.0

#define K_STEERING					2.0
#define MAX_STEERING_RATE 			50.0

#define K_THROTTLE					40.0
		
#define BRAKE_TAU 					1.0

#define K_DYNAMIC_BRAKE 			60.0
#define K_STATIC_BRAKE 				100.0

void *vehicleSimThread(void *);

static int vehicleSimRun = FALSE;
static double vehicleSimThreadHz = 0;				// Stores the calculated uvehicleSimate rate for main state thread
static int vehicleSimThreadRunning = FALSE;
static pthread_t vehicleSimThreadId;			// pthread component thread identifier

static Properties vehicleSimProperties;

static int vehicleRunPause = VEHICLE_SIM_RUN;

static PointLla vehiclePosLla = NULL;
static PointUtm vehiclePosUtm = NULL;

static double vehicleH = 0;
static double vehicleSpeed = 0;

static double vehicleDesiredRotationalEffort = 0;
static double vehicleDesiredLinearEffortX = 0;
static double vehicleDesiredResistiveEffortX = 0;

static double vehicleRotationalEffort = 0;
static double vehicleLinearEffortX = 0;
static double vehicleResistiveEffortX = 0;

int vehicleSimStartup(void)
{
	FILE * propertyFile = NULL;
	char *property = NULL;
	pthread_attr_t attr;	// Thread attributed for the component threads spawned in this function
	char fileName[128] = {0};

	// Create vehiclePosLla object
	vehiclePosLla = pointLlaCreate();
	if(!vehiclePosLla)
	{
		//cError("vehicleSim: Could not create vehicleSimThread\n");
		vehicleSimShutdown();
		return VEHICLE_SIM_MALLOC_ERROR;
	}
	
	sprintf(fileName, "%svehicleSim.conf", CONFIG_DIRECTORY);
	propertyFile = fopen(fileName, "r");
	if(propertyFile)
	{
		vehicleSimProperties = propertiesCreate();
		vehicleSimProperties = propertiesLoad(vehicleSimProperties, propertyFile);
		fclose(propertyFile);
	}
	else
	{
		//cError("vehicleSim: Cannot find or open properties file\n");
		return VEHICLE_SIM_LOAD_CONFIGURATION_ERROR;
	}
	
	property = propertiesGetProperty(vehicleSimProperties, "INITIAL_LAT_DEGREES");
	if(property)
	{
		vehiclePosLla->latitudeRadians = atof( property );
		//cDebug(3, "vehicleSim: Property loaded INITIAL_LAT_DEGREES: %lf\n", vehiclePosLla->latitudeRadians);
		vehiclePosLla->latitudeRadians *= RAD_PER_DEG;
	}
	else
	{
		//cDebug(3, "vehicleSim: Property INITIAL_LAT_DEGREES not found, using default value: %lf\n", vehiclePosLla->latitudeRadians);
	}

	property = propertiesGetProperty(vehicleSimProperties, "INITIAL_LON_DEGREES");
	if(property)
	{
		vehiclePosLla->longitudeRadians = atof( property );
		//cDebug(3, "vehicleSim: Property loaded INITIAL_LON_DEGREES: %lf\n", vehiclePosLla->longitudeRadians);
		vehiclePosLla->longitudeRadians *= RAD_PER_DEG;
	}
	else
	{
		//cDebug(3, "vehicleSim: Property INITIAL_LON_DEGREES not found, using default value: %lf\n", vehiclePosLla->longitudeRadians);
	}

	property = propertiesGetProperty(vehicleSimProperties, "INITIAL_HEADING_DEGREES");
	if(property)
	{
		vehicleH = atof( property );
		//cDebug(3, "vehicleSim: Property loaded INITIAL_HEADING_DEGREES: %lf\n", vehicleH);
	}
	else
	{
		//cDebug(3, "vehicleSim: Property INITIAL_HEADING_DEGREES not found, using default value: %lf\n", vehicleH);
	}

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	vehicleSimRun = TRUE;

	if(pthread_create(&vehicleSimThreadId, &attr, vehicleSimThread, NULL) != 0)
	{
		//cError("vehicleSim: Could not create vehicleSimThread\n");
		vehicleSimShutdown();
		pthread_attr_destroy(&attr);
		return VEHICLE_SIM_THREAD_CREATE_ERROR;
	}
	pthread_attr_destroy(&attr);
	
	return 0;
}

// Function: vehicleSimShutdown
// Access:		Public	
// Description:	This function allows the abstracted component functionality contained in this file to be stoped from an external source.
// 				If the component is in the running state, this function will terminate all threads running in this file
//				This function will also close the Jms connection to the Node Manager and check out the component from the Node Manager
int vehicleSimShutdown(void)
{
	double timeOutSec;

	vehicleSimRun = FALSE;

	timeOutSec = ojGetTimeSec() + VEHICLE_SIM_THREAD_TIMEOUT_SEC;
	while(vehicleSimThreadRunning)
	{
		ojSleepMsec(100);
		if(ojGetTimeSec() >= timeOutSec)
		{
			pthread_cancel(vehicleSimThreadId);
			vehicleSimThreadRunning = FALSE;
			//cError("vehicleSim: vehicleSimThread Shutdown Improperly\n");
			break;
		}
	}

	propertiesDestroy(vehicleSimProperties);

	return 0;
}

int vehicleSimGetState(void)
{
	return VEHICLE_SIM_READY_STATE;
}

int vehicleSimGetRunPause(void)
{
	return vehicleRunPause;
}

void vehicleSimToggleRunPause(void)
{
	vehicleRunPause = vehicleRunPause == VEHICLE_SIM_PAUSE ? VEHICLE_SIM_RUN : VEHICLE_SIM_PAUSE;
}

double vehicleSimGetUpdateRate(void)
{
	return vehicleSimThreadHz;
}

PointLla vehicleSimGetPositionLla(void)
{
	return vehiclePosLla;
}

double vehicleSimGetX(void)
{
	if(vehiclePosUtm)
	{
		return vehiclePosUtm->xMeters;
	}
	else
	{
		return 0;		
	}
}

double vehicleSimGetY(void)
{
	if(vehiclePosUtm)
	{
		return vehiclePosUtm->yMeters;
	}
	else
	{
		return 0;
	}
}

double vehicleSimGetH(void)
{
	return vehicleH;
}

double vehicleSimGetSpeed(void)
{
	return vehicleSpeed;
}

double vehicleSimGetRotateEffort(void)
{
	return vehicleRotationalEffort;
}

double vehicleSimGetLinearEffortX(void)
{
	return 	vehicleDesiredLinearEffortX;
}

double vehicleSimGetResistiveEffortX(void)
{
	return 	vehicleDesiredResistiveEffortX;
}

double vehicleSimGetRotationalEffort(void)
{
	return 	vehicleDesiredRotationalEffort;
}

void vehicleSimSetCommand(double throttle, double brake, double steering)
{
	if(throttle <= 100.0 && throttle >= 0.0)
	{
		vehicleDesiredLinearEffortX = throttle;
	}

	if(brake <= 100.0 && brake >= 0.0)
	{
		vehicleDesiredResistiveEffortX = brake;
	}
	
	if(steering <= 100.0 && steering >= -100.0)
	{
		vehicleDesiredRotationalEffort = steering;
	}
}

// Function: vehicleSimThread
// Access:		Private
// Description:	All core component functionality is contained in this thread.
//				All of the JAUS component state machine code can be found here.
void *vehicleSimThread(void *threadData)
{
	double time, prevTime, dt;
	double steeringRate;

	double motorForce;
	double brakeForce;
	double dragForce;

	double turningCurvature;
	double deltaH;	

	vehicleSimThreadRunning = TRUE;

	// Initialize the UTM engine
	utmConversionInit(vehiclePosLla);
	vehiclePosUtm = pointLlaToPointUtm(vehiclePosLla);

	time = ojGetTimeSec();
		
	while(vehicleSimRun) // Execute state machine code while not in the SHUTDOWN state
	{
		prevTime = time;
		time = ojGetTimeSec();
		dt = time - prevTime;
		vehicleSimThreadHz = 1.0/dt; // Compute the update rate of this thread

		steeringRate = K_STEERING * (vehicleDesiredRotationalEffort - vehicleRotationalEffort);
		if(steeringRate > MAX_STEERING_RATE)
		{
			steeringRate = MAX_STEERING_RATE;
		}
		else if(steeringRate < -MAX_STEERING_RATE)
		{
			steeringRate = -MAX_STEERING_RATE;			
		}
		
		vehicleRotationalEffort += steeringRate * dt; 
		turningCurvature = -vehicleRotationalEffort / 100.0 / MIN_TURNING_RADIUS_M;
		
		vehicleLinearEffortX = vehicleDesiredLinearEffortX;
		motorForce = K_THROTTLE * vehicleLinearEffortX + IDLE_FORCE_N;
		
		vehicleResistiveEffortX += BRAKE_TAU * (vehicleDesiredResistiveEffortX - vehicleResistiveEffortX);
		
		if(vehicleSpeed > 0.005)
		{
			brakeForce = K_DYNAMIC_BRAKE * vehicleResistiveEffortX;
		}
		else
		{
			brakeForce = K_STATIC_BRAKE * vehicleResistiveEffortX;			
			if(brakeForce > motorForce)
			{
				brakeForce = motorForce + 1.0;
			}
		}
		dragForce = DRAG_COEFF_N_PER_MPS_SQ * vehicleSpeed * vehicleSpeed;
		
		
		vehicleSpeed += ( motorForce - brakeForce - dragForce) / VEHICLE_MASS_KG * dt;
		if(vehicleSpeed < 0)
		{
			vehicleSpeed = 0;
		}
		
		deltaH = turningCurvature * vehicleSpeed * dt;
		
		vehiclePosUtm->xMeters += cos(vehicleH + deltaH / 2.0) * vehicleSpeed * dt;
		vehiclePosUtm->yMeters += sin(vehicleH + deltaH / 2.0) * vehicleSpeed * dt;

		vehicleH += deltaH;
		vehicleH = fmod(vehicleH, 2*M_PI);

		if(vehiclePosLla)
		{
			pointLlaDestroy(vehiclePosLla);
		}
		vehiclePosLla = pointUtmToPointLla(vehiclePosUtm);

		ojSleepMsec(25);
	}	
	
	ojSleepMsec(50);	// Sleep for 50 milliseconds and then exit
	
	vehicleSimThreadRunning = FALSE;
	
	return NULL;
}

