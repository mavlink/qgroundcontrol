#pragma once

#include "UnitTest.h"

class NTRIPManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    // GGA format and structure
    void testMakeGGAFormat();
    void testMakeGGAFieldCount();
    void testMakeGGAChecksum();

    // GGA hemisphere encoding
    void testMakeGGANorthEast();
    void testMakeGGASouthWest();
    void testMakeGGAEquator();
    void testMakeGGADateLine();

    // GGA altitude
    void testMakeGGAZeroAltitude();
    void testMakeGGAHighAltitude();
    void testMakeGGANegativeAltitude();

    // GGA coordinate precision
    void testMakeGGADMMPrecision();

    // GGA time field
    void testMakeGGATimeFormat();
};
