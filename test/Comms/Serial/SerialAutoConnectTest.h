#pragma once

#include "UnitTest.h"

class SerialAutoConnectTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _compositeSelection_data();
    void _compositeSelection();
    void _discoveryDeadlineAndRetryState();
    void _replacementDeviceResetsConfiguration();
};
