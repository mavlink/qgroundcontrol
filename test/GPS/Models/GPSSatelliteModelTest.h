#pragma once

#include "UnitTest.h"

class GPSSatelliteModelTest : public UnitTest
{
    Q_OBJECT
private slots:
    void _unchangedAndRoleNotifications();
    void _rolesAndUnknownValues();
    void _nmeaConstellationAndUsedIdentity();
    void _freshnessAndSessionIsolation();
    void _reentrantReset();
};
