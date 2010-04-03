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
#ifndef VEHICLE_SIM_H
#define VEHICLE_SIM_H

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#define VEHICLE_SIM_LOAD_CONFIGURATION_ERROR	-1
#define VEHICLE_SIM_MALLOC_ERROR				-2
#define VEHICLE_SIM_THREAD_CREATE_ERROR			-4

#define VEHICLE_SIM_THREAD_TIMEOUT_SEC			1.0

#define VEHICLE_SIM_UNDEFINED_STATE		-1
#define VEHICLE_SIM_UNKNOWN_STATE		-1
#define VEHICLE_SIM_INVALID_STATE		-1
#define VEHICLE_SIM_MANUAL_STATE		0
#define VEHICLE_SIM_HOMING_STATE		1
#define VEHICLE_SIM_READY_STATE			2
#define VEHICLE_SIM_SHUTDOWN_STATE		3

#define VEHICLE_SIM_RUN		0
#define VEHICLE_SIM_PAUSE	1

#define VEHICLE_SIM_AUTO_STATE	1

#include "utm/pointLla.h"
#include "utm/pointUtm.h"
#include "utm/utmLib.h"

int vehicleSimStartup(void);
int vehicleSimShutdown(void);

double vehicleSimGetUpdateRate(void);
int vehicleSimGetState(void);

PointLla vehicleSimGetPositionLla(void);
double vehicleSimGetX(void);
double vehicleSimGetY(void);
double vehicleSimGetH(void);
double vehicleSimGetSpeed(void);

double vehicleSimGetRotateEffort(void);

int vehicleSimGetRunPause(void);
void vehicleSimToggleRunPause(void);
void vehicleSimSetCommand(double, double, double);
double vehicleSimGetLinearEffortX(void);
double vehicleSimGetResistiveEffortX(void);
double vehicleSimGetRotationalEffort(void);

#endif // VEHICLE_SIM_H
