/****************************************************************************
 *
 *   Copyright (c) 2012-2014 PX4 Development Team. All rights reserved.
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
#include <stdint.h>

/*
 * This file is auto-generated from https://github.com/PX4/Firmware/blob/master/msg/satellite_info.msg
 * and was manually copied here.
 */

struct satellite_info_s {
    uint64_t timestamp;         // time since system start (microseconds)
    static constexpr uint8_t SAT_INFO_MAX_SATELLITES = 20;

    uint8_t count;              // Number of satellites visible to the receiver
    uint8_t svid[20];           // Space vehicle ID [1..255], see scheme below
    uint8_t used[20];           // 0: Satellite not used, 1: used for navigation
    uint8_t elevation[20];      // Elevation (0: right on top of receiver, 90: on the horizon) of satellite
    uint8_t azimuth[20];        // Direction of satellite, 0: 0 deg, 255: 360 deg.
    uint8_t snr[20];            // dBHz, Signal to noise ratio of satellite C/N0, range 0..99, zero when not tracking this satellite.
    uint8_t prn[20];            // Satellite PRN code assignment, (psuedorandom number SBAS, valid codes are 120-144)
};
