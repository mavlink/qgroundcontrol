/****************************************************************************
 *
 *   Copyright (c) 2012-2018 PX4 Development Team. All rights reserved.
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
#include <cstddef>
#include <cstdint>

#define FEMTO_MSG_ID_UAVGPS 8001
#define FEMTO_MSG_ID_GPGGA 218
#define FEMTO_MSG_ID_UAVSTATUS 8017

/*** femtomes protocol binary message and payload definitions ***/

typedef struct
{
    uint64_t time_utc_usec; /** Timestamp (microseconds, UTC), this is the timestamp which comes from the gps module. It
                               might be unavailable right after cold start, indicated by a value of 0*/
    int32_t lat;            /** Latitude in 1E-7 degrees*/
    int32_t lon;            /** Longitude in 1E-7 degrees*/
    int32_t alt;            /** Altitude in 1E-3 meters above MSL, (millimetres)*/
    int32_t alt_ellipsoid;  /** Altitude in 1E-3 meters bove Ellipsoid, (millimetres)*/
    float eph;              /** GPS horizontal position accuracy (metres)*/
    float epv;              /** GPS vertical position accuracy (metres)*/
    float hdop;             /** Horizontal dilution of precision*/
    float vdop;             /** Vertical dilution of precision*/
    int32_t noise_per_ms;   /** GPS noise per millisecond*/
    int32_t jamming_indicator; /** indicates jamming is occurring*/
    float vel_m_s;             /** GPS ground speed, (metres/sec)*/
    float cog_rad;             /** Course over ground (NOT heading, but direction of movement), -PI..PI, (radians)*/
    float heading;    /** heading angle of XYZ body frame rel to NED. Set to NaN if not available and updated (used for
                         dual antenna GPS), (rad, [-PI, PI])*/
    uint8_t fix_type; /** 0-1: no fix, 2: 2D fix, 3: 3D fix, 4: RTCM code differential, 5: Real-Time Kinematic, float,
                         6: Real-Time Kinematic, fixed, 8: Extrapolated. Some applications will not use the value of
                         this field unless it is at least two, so always correctly fill in the fix.*/
    uint8_t velocityValid;   /** True if NED velocity is valid*/
    uint8_t satellites_used; /** Number of satellites used*/
    uint8_t heading_type;    /**< 0 invalid,5 for float,6 for fix*/
} femto_uav_gps_t;

namespace Femto {
inline constexpr size_t HEADER_SIZE = 28;
inline constexpr size_t GPS_PAYLOAD_SIZE = 88;
inline constexpr size_t STATUS_HEADER_SIZE = 40;
}  // namespace Femto

struct femto_msg_t
{
    uint8_t data[600]{};
    uint32_t crc = 0;
    uint8_t header[Femto::HEADER_SIZE]{};
    uint16_t messageId = 0;
    uint16_t payloadLength = 0;
    uint16_t read = 0;
};

/*** END OF femtomes protocol binary message and payload definitions ***/

enum class FemtoDecodeState
{
    pream_ble1,  /**< Frame header preamble first byte 0xaa */
    pream_ble2,  /**< Frame header preamble second byte 0x44 */
    pream_ble3,  /**< Frame header preamble third byte 0x12 */
    head_length, /**< Frame header length */
    head_data,   /**< Frame header data */
    data,        /**< Frame data */
    crc1,        /**< Frame crc1 */
    crc2,        /**< Frame crc2 */
    crc3,        /**< Frame crc3 */
    crc4,        /**< Frame crc4 */
};
