#pragma once

#include "UnitTest.h"

class VehicleRPMFactGroupTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _validValuesPassThrough();
    void _negativeRpmIsNaN();
    void _zeroRpmStaysZero();
};
