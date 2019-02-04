/****************************************************************************
 *
 *   Copyright (c) 2016 PX4 Development Team. All rights reserved.
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

/**
 * @file definitions.h
 * common platform-specific definitions & abstractions for gps
 * @author Beat Küng <beat-kueng@gmx.net>
 */

#pragma once

#include <QtGlobal>

#define GPS_READ_BUFFER_SIZE 1024

#define GPS_INFO(...) qInfo(__VA_ARGS__)
#define GPS_WARN(...) qWarning(__VA_ARGS__)
#define GPS_ERR(...) qCritical(__VA_ARGS__)

#include "vehicle_gps_position.h"
#include "satellite_info.h"

#define M_DEG_TO_RAD 		(M_PI / 180.0)
#define M_RAD_TO_DEG 		(180.0 / M_PI)
#define M_DEG_TO_RAD_F 		0.01745329251994f
#define M_RAD_TO_DEG_F 		57.2957795130823f

#include <QThread>

class Sleeper : public QThread
{
public:
    static void usleep(unsigned long usecs) { QThread::usleep(usecs); }
};

static inline void gps_usleep(unsigned long usecs) {
    Sleeper::usleep(usecs);
}

typedef uint64_t gps_abstime;

#include <QDateTime>
/**
 * Get the current time in us. Function signature:
 * uint64_t hrt_absolute_time()
 */
static inline gps_abstime gps_absolute_time() {
    //FIXME: is there something with microsecond accuracy?
    return QDateTime::currentMSecsSinceEpoch() * 1000;
}

//timespec is UNIX-specific
#ifdef _WIN32
#if _MSC_VER < 1900
struct timespec
{
    time_t tv_sec;
    long tv_nsec;
};
#else
#include <time.h>
#endif
#endif

