#pragma once

#include "UnitTest.h"

class ADSBTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _adsbVehicleTest();
    void _adsbTcpLinkTest();
    void _adsbTcpLinkRejectsNullHostTest();
    void _adsbTcpLinkIgnoresInvalidMessagesTest();
    void _adsbTcpLinkCallsignMessageTest();
    void _adsbVehicleManagerTest();
};
