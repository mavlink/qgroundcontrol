#pragma once

#include "UnitTest.h"

/// GStreamer::Environment's process environment setup.
class GStreamerEnvironmentTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testPrivateRegistry();
    void _testPrivateRegistryKeepsUserRegistry_data();
    void _testPrivateRegistryKeepsUserRegistry();
    void _testSystemGStreamerKeepsDefaultRegistry();
};
