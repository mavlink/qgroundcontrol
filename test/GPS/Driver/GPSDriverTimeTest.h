#pragma once

#include "UnitTest.h"

class GPSDriverTimeTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _convertsUtc_data();
    void _convertsUtc();
    void _normalizesGpsWeek();
    void _rejectsInvalidEpoch();
};
