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
// File:		vehicle.h
// Version:		0.1
// Written by:	Bob Touchton (btouch@ufl.edu) and Tom Galluzzo (galluzzt@ufl.edu)
// Date:		07/22/2005

#ifndef VEHICLE_H
#define VEHICLE_H

#define VEHICLE_MAX_PHI_RATE_EFFORT_PER_SEC		60.0 // Tuned: 8/13/2005, Roberto and Tom
#define VEHICLE_CURVATURE_PER_EFFORT			0.0016f // Changed from 0.00078f per Tom 8/19/05 Tuned: 8/13/2005, Danny and Tom
#define VEHICLE_MAX_PHI_EFFORT					100.0

typedef struct VehicleState
{
	float xM;
	float yM;
	float yawRad;
	float speedMps;
	float desiredSpeedMps;
	float phiEffort;
	float desiredPhiEffort;	
}* VehicleState;

VehicleState vehicleStateCreate(void);
void vehicleStateDestroy(VehicleState);

void vehicleModelTimeStep(VehicleState, float);
void vehicleModelDistStep(VehicleState, float);

#endif // VEHICLE_H
