#pragma once
#include "UnitTest.h"

class NMEADecoderSessionTest : public UnitTest
{
    Q_OBJECT
private slots:
    void _decoderSessionRestart();
    void _fixLossInvalidatesHealth();
    void _activityAndExpiry();
    void _closeRetiresHealth();
    void _ordinaryDeviceUsesSessionClock();
    void _delayedInputRetainsReceiptAge();
};
