#pragma once

#include <QtCore/QtTypes>

namespace QGC
{
    float limitAngleToPMPIf(double angle);
    double limitAngleToPMPId(double angle);

    /// Returns true if the two values are equal or close. Correctly handles 0 and NaN values.
    bool fuzzyCompare(double value1, double value2);
    bool fuzzyCompare(float value1, float value2);
    bool fuzzyCompare(double value1, double value2, double tolerance);
    bool fuzzyCompare(float value1, float value2, float tolerance);

    quint32 crc32(const quint8 *src, unsigned len, unsigned state);
}
