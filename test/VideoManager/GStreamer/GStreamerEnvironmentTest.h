#pragma once

#include "UnitTest.h"

/// GStreamer::Environment's process environment setup.
class GStreamerEnvironmentTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testPrivateRegistry();
    void _testPrivateRegistryStableInstall_data();
    void _testPrivateRegistryStableInstall();
    void _testPrivateRegistryPrunesUnusedRegistries();
    void _testPrivateRegistryKeepsUserRegistry_data();
    void _testPrivateRegistryKeepsUserRegistry();
    void _testSystemGStreamerKeepsDefaultRegistry();
    void _testBundledRuntime();
};
