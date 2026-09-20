#pragma once

#include "UnitTest.h"

class NMEASentenceTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _frameValidation_data();
    void _frameValidation();
    void _borrowedBytesAreOwned();
    void _utcMilliseconds_data();
    void _utcMilliseconds();
    void _qtTimestampEquivalence();
    void _makeGga_data();
    void _makeGga();
    void _repairChecksum_data();
    void _repairChecksum();
    void _incrementalFraming_data();
    void _incrementalFraming();
    void _incrementalReset();
};
