#pragma once

#include "UnitTest.h"

/// Geodesy, receiver time conversions and command decimal text shared by receiver families.
class GPSProtocolMathTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _ecefConversion_data();
    void _ecefConversion();
    void _utcMicroseconds_data();
    void _utcMicroseconds();
    void _towAdvances_data();
    void _towAdvances();
    void _nearEarthSurface();
    void _fixedDecimals_data();
    void _fixedDecimals();
};
