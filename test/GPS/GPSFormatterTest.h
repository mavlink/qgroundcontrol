#pragma once
#include "UnitTest.h"

class GPSFormatterTest : public UnitTest
{
    Q_OBJECT
private slots:
    void testFormatDuration();
    void testFormatDataSize();
    void testFormatDataRate();
    void testFormatAccuracy();
    void testFormatCoordinate();
    void testFormatHeading();
    void testFixTypeQuality();
    void testFixTypeColor();
};
