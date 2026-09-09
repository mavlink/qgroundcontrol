/****************************************************************************
 *
 *   Copyright (c) 2023 PX4 Development Team. All rights reserved.
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

#include "crc.h"

// According to https://en.wikipedia.org/wiki/Cyclic_redundancy_check
//
// the CRC is based on the CRC-32 ISO 3309 / ANSI X3.66
// implementetation, and the polynomial is supplied as "Reversed".
//
// The CRC algorithm was extracted from the Femtomes driver.
//

static constexpr uint32_t CRC32_POLYNOMIAL = 0xEDB88320;

static uint32_t crc32Value(uint32_t crc)
{
	for (int i = 8 ; i > 0; i--) {
		if (crc & 1) {
			crc = (crc >> 1) ^ CRC32_POLYNOMIAL;

		} else {
			crc >>= 1;
		}
	}

	return crc;
}

uint32_t
calculateCRC32(uint32_t length, uint8_t *buffer, uint32_t crc)
{
	while (length-- != 0) {
		crc = ((crc >> 8) & 0x00FFFFFFL) ^ (crc32Value(((uint32_t) crc ^ *buffer++) & 0xff));
	}

	return crc;
}
