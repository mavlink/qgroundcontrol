#pragma once

#include "UnitTest.h"

class VehicleGPSFactGroupTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _rawNormalization_data();
    void _rawNormalization();
    void _positionReported_data();
    void _positionReported();
    void _highLatency2KeepsReceiverState();
    void _receiverDispatch_data();
    void _receiverDispatch();
    void _rtkStatus_data();
    void _rtkStatus();
    void _systemErrorText_data();
    void _systemErrorText();
    void _resilienceSummary_data();
    void _resilienceSummary();
    void _integrityExpires();
    void _schedulerDestruction();
};
