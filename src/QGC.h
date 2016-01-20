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

#ifndef QGC_H
#define QGC_H

#include <QDateTime>
#include <QColor>
#include <QThread>

#include "QGCConfig.h"


/* Windows fixes */
#ifdef _MSC_VER
#if (_MSC_VER < 1800)	/* only PRIOR to Visual Studio 2013 */
/* Needed define for Eigen */
//#define NOMINMAX
#include <limits>
template<typename T>
inline bool isnan(T value)
{
	return value != value;

}

// requires #include <limits>
template<typename T>
inline bool isinf(T value)
{
	return (value == std::numeric_limits<T>::infinity() || (-1 * value) == std::numeric_limits<T>::infinity()) && std::numeric_limits<T>::has_infinity;
}
#endif
#elif defined __APPLE__
#include <cmath>
#ifndef isnan
#define isnan(x) std::isnan(x)
#endif
#ifndef isinf
#define isinf(x) std::isinf(x)
#endif
#endif
#ifdef __android__
#define isinf(x) std::isinf(x)
#endif

namespace QGC
{
extern const int defaultSystemId;
extern const int defaultComponentId;

extern const QColor colorCyan;
extern const QColor colorRed;
extern const QColor colorGreen;
extern const QColor colorYellow;
extern const QColor colorOrange;
extern const QColor colorMagenta;
extern const QColor colorDarkWhite;
extern const QColor colorDarkYellow;
extern const QColor colorBackground;
extern const QColor colorBlack;

/**
 * @brief Get the current ground time in microseconds.
 * @note This does not have microsecond precision, it is limited to millisecond precision.
 */
quint64 groundTimeUsecs();
/** @brief Get the current ground time in milliseconds */
quint64 groundTimeMilliseconds();
/** 
 * @brief Get the current ground time in fractional seconds
 * @note Precision is limited to milliseconds.
 */
qreal groundTimeSeconds();
/** @brief Returns the angle limited to -pi - pi */
float limitAngleToPMPIf(float angle);
/** @brief Returns the angle limited to -pi - pi */
double limitAngleToPMPId(double angle);

const static int MAX_FLIGHT_TIME = 60 * 60 * 24 * 21;

class SLEEP : public QThread
{
public:
    /**
     * @brief Set a thread to sleep for seconds
     * @param s time in seconds to sleep
     **/
    static void sleep(unsigned long s) {
        QThread::sleep(s);
    }
    /**
     * @brief Set a thread to sleep for milliseconds
     * @param ms time in milliseconds to sleep
     **/
    static void msleep(unsigned long ms) {
        QThread::msleep(ms);
    }
    /**
     * @brief Set a thread to sleep for microseconds
     * @param us time in microseconds to sleep
     **/
    static void usleep(unsigned long us) {
        QThread::usleep(us);
    }
};

quint32 crc32(const quint8 *src, unsigned len, unsigned state);

}

#define QGC_EVENTLOOP_DEBUG 0

#endif // QGC_H
