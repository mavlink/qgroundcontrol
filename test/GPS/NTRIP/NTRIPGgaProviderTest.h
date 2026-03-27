#pragma once

#include "UnitTest.h"

class NTRIPGgaProviderTest : public UnitTest
{
    Q_OBJECT

private slots:
    // Format / structure
    void testMakeGGA_basicCoordinate();
    void testMakeGGA_checksumFormat();
    void testMakeGGA_fieldCount();
    void testMakeGGA_timeFormat();

    // Hemisphere encoding
    void testMakeGGA_negativeCoordinates();
    void testMakeGGA_equator();
    void testMakeGGA_dateLine();

    // Altitude edge cases
    void testMakeGGA_zeroAltitude();
    void testMakeGGA_highAltitude();
    void testMakeGGA_negativeAltitude();

    // Coordinate precision (DDMM.mmmm)
    void testMakeGGA_dmmPrecision();
};
