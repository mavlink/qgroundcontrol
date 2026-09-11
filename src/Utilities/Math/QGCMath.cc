// MSVC's <cmath> hides M_PI unless _USE_MATH_DEFINES is set before any C runtime header.
#define _USE_MATH_DEFINES

#include "QGCMath.h"

#include <QtCore/QtNumeric>

#include <cmath>
#include <float.h>

#include "CRC32.h"

namespace QGC {

float limitAngleToPMPIf(double angle)
{
    if ((angle > (-20 * M_PI)) && (angle < (20 * M_PI))) {
        while (angle > ((float) M_PI + FLT_EPSILON)) {
            angle -= 2.0f * (float) M_PI;
        }

        while (angle <= -((float) M_PI + FLT_EPSILON)) {
            angle += 2.0f * (float) M_PI;
        }
    } else {
        // Approximate
        angle = fmodf(angle, (float) M_PI);
    }

    return angle;
}

double limitAngleToPMPId(double angle)
{
    if ((angle > (-20 * M_PI)) && (angle < (20 * M_PI))) {
        if (angle < -M_PI) {
            while (angle < -M_PI) {
                angle += 2.0f * M_PI;
            }
        } else if (angle > M_PI) {
            while (angle > M_PI) {
                angle -= 2.0f * M_PI;
            }
        }
    } else {
        // Approximate
        angle = fmod(angle, M_PI);
    }

    return angle;
}

quint32 crc32(const quint8* src, unsigned len, unsigned state)
{
    return crc32Update({src, len}, state);
}

bool fuzzyCompare(double value1, double value2)
{
    if (qIsNaN(value1) && qIsNaN(value2)) {
        return true;
    } else if (qIsNaN(value1) || qIsNaN(value2)) {
        return false;
    } else if (value1 == value2) {
        return true;
    } else {
        return qFuzzyCompare(value1, value2);
    }
}

bool fuzzyCompare(double value1, double value2, double tolerance)
{
    if (qIsNaN(value1) && qIsNaN(value2)) {
        return true;
    } else if (qIsNaN(value1) || qIsNaN(value2)) {
        return false;
    } else {
        return fabs(value1 - value2) <= tolerance;
    }
}

bool fuzzyCompare(float value1, float value2)
{
    if (qIsNaN(value1) && qIsNaN(value2)) {
        return true;
    } else if (qIsNaN(value1) || qIsNaN(value2)) {
        return false;
    } else if (value1 == value2) {
        return true;
    } else {
        return qFuzzyCompare(value1, value2);
    }
}

bool fuzzyCompare(float value1, float value2, float tolerance)
{
    if (qIsNaN(value1) && qIsNaN(value2)) {
        return true;
    } else if (qIsNaN(value1) || qIsNaN(value2)) {
        return false;
    } else {
        return fabsf(value1 - value2) <= tolerance;
    }
}

}  // namespace QGC
