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
// File Name: jausComponent.h
//
// Written By: Danny Kent (jaus AT dannykent DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file defines all the attributes of a JausComponent and defines all 
// pre-defined Jaus Component Ids according to RA 3.2

#ifndef JAUS_COMPONENT_H
#define JAUS_COMPONENT_H

#include <time.h>

#include "jaus.h"

#define JAUS_DEFAULT_AUTHORITY		0

// Jaus-Defined Component Ids:
#define JAUS_NODE_MANAGER 									 1
#define JAUS_SUBSYSTEM_COMMANDER 							32
#define JAUS_PRIMITIVE_DRIVER 								33
#define JAUS_GLOBAL_VECTOR_DRIVER 							34
#define JAUS_COMMUNICATOR 									35
#define JAUS_MISSION_SPOOLER								36
#define JAUS_VISUAL_SENSOR 									37
#define JAUS_GLOBAL_POSE_SENSOR 							38
#define JAUS_MISSION_PLANNER								39
#define JAUS_SYSTEM_COMMANDER 								40
#define JAUS_LOCAL_POSE_SENSOR 								41
#define JAUS_VELOCITY_STATE_SENSOR 							42
#define JAUS_REFLEXIVE_DRIVER 								43
#define JAUS_LOCAL_VECTOR_DRIVER 							44
#define JAUS_GLOBAL_WAYPOINT_DRIVER 						45
#define JAUS_LOCAL_WAYPOINT_DRIVER 							46
#define JAUS_GLOBAL_PATH_SEGMENT_DRIVER 					47
#define JAUS_LOCAL_PATH_SEGMENT_DRIVER 						48
#define JAUS_PRIMITIVE_MANIPULATOR 							49
#define JAUS_RANGE_SENSOR 									50
#define JAUS_MANIPULATOR_JOINT_POSITION_SENSOR 				51
#define JAUS_MANIPULATOR_JOINT_VELOCITY_SENSOR 				52
#define JAUS_MANIPULATOR_JOINT_FORCE_TORQUE_SENSOR 			53
#define JAUS_MANIPULATOR_JOINT_POSITIONS_DRIVER 			54
#define JAUS_MANIPULATOR_END_EFFECTOR_DRIVER 				55
#define JAUS_MANIPULATOR_JOINT_VELOCITIES_DRIVER 			56
#define JAUS_MANIPULATOR_END_EFFECTOR_VELOCITY_STATE_DRIVER 57
#define JAUS_MANIPULATOR_JOINT_MOVE_DRIVER 					58
#define JAUS_MANIPULATOR_END_EFFECTOR_DISCRETE_POSE_DRIVER 	59
#define JAUS_WORLD_MODEL_VECTOR_KNOWLEDGE_STORE				61

#define COMPONENT_TIMEOUT_SEC 3.0

typedef struct
{
	char *identification;
	JausAddress address;
	JausState state;
	JausByte authority;
	JausNode node;
	JausArray services;
	
	struct
	{
		JausAddress address;
		JausState state;
		JausByte authority;
		JausBoolean active;
	}controller;
	
	time_t timeStampSec;
}JausComponentStruct;

typedef JausComponentStruct *JausComponent;

JAUS_EXPORT JausComponent jausComponentCreate(void);
JAUS_EXPORT void jausComponentDestroy(JausComponent cmpt);
JAUS_EXPORT JausComponent jausComponentClone(JausComponent cmpt);

JAUS_EXPORT char *jausComponentGetTypeString(JausComponent cmpt);

JAUS_EXPORT void jausComponentUpdateTimestamp(JausComponent cmpt);
JAUS_EXPORT JausBoolean jausComponentIsTimedOut(JausComponent cmpt);
JAUS_EXPORT JausBoolean jausComponentHasIdentification(JausComponent cmpt);
JAUS_EXPORT JausBoolean jausComponentHasServices(JausComponent cmpt);

JAUS_EXPORT int jausComponentToString(JausComponent cmpt, char *buf);

#endif // JAUS_COMPONENT_H
