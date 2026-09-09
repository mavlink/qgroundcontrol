#pragma once

#include "UnitTest.h"

class GPSReceiverIntegrationTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _passiveAttemptNeverConfigures();
    void _passiveTerminalTransitionIsIdempotent();
};
