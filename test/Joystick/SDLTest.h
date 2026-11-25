#pragma once

#include "UnitTest.h"

/// Unit tests for SDL utility functions (SDLPlatform and SDLJoystick namespaces)
class SDLTest : public UnitTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;

    // =========================================================================
    // SDLPlatform Tests
    // =========================================================================

    void _versionInfoTest();
    void _platformDetectionTest();
    void _hintManagementTest();
    void _systemInfoTest();
    void _powerInfoTest();
    // Android-specific tests (run on all platforms, verify graceful behavior)
    void _androidFunctionsTest();

    // =========================================================================
    // SDLJoystick Tests
    // =========================================================================

    void _initializationTest();
    void _eventControlTest();
    void _pumpEventsEmptiesEventQueueTest();
    void _threadSafetyTest();
    void _gamepadTypeConversionTest();
    void _gamepadAxisConversionTest();
    void _gamepadButtonConversionTest();
    void _connectionStateConversionTest();
    void _guidInfoTest();
    void _virtualJoystickTest();
    void _preOpenDeviceQueryTest();
    void _playerIndexTest();
    void _mappingManagementTest();
    void _mappingPlatformFilterTest();
};
