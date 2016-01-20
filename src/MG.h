/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Helper functions
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */


#ifndef _MG_H_
#define _MG_H_

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QThread>
#include <cmath>

namespace MG
{

class TIME
{

public:

    //static const QString ICONDIR = "./icons";

    /**
     * @brief Convenience method to get the milliseconds time stamp for now
     *
     * The timestamp is created at the instant of calling this method. It is
     * defined as the number of milliseconds since unix epoch, which is
     * 1.1.1970, 00:00 UTC.
     *
     * @return The number of milliseconds elapsed since unix epoch
     * @deprecated Will the replaced by time helper class
     **/
    static quint64 getGroundTimeNow() {
        QDateTime time = QDateTime::currentDateTime();
        time = time.toUTC();
        /* Return seconds and milliseconds, in milliseconds unit */
        quint64 milliseconds = time.toTime_t() * static_cast<quint64>(1000);
        return static_cast<quint64>(milliseconds + time.time().msec());
    }

    /**
     * @brief Convenience method to get the milliseconds time stamp for now
     *
     * The timestamp is created at the instant of calling this method. It is
     * defined as the number of milliseconds since unix epoch, which is
     * 1.1.1970, 00:00 UTC.
     *
     * @return The number of milliseconds elapsed since unix epoch
     * @deprecated Will the replaced by time helper class
     **/
    static quint64 getGroundTimeNowUsecs() {
        QDateTime time = QDateTime::currentDateTime();
        time = time.toUTC();
        /* Return seconds and milliseconds, in milliseconds unit */
        quint64 microseconds = time.toTime_t() * static_cast<quint64>(1000000);
        return static_cast<quint64>(microseconds + (time.time().msec()*1000));
    }

    /*tatic quint64 getMissionTimeUsecs()
    {
        ;
    }*/

    /**
     * Convert milliseconds to an QDateTime object. This method converts the amount of
     * milliseconds since 1.1.1970, 00:00 UTC (unix epoch) to a QDateTime date object.
     *
     * @param msecs The milliseconds since unix epoch (in Qt unsigned 64bit integer type quint64)
     * @return The QDateTime object set to corresponding date and time
     * @deprecated Will the replaced by time helper class
     **/
    static QDateTime msecToQDateTime(quint64 msecs) {
        QDateTime time = QDateTime();
        /* Set date and time depending on the seconds since unix epoch,
             * integer division truncates the milliseconds */
        time.setTime_t(msecs / 1000);
        /* Add the milliseconds, modulo returns the milliseconds part */
        return time.addMSecs(msecs % 1000);
    }

};
}

#endif // _MG_H_
