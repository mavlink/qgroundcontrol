#pragma once

#include "UnitTest.h"

class QGroundControlQmlGlobalTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testPositionManagerAccessor();
    void _testFlightMapPositionValidation();
    void _testFlightMapZoomValidation();
};
