/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QDateTime>
#include <QColor>
#include <QThread>

#include "QGCConfig.h"

namespace QGC
{

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
float limitAngleToPMPIf(double angle);
/** @brief Returns the angle limited to -pi - pi */
double limitAngleToPMPId(double angle);

/** @brief Records boot time (called from main) */
void initTimer();
/** @brief Get the ground time since boot in milliseconds */
quint64 bootTimeMilliseconds();

/// Returns true if the two values are equal or close. Correctly handles 0 and NaN values.
bool fuzzyCompare(double value1, double value2);

class SLEEP : public QThread
{
    Q_OBJECT
public:
    using QThread::sleep;
    using QThread::msleep;
    using QThread::usleep;
};

quint32 crc32(const quint8 *src, unsigned len, unsigned state);

}
