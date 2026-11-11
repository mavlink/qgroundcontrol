#include "QGCMath.h"

#include <cmath>
#include <float.h>
#include <numbers>

#include <QtCore/QtNumeric>

#include "CRC32.h"

namespace QGC {

float limitAngleToPMPIf(double angle)
{
    if ((angle > (-20 * std::numbers::pi)) && (angle < (20 * std::numbers::pi))) {
        while (angle > (std::numbers::pi_v<float> + FLT_EPSILON)) {
            angle -= 2.0f * std::numbers::pi_v<float>;
        }

        while (angle <= -(std::numbers::pi_v<float> + FLT_EPSILON)) {
            angle += 2.0f * std::numbers::pi_v<float>;
        }
    } else {
        // Too many turns for the loops above
        angle = std::remainder(angle, 2.0 * std::numbers::pi);
    }

    return angle;
}

double limitAngleToPMPId(double angle)
{
    if ((angle > (-20 * std::numbers::pi)) && (angle < (20 * std::numbers::pi))) {
        if (angle < -std::numbers::pi) {
            while (angle < -std::numbers::pi) {
                angle += 2.0f * std::numbers::pi;
            }
        } else if (angle > std::numbers::pi) {
            while (angle > std::numbers::pi) {
                angle -= 2.0f * std::numbers::pi;
            }
        }
    } else {
        // Too many turns for the loops above
        angle = std::remainder(angle, 2.0 * std::numbers::pi);
    }

    return angle;
}

quint32 crc32(const quint8 *src, unsigned len, unsigned state)
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
    } else if (qIsInf(value1) || qIsInf(value2)) {
        return false;
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
    } else if (qIsInf(value1) || qIsInf(value2)) {
        return value1 == value2;
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
    } else if (qIsInf(value1) || qIsInf(value2)) {
        return false;
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
    } else if (qIsInf(value1) || qIsInf(value2)) {
        return value1 == value2;
    } else {
        return fabsf(value1 - value2) <= tolerance;
    }
}

}
