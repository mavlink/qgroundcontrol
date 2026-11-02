#pragma once

#include "UnitTest.h"

class UVCDevicesTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testPixelFormatName();
    void _testFormatNamesBestFirst();
    void _testFormatByName();
};
