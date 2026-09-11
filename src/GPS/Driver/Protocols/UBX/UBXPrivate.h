/****************************************************************************
 *
 *   Copyright (c) 2012-2023 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#pragma once

#include <cmath>
#include <string.h>

#include "RTCMFramer.h"
#include "UBX/GPSDriverUBX.h"

// RTCM3 message sets for a base: the station/bias messages plus GPS, GLONASS, Galileo and BeiDou
// observations as MSM4 or MSM7 (1074/1084/1094/1124 vs 1077/1087/1097/1127)
static constexpr uint32_t RTCM_BASE_MSGOUT_I2C[] = {
    UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1005_I2C, UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1077_I2C,
    UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1087_I2C, UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1230_I2C,
    UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1097_I2C, UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1127_I2C};
