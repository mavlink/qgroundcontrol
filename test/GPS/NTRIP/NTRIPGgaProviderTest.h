#pragma once

#include "UnitTest.h"

class NTRIPGgaProviderTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testMakeGGA_basicCoordinate();
    void testMakeGGA_negativeCoordinates();
    void testMakeGGA_equator();
    void testMakeGGA_zeroAltitude();
    void testMakeGGA_checksumFormat();
    void testMakeGGA_highAltitude();
};
