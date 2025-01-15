/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QtTypes>

namespace QGC {

float limitAngleToPMPIf(double angle);

double limitAngleToPMPId(double angle);

/// Returns true if the two values are equal or close. Correctly handles 0 and NaN values.
bool fuzzyCompare(double value1, double value2);

quint32 crc32(const quint8 *src, unsigned len, unsigned state);

}
