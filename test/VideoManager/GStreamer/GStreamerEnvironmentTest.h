#pragma once

#include "UnitTest.h"

/// GStreamer::Environment's process environment setup.
class GStreamerEnvironmentTest : public UnitTest
{
    Q_OBJECT

private slots:
#ifdef QGC_GST_STREAMING
    void _testPrivateRegistry();
    void _testPrivateRegistryStableInstall_data();
    void _testPrivateRegistryStableInstall();
    void _testPrivateRegistryPrunesUnusedRegistries();
    void _testPrivateRegistryKeepsUserRegistry_data();
    void _testPrivateRegistryKeepsUserRegistry();
    void _testSystemGStreamerKeepsDefaultRegistry();
    void _testBundleLayouts_data();
    void _testBundleLayouts();
    void _testMacOSFrameworkVersionDir();
    void _testBundledRuntime();
    void _testEnvironmentSetup();
#else
    void _testGStreamerDisabled();
#endif
};
