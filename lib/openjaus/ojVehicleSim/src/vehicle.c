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
// File:		vehicle.c 
// Version:		0.4
// Written by:	Tom Galluzzo (galluzzt@ufl.edu)
// Date:		07/22/2005

#include <stdlib.h>	
#include <string.h>	
#include <math.h>

#include "vehicle.h"

#define K_MINIMUM_THRESHOLD				0.001
#define SPEED_MINIMUM_THRESHOLD			0.001

VehicleState vehicleStateCreate(void)
{
	VehicleState state;
	
	state = (VehicleState)malloc( sizeof(struct VehicleState) );
	if(state == NULL)
	{
		return NULL;
	}
	
	memset(state, 0, sizeof(struct VehicleState) );
	
	return state;
}

void vehicleStateDestroy( VehicleState state)
{
	free(state);
}

void vehicleModelTimeStep( VehicleState state, float dTime)
{
	float kAve = state->phiEffort;
	float dPhi = state->desiredPhiEffort - state->phiEffort;
	float dMaxPhi = (float)VEHICLE_MAX_PHI_RATE_EFFORT_PER_SEC * dTime;
	float dDistM = state->speedMps * dTime;
	float dYawRad;
	
	if(dPhi > 0)
	{
		state->phiEffort += dPhi > dMaxPhi? dMaxPhi : dPhi;
	}
	else
	{
		state->phiEffort += dPhi < -dMaxPhi? -dMaxPhi : dPhi;
	}
	
	kAve += state->phiEffort;
	kAve /= 2.0f;
	kAve *= VEHICLE_CURVATURE_PER_EFFORT;
	
	dYawRad = dDistM * kAve;
	
	if(kAve < K_MINIMUM_THRESHOLD && kAve > -K_MINIMUM_THRESHOLD)
	{
		// Project straight line
		state->xM += sinf(state->yawRad) * dDistM; 
		state->yM += cosf(state->yawRad) * dDistM; 
	}
	else
	{
		// Project path along curve
		state->xM += (cosf(state->yawRad)*(1.0f - cosf(dYawRad)) + sinf(state->yawRad)*sinf(dYawRad)) / kAve; 
		state->yM += (cosf(state->yawRad)*sinf(dYawRad) - sinf(state->yawRad)*(1.0f - cosf(dYawRad))) / kAve; 
	}
	
	state->yawRad += dYawRad;
}

void vehicleModelDistStep( VehicleState state, float dDistM)
{
	float kAve;
	float dYawRad;
	
	if(state->speedMps > SPEED_MINIMUM_THRESHOLD)
	{
		vehicleModelTimeStep( state, dDistM / state->speedMps);
	}
	else
	{
		kAve = state->phiEffort;
		state->phiEffort = state->desiredPhiEffort;
		kAve += state->phiEffort;
		kAve /= 2.0f;
		kAve *= VEHICLE_CURVATURE_PER_EFFORT;

		dYawRad = dDistM * kAve;

		if(kAve < K_MINIMUM_THRESHOLD && kAve > -K_MINIMUM_THRESHOLD)
		{
			// Project straight line
			state->xM += sinf(state->yawRad) * dDistM; 
			state->yM += cosf(state->yawRad) * dDistM; 
		}
		else
		{
			// Project path along curve
			state->xM += (cosf(state->yawRad)*(1.0f - cosf(dYawRad)) + sinf(state->yawRad)*sinf(dYawRad)) / kAve; 
			state->yM += (cosf(state->yawRad)*sinf(dYawRad) - sinf(state->yawRad)*(1.0f - cosf(dYawRad))) / kAve; 
		}
		
		state->yawRad += dYawRad;
	}
}
