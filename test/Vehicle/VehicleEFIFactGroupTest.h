#pragma once

#include "UnitTest.h"

class VehicleEFIFactGroupTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _validValuesPassThrough();
    void _unavailableValuesAreNaN_data();
    void _unavailableValuesAreNaN();
    void _zeroReadingsStayZero();
};
