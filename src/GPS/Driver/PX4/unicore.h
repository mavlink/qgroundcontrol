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

#pragma once

#include <cstdint>


class UnicoreParser
{
public:
	enum class Result {
		None,
		WrongCrc,
		WrongStructure,
		GotHeading,
		GotAgrica,
		UnknownSentence,
	};

	Result parseChar(char c);

	struct Heading {
		float heading_deg;
		float heading_stddev_deg;
		float baseline_m;
	};

	struct Agrica {
		float velocity_m_s;
		float velocity_north_m_s;
		float velocity_east_m_s;
		float velocity_up_m_s;
		float stddev_velocity_north_m_s;
		float stddev_velocity_east_m_s;
		float stddev_velocity_up_m_s;
	};

	Heading heading() const
	{
		return _heading;
	}

	Agrica agrica() const
	{
		return _agrica;
	}

	bool agricaValid() const { return _agrica_valid; }

private:
	void reset();
	bool crcCorrect() const;
	bool isHeading() const;
	bool isAgrica() const;
	bool extractHeading();
	bool extractAgrica();

	// We have seen buffers with 540 bytes for AGRICA.
	char _buffer[600];
	unsigned _buffer_pos {0};
	char _buffer_crc[9];
	unsigned _buffer_crc_pos {0};

	enum class State {
		Uninit,
		GotHashtag,
		GotStar,
	} _state {State::Uninit};

	Heading _heading{};
	Agrica _agrica{};

	bool _agrica_valid{false};
};