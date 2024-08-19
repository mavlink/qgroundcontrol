#pragma once

#include "UnitTest.h"

class ADSBTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _adsbVehicleTest();
    void _adsbTcpLinkTest();
    void _adsbVehicleManagerTest();
};
