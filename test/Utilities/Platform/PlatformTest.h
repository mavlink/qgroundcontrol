#pragma once

#include "UnitTest.h"

/// Tests for Platform utilities (safety checks, environment setup)
class PlatformTest : public UnitTest
{
    Q_OBJECT

private slots:
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    void _testCheckSingleInstanceWhileLockIsHeld();
#endif

#if defined(QGC_UNITTEST_BUILD)
    // Unit-test initialization environment setup
    void _testInitializeSetsUnitTestEnvironment();
#endif

    // GraphicsSetup configures the first QQuickWindow when its native surface is created
    void _testGraphicsSetupConfiguresFirstWindowOnCreation();

#if defined(QGC_UNITTEST_BUILD) && defined(Q_OS_WIN)
    void _testInitializeSwRastRequestsSoftwareDevice();
#endif
};
