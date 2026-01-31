#pragma once

#include "UnitTest.h"

class GeoTagDataTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _captureResultEnumTest();
    void _isValidTest();
    void _defaultValuesTest();
};
