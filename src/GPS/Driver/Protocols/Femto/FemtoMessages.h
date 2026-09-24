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

#define FEMTO_MSG_ID_GPGGA 218

/*** femtomes protocol binary message definitions ***/

namespace Femto {
inline constexpr size_t HEADER_SIZE = 28;
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
