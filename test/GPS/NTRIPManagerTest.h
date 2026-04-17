#pragma once

#include "UnitTest.h"

class NTRIPManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    // GGA format and structure
    void _testMakeGGAFormat();
    void _testMakeGGAFieldCount();
    void _testMakeGGAChecksum();

    // GGA hemisphere encoding
    void _testMakeGGANorthEast();
    void _testMakeGGASouthWest();
    void _testMakeGGAEquator();
    void _testMakeGGADateLine();

    // GGA altitude
    void _testMakeGGAZeroAltitude();
    void _testMakeGGAHighAltitude();
    void _testMakeGGANegativeAltitude();

    // GGA coordinate precision
    void _testMakeGGADMMPrecision();

    // GGA time field
    void _testMakeGGATimeFormat();
};
