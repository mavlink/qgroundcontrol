#pragma once

#include "UnitTest.h"

class LogRemoteSinkTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _defaultState();
    void _setProperties();
    void _sendWhenDisabled();
    void _sendBatching();
    void _dropWhenOverMax();
    void _compressionToggle();
    void _protocolChange();
    void _resetBytesSent();
    void _vehicleIdInPayload();
};
