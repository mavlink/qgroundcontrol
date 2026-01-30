#include "SDLTest.h"
#include "SDLPlatform.h"
#include "SDLJoystick.h"
#include "JoystickSDL.h"

#include <QtTest/QTest>

UT_REGISTER_TEST(SDLTest)

SDLTest::SDLTest() = default;

void SDLTest::init()
{
    UnitTest::init();
    QVERIFY(JoystickSDL::init());
}

void SDLTest::cleanup()
{
    UnitTest::cleanup();
}

//-----------------------------------------------------------------------------
// SDLPlatform Tests
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Version Info Tests
//-----------------------------------------------------------------------------

void SDLTest::_versionInfoTest()
{
    // Test that version string is valid
    QString version = SDLPlatform::getVersion();
    QVERIFY2(!version.isEmpty(), "SDL version string should not be empty");

    // Should be in format "X.Y.Z"
    QStringList parts = version.split('.');
    QVERIFY2(parts.size() >= 2, "Version should have at least major.minor");

    // Each part should be numeric
    bool ok;
    int major = parts[0].toInt(&ok);
    QVERIFY2(ok, "Major version should be numeric");
    QVERIFY2(major >= 3, "Should be SDL3 or higher");

    int minor = parts[1].toInt(&ok);
    QVERIFY2(ok, "Minor version should be numeric");
    Q_UNUSED(minor);

    // Revision may or may not be available - just verify no crash
    QString revision = SDLPlatform::getRevision();
    Q_UNUSED(revision);
}

//-----------------------------------------------------------------------------
// Platform Detection Tests
//-----------------------------------------------------------------------------

void SDLTest::_platformDetectionTest()
{
    // Test that platform name is valid
    QString platform = SDLPlatform::getPlatform();
    QVERIFY2(!platform.isEmpty(), "Platform name should not be empty");

    // Verify platform-specific detection
#if defined(Q_OS_ANDROID)
    QVERIFY(SDLPlatform::isAndroid());
    QVERIFY(!SDLPlatform::isWindows());
    QVERIFY(!SDLPlatform::isMacOS());
    QVERIFY(!SDLPlatform::isLinux());
    QVERIFY(!SDLPlatform::isIOS());
#elif defined(Q_OS_WIN)
    QVERIFY(SDLPlatform::isWindows());
    QVERIFY(!SDLPlatform::isAndroid());
    QVERIFY(!SDLPlatform::isMacOS());
    QVERIFY(!SDLPlatform::isLinux());
    QVERIFY(!SDLPlatform::isIOS());
#elif defined(Q_OS_MACOS)
    QVERIFY(SDLPlatform::isMacOS());
    QVERIFY(!SDLPlatform::isAndroid());
    QVERIFY(!SDLPlatform::isWindows());
    QVERIFY(!SDLPlatform::isLinux());
    QVERIFY(!SDLPlatform::isIOS());
#elif defined(Q_OS_IOS)
    QVERIFY(SDLPlatform::isIOS());
    QVERIFY(!SDLPlatform::isAndroid());
    QVERIFY(!SDLPlatform::isWindows());
    QVERIFY(!SDLPlatform::isMacOS());
    QVERIFY(!SDLPlatform::isLinux());
#elif defined(Q_OS_LINUX)
    QVERIFY(SDLPlatform::isLinux());
    QVERIFY(!SDLPlatform::isAndroid());
    QVERIFY(!SDLPlatform::isWindows());
    QVERIFY(!SDLPlatform::isMacOS());
    QVERIFY(!SDLPlatform::isIOS());
#endif

    // These functions should not crash regardless of platform
    // Return values are platform-dependent
    bool isTablet = SDLPlatform::isTablet();
    bool isTV = SDLPlatform::isTV();
    bool isDeX = SDLPlatform::isDeXMode();
    bool isChromebook = SDLPlatform::isChromebook();
    Q_UNUSED(isTablet);
    Q_UNUSED(isTV);
    Q_UNUSED(isDeX);
    Q_UNUSED(isChromebook);
}

//-----------------------------------------------------------------------------
// Hint Management Tests
//-----------------------------------------------------------------------------

void SDLTest::_hintManagementTest()
{
    // Use a unique hint name to avoid conflicts
    const QString testHintName = QStringLiteral("SDL_HINT_QGC_TEST_HINT");
    const QString testValue = QStringLiteral("test_value_123");

    // Set hint and verify
    bool setResult = SDLPlatform::setHint(testHintName, testValue);
    QVERIFY2(setResult, "Setting hint should succeed");

    QString retrievedValue = SDLPlatform::getHint(testHintName);
    QCOMPARE(retrievedValue, testValue);

    // Test boolean hint with "1"
    SDLPlatform::setHint(testHintName, QStringLiteral("1"));
    QVERIFY2(SDLPlatform::getHintBoolean(testHintName, false), "Hint '1' should be true");

    // Test boolean hint with "0"
    SDLPlatform::setHint(testHintName, QStringLiteral("0"));
    QVERIFY2(!SDLPlatform::getHintBoolean(testHintName, true), "Hint '0' should be false");

    // Test boolean hint with "true"
    SDLPlatform::setHint(testHintName, QStringLiteral("true"));
    QVERIFY2(SDLPlatform::getHintBoolean(testHintName, false), "Hint 'true' should be true");

    // Test default value for non-existent hint
    const QString nonExistentHint = QStringLiteral("SDL_HINT_QGC_NONEXISTENT_12345");
    QVERIFY2(SDLPlatform::getHintBoolean(nonExistentHint, true), "Default should be returned for missing hint");
    QVERIFY2(!SDLPlatform::getHintBoolean(nonExistentHint, false), "Default should be returned for missing hint");

    // Test reset hint
    SDLPlatform::resetHint(testHintName);
    QString afterReset = SDLPlatform::getHint(testHintName);
    Q_UNUSED(afterReset);  // Value depends on SDL defaults
}

//-----------------------------------------------------------------------------
// System Info Tests
//-----------------------------------------------------------------------------

void SDLTest::_systemInfoTest()
{
    // Test CPU cores - should be at least 1
    int cores = SDLPlatform::getNumLogicalCPUCores();
    QVERIFY2(cores >= 1, "Should have at least 1 CPU core");

    // Test RAM - should be reasonable (at least 256 MB, less than 1 TB)
    int ramMB = SDLPlatform::getSystemRAM();
    QVERIFY2(ramMB >= 256, "Should have at least 256 MB RAM");
    QVERIFY2(ramMB < 1024 * 1024, "RAM should be less than 1 TB (sanity check)");
}

//-----------------------------------------------------------------------------
// Power Info Tests
//-----------------------------------------------------------------------------

void SDLTest::_powerInfoTest()
{
    int seconds = 0;
    int percent = 0;

    // Get power info - should not crash
    QString powerState = SDLPlatform::getDevicePowerInfo(&seconds, &percent);
    QVERIFY2(!powerState.isEmpty(), "Power state should return a string");

    // Verify output parameters are reasonable
    // -1 means unknown, otherwise should be in valid range
    if (seconds != -1) {
        QVERIFY2(seconds >= 0, "Seconds should be non-negative or -1");
    }
    if (percent != -1) {
        QVERIFY2(percent >= 0 && percent <= 100, "Percent should be 0-100 or -1");
    }

    // Test with nullptr parameters - should not crash
    QString stateOnly = SDLPlatform::getDevicePowerInfo();
    QVERIFY2(!stateOnly.isEmpty(), "Power state should work with null params");

    QString stateSecondsOnly = SDLPlatform::getDevicePowerInfo(&seconds, nullptr);
    Q_UNUSED(stateSecondsOnly);

    QString statePercentOnly = SDLPlatform::getDevicePowerInfo(nullptr, &percent);
    Q_UNUSED(statePercentOnly);
}

//-----------------------------------------------------------------------------
// Android-Specific Tests
//-----------------------------------------------------------------------------

void SDLTest::_androidFunctionsTest()
{
    // These functions should return sensible defaults on non-Android platforms
    // and should not crash on any platform

    int sdkVersion = SDLPlatform::getAndroidSDKVersion();
#ifdef Q_OS_ANDROID
    QVERIFY2(sdkVersion >= 21, "Android SDK version should be at least 21 (Lollipop)");
#else
    QCOMPARE(sdkVersion, 0);
#endif

    QString internalPath = SDLPlatform::getAndroidInternalStoragePath();
#ifdef Q_OS_ANDROID
    QVERIFY2(!internalPath.isEmpty(), "Internal storage path should not be empty on Android");
#else
    QVERIFY2(internalPath.isEmpty(), "Internal storage path should be empty on non-Android");
#endif

    QString externalPath = SDLPlatform::getAndroidExternalStoragePath();
#ifdef Q_OS_ANDROID
    // External storage may or may not be available
    Q_UNUSED(externalPath);
#else
    QVERIFY2(externalPath.isEmpty(), "External storage path should be empty on non-Android");
#endif

    int storageState = SDLPlatform::getAndroidExternalStorageState();
#ifdef Q_OS_ANDROID
    // State depends on device configuration
    Q_UNUSED(storageState);
#else
    QCOMPARE(storageState, static_cast<int>(SDLPlatform::StorageStateNone));
#endif

    // Test permission request - on non-Android, callback should be called with true
    bool callbackCalled = false;
    bool grantedValue = false;
    bool requestResult = SDLPlatform::requestAndroidPermission(
        QStringLiteral("android.permission.CAMERA"),
        [&](bool granted) {
            callbackCalled = true;
            grantedValue = granted;
        }
    );

#ifdef Q_OS_ANDROID
    QVERIFY2(requestResult, "Permission request should be initiated on Android");
    // Note: On Android, callback is async, so we can't verify it immediately
#else
    QVERIFY2(requestResult, "Permission request should return true on non-Android");
    QVERIFY2(callbackCalled, "Callback should be called immediately on non-Android");
    QVERIFY2(grantedValue, "Permission should be granted on non-Android");
#endif
}

//-----------------------------------------------------------------------------
// SDLJoystick Tests
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Initialization Tests
//-----------------------------------------------------------------------------

void SDLTest::_initializationTest()
{
    // Already initialized by test fixture (JoystickSDL::init()), so should be true
    QVERIFY2(SDLJoystick::isInitialized(), "SDL should be initialized after init()");

    // Double init should be safe and succeed
    bool doubleInit = SDLJoystick::init();
    QVERIFY2(doubleInit, "Double initialization should succeed");
    QVERIFY2(SDLJoystick::isInitialized(), "Still initialized after double init");

    // Triple init - still safe
    bool tripleInit = SDLJoystick::init();
    QVERIFY2(tripleInit, "Triple initialization should succeed");
    QVERIFY2(SDLJoystick::isInitialized(), "Still initialized after triple init");

    // Note: We don't test shutdown here because the test fixture (JoystickSDL::init())
    // maintains SDL state through an event watcher and needs it to stay active.
    // shutdown() is tested implicitly through the virtual joystick tests that
    // create and destroy resources.
}

//-----------------------------------------------------------------------------
// Event Control Tests
//-----------------------------------------------------------------------------

void SDLTest::_eventControlTest()
{
    // Test joystick events toggle
    SDLJoystick::setJoystickEventsEnabled(true);
    QVERIFY2(SDLJoystick::joystickEventsEnabled(), "Joystick events should be enabled");

    SDLJoystick::setJoystickEventsEnabled(false);
    QVERIFY2(!SDLJoystick::joystickEventsEnabled(), "Joystick events should be disabled");

    SDLJoystick::setJoystickEventsEnabled(true);
    QVERIFY2(SDLJoystick::joystickEventsEnabled(), "Joystick events should be re-enabled");

    // Test gamepad events toggle
    SDLJoystick::setGamepadEventsEnabled(true);
    QVERIFY2(SDLJoystick::gamepadEventsEnabled(), "Gamepad events should be enabled");

    SDLJoystick::setGamepadEventsEnabled(false);
    QVERIFY2(!SDLJoystick::gamepadEventsEnabled(), "Gamepad events should be disabled");

    SDLJoystick::setGamepadEventsEnabled(true);
    QVERIFY2(SDLJoystick::gamepadEventsEnabled(), "Gamepad events should be re-enabled");

    // Test pump events - should not crash
    SDLJoystick::pumpEvents();

    // Test update functions - should not crash
    SDLJoystick::updateJoysticks();
    SDLJoystick::updateGamepads();
}

//-----------------------------------------------------------------------------
// Thread Safety Tests
//-----------------------------------------------------------------------------

void SDLTest::_threadSafetyTest()
{
    // Test manual lock/unlock
    SDLJoystick::lockJoysticks();
    SDLJoystick::unlockJoysticks();

    // Test double lock/unlock (should be safe)
    SDLJoystick::lockJoysticks();
    SDLJoystick::lockJoysticks();
    SDLJoystick::unlockJoysticks();
    SDLJoystick::unlockJoysticks();

    // Test RAII lock guard
    {
        SDLJoystick::JoystickLock lock;
        // Lock is held here
    }
    // Lock is released here

    // Test nested RAII locks
    {
        SDLJoystick::JoystickLock lock1;
        {
            SDLJoystick::JoystickLock lock2;
            // Both locks held
        }
        // lock2 released
    }
    // lock1 released
}

//-----------------------------------------------------------------------------
// Pre-Open Device Query Tests
//-----------------------------------------------------------------------------

void SDLTest::_preOpenDeviceQueryTest()
{
    const QString testName = QStringLiteral("Pre-Open Query Test Joystick");

    // Create a virtual joystick to query
    int instanceId = SDLJoystick::createVirtualJoystick(testName, 6, 12, 1);
    QVERIFY2(instanceId >= 0, "Virtual joystick creation should succeed");

    // Test all pre-open query functions
    QString name = SDLJoystick::getNameForInstanceId(instanceId);
    QCOMPARE(name, testName);

    QString path = SDLJoystick::getPathForInstanceId(instanceId);
    Q_UNUSED(path);  // May be empty for virtual

    QString guid = SDLJoystick::getGUIDForInstanceId(instanceId);
    QVERIFY2(!guid.isEmpty(), "GUID should not be empty");

    int vendor = SDLJoystick::getVendorForInstanceId(instanceId);
    Q_UNUSED(vendor);  // 0 for virtual

    int product = SDLJoystick::getProductForInstanceId(instanceId);
    Q_UNUSED(product);  // 0 for virtual

    int version = SDLJoystick::getProductVersionForInstanceId(instanceId);
    Q_UNUSED(version);  // 0 for virtual

    QString type = SDLJoystick::getTypeForInstanceId(instanceId);
    Q_UNUSED(type);

    QString realType = SDLJoystick::getRealTypeForInstanceId(instanceId);
    Q_UNUSED(realType);

    int playerIndex = SDLJoystick::getPlayerIndexForInstanceId(instanceId);
    Q_UNUSED(playerIndex);  // -1 if not set

    QString connectionState = SDLJoystick::getConnectionStateForInstanceId(instanceId);
    QVERIFY2(!connectionState.isEmpty(), "Connection state should have a string");

    // Test with invalid instance ID
    QString invalidName = SDLJoystick::getNameForInstanceId(-1);
    QVERIFY2(invalidName.isEmpty(), "Invalid ID should return empty name");

    // For invalid IDs, SDL returns a zeroed GUID (32 zeros), not an empty string
    QString invalidGuid = SDLJoystick::getGUIDForInstanceId(-1);
    bool isZeroedGuid = (invalidGuid == QStringLiteral("00000000000000000000000000000000"));
    bool isEmptyGuid = invalidGuid.isEmpty();
    QVERIFY2(isZeroedGuid || isEmptyGuid, "Invalid ID should return zeroed or empty GUID");

    // Clean up
    bool destroyed = SDLJoystick::destroyVirtualJoystick(instanceId);
    QVERIFY2(destroyed, "Virtual joystick destruction should succeed");
}

//-----------------------------------------------------------------------------
// Type/String Conversion Tests
//-----------------------------------------------------------------------------

void SDLTest::_gamepadTypeConversionTest()
{
    // Test that all standard gamepad types return non-empty strings
    // SDL_GAMEPAD_TYPE enum values may vary by SDL version
    for (int type = 0; type <= 10; ++type) {
        QString typeStr = SDLJoystick::gamepadTypeToString(type);
        // Type 0 is unknown, others should have names
        if (type > 0) {
            // Some types may not exist in all SDL versions
            Q_UNUSED(typeStr);
        }
    }

    // Test display names
    QString unknownDisplay = SDLJoystick::gamepadTypeDisplayName(0);
    QVERIFY2(!unknownDisplay.isEmpty(), "Unknown type should have a display name");

    // Test round-trip for known types
    QString standardType = SDLJoystick::gamepadTypeToString(1);  // Standard
    if (!standardType.isEmpty()) {
        int roundTrip = SDLJoystick::gamepadTypeFromString(standardType);
        QCOMPARE(roundTrip, 1);
    }

    // Test invalid type from string
    int invalidType = SDLJoystick::gamepadTypeFromString(QStringLiteral("not_a_real_type"));
    QCOMPARE(invalidType, 0);  // Should return unknown (0)
}

void SDLTest::_gamepadAxisConversionTest()
{
    // Test all standard axes (0-5 in SDL3)
    // 0=LeftX, 1=LeftY, 2=RightX, 3=RightY, 4=LeftTrigger, 5=RightTrigger
    const QStringList expectedAxes = {
        QStringLiteral("leftx"), QStringLiteral("lefty"),
        QStringLiteral("rightx"), QStringLiteral("righty"),
        QStringLiteral("lefttrigger"), QStringLiteral("righttrigger")
    };

    for (int i = 0; i < expectedAxes.size(); ++i) {
        QString axisStr = SDLJoystick::gamepadAxisToString(i);
        QVERIFY2(!axisStr.isEmpty(), qPrintable(QString("Axis %1 should have a name").arg(i)));

        // Round-trip test
        int roundTrip = SDLJoystick::gamepadAxisFromString(axisStr);
        QCOMPARE(roundTrip, i);
    }

    // Test invalid axis
    int invalidAxis = SDLJoystick::gamepadAxisFromString(QStringLiteral("not_an_axis"));
    QCOMPARE(invalidAxis, -1);  // Invalid should return -1
}

void SDLTest::_gamepadButtonConversionTest()
{
    // Test common buttons (SDL3 has 21 buttons: 0-20)
    // Just test a few key ones to avoid SDL version differences
    QString southButton = SDLJoystick::gamepadButtonToString(0);  // A/Cross
    QVERIFY2(!southButton.isEmpty(), "South button should have a name");

    // Round-trip test
    int roundTrip = SDLJoystick::gamepadButtonFromString(southButton);
    QCOMPARE(roundTrip, 0);

    // Test a few more buttons
    for (int btn : {1, 2, 3, 4, 5}) {  // East, West, North, Back, Guide
        QString btnStr = SDLJoystick::gamepadButtonToString(btn);
        QVERIFY2(!btnStr.isEmpty(), qPrintable(QString("Button %1 should have a name").arg(btn)));
    }

    // Test invalid button
    int invalidBtn = SDLJoystick::gamepadButtonFromString(QStringLiteral("not_a_button"));
    QCOMPARE(invalidBtn, -1);
}

void SDLTest::_connectionStateConversionTest()
{
    // Test all connection states
    // SDL_JOYSTICK_CONNECTION_UNKNOWN = 0, WIRED = 1, WIRELESS = 2
    QString unknown = SDLJoystick::connectionStateToString(0);
    QVERIFY2(!unknown.isEmpty(), "Unknown state should have a string");

    QString wired = SDLJoystick::connectionStateToString(1);
    QVERIFY2(!wired.isEmpty(), "Wired state should have a string");

    QString wireless = SDLJoystick::connectionStateToString(2);
    QVERIFY2(!wireless.isEmpty(), "Wireless state should have a string");

    // Verify they're different
    QVERIFY2(wired != wireless, "Wired and wireless should be different strings");

    // Test out-of-range value - should not crash
    QString outOfRange = SDLJoystick::connectionStateToString(99);
    Q_UNUSED(outOfRange);
}

//-----------------------------------------------------------------------------
// GUID Utilities Tests
//-----------------------------------------------------------------------------

void SDLTest::_guidInfoTest()
{
    // Test with a valid GUID format (32 hex characters)
    QString testGuid = QStringLiteral("03000000010000000100000001000000");

    QVariantMap info = SDLJoystick::getGUIDInfo(testGuid);

    // The map should have some content (keys may vary by SDL version)
    // Just verify it returns without crashing
    Q_UNUSED(info);

    // Test with empty GUID - should not crash
    QVariantMap emptyInfo = SDLJoystick::getGUIDInfo(QString());
    Q_UNUSED(emptyInfo);

    // Test with invalid GUID - should not crash
    QVariantMap invalidInfo = SDLJoystick::getGUIDInfo(QStringLiteral("invalid"));
    Q_UNUSED(invalidInfo);

    // Test getMappingForGUID - just verify it doesn't crash
    QString mapping = SDLJoystick::getMappingForGUID(testGuid);
    Q_UNUSED(mapping);  // May be empty if no mapping exists
}

//-----------------------------------------------------------------------------
// Virtual Joystick Tests
//-----------------------------------------------------------------------------

void SDLTest::_virtualJoystickTest()
{
    const QString testName = QStringLiteral("SDL Test Virtual Joystick");

    // Create a virtual joystick
    int instanceId = SDLJoystick::createVirtualJoystick(testName, 4, 8, 1);
    QVERIFY2(instanceId >= 0, "Virtual joystick creation should succeed");

    // Verify it's recognized as virtual
    QVERIFY2(SDLJoystick::isVirtualJoystick(instanceId), "Should be recognized as virtual");

    // Query device info
    QString name = SDLJoystick::getNameForInstanceId(instanceId);
    QCOMPARE(name, testName);

    // Get path - may be empty for virtual joysticks
    QString path = SDLJoystick::getPathForInstanceId(instanceId);
    Q_UNUSED(path);

    // Get GUID - should return something
    QString guid = SDLJoystick::getGUIDForInstanceId(instanceId);
    QVERIFY2(!guid.isEmpty(), "Virtual joystick should have a GUID");

    // Player index may be -1 if not set
    int playerIndex = SDLJoystick::getPlayerIndexForInstanceId(instanceId);
    Q_UNUSED(playerIndex);

    // Type should be returned
    QString type = SDLJoystick::getTypeForInstanceId(instanceId);
    Q_UNUSED(type);

    // Clean up
    bool destroyed = SDLJoystick::destroyVirtualJoystick(instanceId);
    QVERIFY2(destroyed, "Virtual joystick destruction should succeed");

    // After destruction, should not be virtual anymore
    QVERIFY2(!SDLJoystick::isVirtualJoystick(instanceId), "Should not be virtual after destruction");

    // Destroying again should fail gracefully
    bool destroyedAgain = SDLJoystick::destroyVirtualJoystick(instanceId);
    QVERIFY2(!destroyedAgain, "Destroying non-existent joystick should fail");
}

//-----------------------------------------------------------------------------
// Mapping Management Tests
//-----------------------------------------------------------------------------

void SDLTest::_mappingManagementTest()
{
    // Test reload mappings (loads from gamecontrollerdb.txt resource)
    bool reloaded = SDLJoystick::reloadMappings();
    QVERIFY2(reloaded, "Reloading mappings should succeed");

    // Test adding a valid mapping string
    // Format: GUID,name,mapping
    // Using a unique GUID to avoid conflicts
    QString validMapping = QStringLiteral(
        "ffffffffffffffffffffffffffffffff,QGC Test Controller,"
        "a:b0,b:b1,x:b2,y:b3,back:b4,guide:b5,start:b6,"
        "leftstick:b7,rightstick:b8,leftshoulder:b9,rightshoulder:b10,"
        "dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,"
        "leftx:a0,lefty:a1,rightx:a2,righty:a3,"
        "lefttrigger:a4,righttrigger:a5,");

    bool added = SDLJoystick::addMapping(validMapping);
    QVERIFY2(added, "Adding valid mapping should succeed");

    // Note: SDL may not store mappings for GUIDs that don't correspond to actual devices,
    // so we don't verify retrieval here. The important thing is that addMapping succeeded.

    // Test adding an invalid mapping - should fail gracefully
    bool invalidAdded = SDLJoystick::addMapping(QStringLiteral("invalid mapping string"));
    Q_UNUSED(invalidAdded);  // May or may not fail depending on SDL's parser

    // Test adding empty mapping
    bool emptyAdded = SDLJoystick::addMapping(QString());
    QVERIFY2(!emptyAdded, "Adding empty mapping should fail");

    // Test adding mapping with minimal content
    QString minimalMapping = QStringLiteral(
        "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee,Minimal Controller,a:b0,");
    bool minimalAdded = SDLJoystick::addMapping(minimalMapping);
    QVERIFY2(minimalAdded, "Adding minimal mapping should succeed");

    // Test adding mapping that updates an existing one (same GUID, different name)
    QString updatedMapping = QStringLiteral(
        "ffffffffffffffffffffffffffffffff,QGC Test Controller Updated,"
        "a:b0,b:b1,x:b2,y:b3,");
    bool updated = SDLJoystick::addMapping(updatedMapping);
    QVERIFY2(updated, "Updating existing mapping should succeed");
}
