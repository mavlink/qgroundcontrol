/****************************************************************************
 *
 *   Copyright (c) 2023-2024 PX4 Development Team. All rights reserved.
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

#include "unicore.h"
#include "crc.h"
#include <cstdio>
#include <stdlib.h>
#include <string.h>

UnicoreParser::Result UnicoreParser::parseChar(char c)
{
	switch (_state) {
	case State::Uninit:
		if (c == '#') {
			_state = State::GotHashtag;
		}

		break;

	case State::GotHashtag:
		if (c == '*') {
			_state = State::GotStar;

			// Make sure buffer is zero terminated.
			_buffer[_buffer_pos] = '\0';

		} else {
			if (_buffer_pos >= sizeof(_buffer) - 1) {
				reset();
				return Result::None;
			}

			_buffer[_buffer_pos++] = c;
		}

		break;

	case State::GotStar:
		_buffer_crc[_buffer_crc_pos++] = c;

		if (_buffer_crc_pos >= 8) {

			// Make sure the CRC buffer is zero terminated.
			_buffer_crc[_buffer_crc_pos] = '\0';

			if (!crcCorrect()) {
				reset();
				return Result::WrongCrc;
			}

			if (isHeading()) {
				if (extractHeading()) {
					reset();
					return Result::GotHeading;

				} else {
					reset();
					return Result::WrongStructure;
				}

			} else if (isAgrica()) {
				if (extractAgrica()) {
					reset();
					return Result::GotAgrica;

				} else {
					reset();
					return Result::WrongStructure;
				}

			} else {
				reset();
				return Result::UnknownSentence;
			}
		}

		break;
	}

	return Result::None;
}

void UnicoreParser::reset()
{
	_state = State::Uninit;
	_buffer_pos = 0;
	_buffer_crc_pos = 0;
}

bool UnicoreParser::crcCorrect() const
{
	const uint32_t crc_calculated = calculateCRC32(_buffer_pos, (uint8_t *)_buffer, 0);
	const uint32_t crc_parsed = (uint32_t)strtoul(_buffer_crc, nullptr, 16);
	return (crc_calculated == crc_parsed);
}

bool UnicoreParser::isHeading() const
{
	const char header[] = "UNIHEADINGA";

	return strncmp(header, _buffer, strlen(header)) == 0;
}

bool UnicoreParser::isAgrica() const
{
	const char header[] = "AGRICA";

	return strncmp(header, _buffer, strlen(header)) == 0;
}

bool UnicoreParser::extractHeading()
{
	// The basline starts after ;,, and then follows the heading.

	// Skip to ;
	char *ptr = strchr(_buffer, ';');

	if (ptr == nullptr) {
		return false;
	}

	ptr = strtok(ptr, ",");

	unsigned i = 0;

	while (ptr != nullptr) {
		ptr = strtok(nullptr, ",");

		if (ptr == nullptr) {
			return false;
		}

		if (i == 1) {
			_heading.baseline_m = strtof(ptr, nullptr);

		} else if (i == 2) {
			_heading.heading_deg = strtof(ptr, nullptr);

		} else if (i == 5) {
			_heading.heading_stddev_deg = strtof(ptr, nullptr);
			return true;
		}

		++i;
	}

	return false;
}

bool UnicoreParser::extractAgrica()
{
	// Skip to ;
	char *ptr = strchr(_buffer, ';');

	if (ptr == nullptr) {
		return false;
	}

	ptr = strtok(ptr, ",");

	unsigned i = 0;

	while (ptr != nullptr) {
		ptr = strtok(nullptr, ",");

		if (ptr == nullptr) {
			return false;
		}

		if (i == 21) {
			_agrica.velocity_m_s = strtof(ptr, nullptr);
		}

		else if (i == 22) {
			_agrica.velocity_north_m_s = strtof(ptr, nullptr);
		}

		else if (i == 23) {
			_agrica.velocity_east_m_s = strtof(ptr, nullptr);
		}

		else if (i == 24) {
			_agrica.velocity_up_m_s = strtof(ptr, nullptr);
		}

		else if (i == 25) {
			_agrica.stddev_velocity_north_m_s = strtof(ptr, nullptr);
		}

		else if (i == 26) {
			_agrica.stddev_velocity_east_m_s = strtof(ptr, nullptr);
		}

		else if (i == 27) {
			_agrica.stddev_velocity_up_m_s = strtof(ptr, nullptr);
			_agrica_valid = true;
			return true;
		}

		++i;
	}

	return false;
}