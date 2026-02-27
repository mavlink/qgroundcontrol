#pragma once

#include "UnitTest.h"

/// Unit tests for SDL utility functions (SDLPlatform and SDLJoystick namespaces)
class SDLTest : public UnitTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;
    void init() override;
    void cleanup() override;

    // =========================================================================
    // SDLPlatform Tests
    // =========================================================================

    // Version info tests
    void _versionInfoTest();

    // Platform detection tests
    void _platformDetectionTest();

    // Hint management tests
    void _hintManagementTest();

    // System info tests
    void _systemInfoTest();

    // Power info tests
    void _powerInfoTest();

    // Android-specific tests (run on all platforms, verify graceful behavior)
    void _androidFunctionsTest();

    // =========================================================================
    // SDLJoystick Tests
    // =========================================================================

    // Initialization tests
    void _initializationTest();

    // Event control tests
    void _eventControlTest();

    // Thread safety tests
    void _threadSafetyTest();

    // Type/string conversion tests
    void _gamepadTypeConversionTest();
    void _gamepadAxisConversionTest();
    void _gamepadButtonConversionTest();
    void _connectionStateConversionTest();

    // GUID utilities tests
    void _guidInfoTest();

    // Virtual joystick tests
    void _virtualJoystickTest();

    // Pre-open device query tests
    void _preOpenDeviceQueryTest();

    // Mapping management tests
    void _mappingManagementTest();
};
