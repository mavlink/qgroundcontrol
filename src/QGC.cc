/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2011 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

#include "QGC.h"
#include <qmath.h>
#include <float.h>

namespace QGC
{

quint64 groundTimeUsecs()
{
    QDateTime time = QDateTime::currentDateTime();
    time = time.toUTC();
    /* Return seconds and milliseconds, in milliseconds unit */
    quint64 microseconds = time.toTime_t() * static_cast<quint64>(1000000);
    return static_cast<quint64>(microseconds + (time.time().msec()*1000));
}

quint64 groundTimeMilliseconds()
{
    QDateTime time = QDateTime::currentDateTime();
    time = time.toUTC();
    /* Return seconds and milliseconds, in milliseconds unit */
    quint64 seconds = time.toTime_t() * static_cast<quint64>(1000);
    return static_cast<quint64>(seconds + (time.time().msec()));
}

qreal groundTimeSeconds()
{
    QDateTime time = QDateTime::currentDateTime();
    time = time.toUTC();
    /* Return time in seconds unit */
    quint64 seconds = time.toTime_t();
    return static_cast<qreal>(seconds + (time.time().msec() / 1000.0));
}

float limitAngleToPMPIf(float angle)
{
    if (angle > -20*M_PI && angle < 20*M_PI)
    {
        while (angle > ((float)M_PI+FLT_EPSILON))
        {
            angle -= 2.0f * (float)M_PI;
        }

        while (angle <= -((float)M_PI+FLT_EPSILON))
        {
            angle += 2.0f * (float)M_PI;
        }
    }
    else
    {
        // Approximate
        angle = fmodf(angle, (float)M_PI);
    }

    return angle;
}

double limitAngleToPMPId(double angle)
{
    if (angle > -20*M_PI && angle < 20*M_PI)
    {
        if (angle < -M_PI)
        {
            while (angle < -M_PI)
            {
                angle += M_PI;
            }
        }
        else if (angle > M_PI)
        {
            while (angle > M_PI)
            {
                angle -= M_PI;
            }
        }
    }
    else
    {
        // Approximate
        angle = fmod(angle, M_PI);
    }

    return angle;
}

int applicationVersion()
{
    return APPLICATIONVERSION;
}

}
