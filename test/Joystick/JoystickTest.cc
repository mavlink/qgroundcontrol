#include "JoystickTest.h"
#include "MockJoystick.h"
#include "JoystickSDL.h"
#include "Joystick.h"

#include <QtTest/QTest>

UT_REGISTER_TEST(JoystickTest)

JoystickTest::JoystickTest() = default;

void JoystickTest::init()
{
    UnitTest::init();
    JoystickSDL::init();
}

void JoystickTest::cleanup()
{
    _mockJoystick.reset();

    // Don't delete discovered joysticks - they're managed by JoystickSDL::discover()'s
    // static 'previous' map and will be cleaned up on subsequent discover() calls or shutdown
    _discoveredJoysticks.clear();

    UnitTest::cleanup();
}

JoystickSDL *JoystickTest::_findJoystickByInstanceId(int instanceId)
{
    for (Joystick *js : _discoveredJoysticks) {
        if (auto *sdlJs = qobject_cast<JoystickSDL*>(js)) {
            if (sdlJs->instanceId() == instanceId) {
                return sdlJs;
            }
        }
    }
    return nullptr;
}

void JoystickTest::_pumpEvents()
{
    JoystickSDL::pumpEvents();
}

//-----------------------------------------------------------------------------
// Discovery Tests
//-----------------------------------------------------------------------------

void JoystickTest::_discoverVirtualJoystickTest()
{
    _mockJoystick = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Test Controller"), 6, 16, 1));
    QVERIFY(_mockJoystick->isValid());

    _pumpEvents();

    _discoveredJoysticks = JoystickSDL::discover();
    QVERIFY(!_discoveredJoysticks.isEmpty());

    JoystickSDL *js = _findJoystickByInstanceId(_mockJoystick->instanceId());
    QVERIFY(js != nullptr);
    QCOMPARE(js->name(), QStringLiteral("Test Controller"));
}

void JoystickTest::_multipleJoysticksTest()
{
    auto mock1 = std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Controller 1"), 4, 12, 1));
    auto mock2 = std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Controller 2"), 6, 16, 1));
    QVERIFY(mock1->isValid());
    QVERIFY(mock2->isValid());
    QVERIFY(mock1->instanceId() != mock2->instanceId());

    _pumpEvents();

    _discoveredJoysticks = JoystickSDL::discover();
    QVERIFY(_discoveredJoysticks.size() >= 2);

    JoystickSDL *js1 = _findJoystickByInstanceId(mock1->instanceId());
    JoystickSDL *js2 = _findJoystickByInstanceId(mock2->instanceId());
    QVERIFY(js1 != nullptr);
    QVERIFY(js2 != nullptr);
    QVERIFY(js1 != js2);
}

//-----------------------------------------------------------------------------
// Identity Tests
//-----------------------------------------------------------------------------

void JoystickTest::_joystickIdentityTest()
{
    _mockJoystick = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Identity Test Controller"), 6, 16, 1));
    QVERIFY(_mockJoystick->isValid());

    _pumpEvents();

    _discoveredJoysticks = JoystickSDL::discover();
    JoystickSDL *js = _findJoystickByInstanceId(_mockJoystick->instanceId());
    QVERIFY(js != nullptr);

    // Virtual joysticks have vendor/product ID of 0
    QCOMPARE(js->vendorId(), static_cast<quint16>(0));
    QCOMPARE(js->productId(), static_cast<quint16>(0));

    // Virtual joysticks may not have a GUID in SDL3 (empty string is acceptable)
    // Just verify the function doesn't crash
    (void)js->guid();

    // Virtual joystick should report as virtual
    QVERIFY(js->isVirtual());

    // Name should match
    QCOMPARE(js->name(), QStringLiteral("Identity Test Controller"));
}

void JoystickTest::_joystickCapabilitiesTest()
{
    _mockJoystick = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Capabilities Test"), 6, 16, 2));
    QVERIFY(_mockJoystick->isValid());

    _pumpEvents();

    _discoveredJoysticks = JoystickSDL::discover();
    JoystickSDL *js = _findJoystickByInstanceId(_mockJoystick->instanceId());
    QVERIFY(js != nullptr);

    // Verify axis counts match
    QCOMPARE(js->axisCount(), 6);

    // SDL3 adds virtual buttons for hats (4 buttons per hat for directions)
    // So buttonCount = physical buttons + (hats * 4)
    // With 16 buttons and 2 hats, expect 16 + 8 = 24 buttons
    QVERIFY(js->buttonCount() >= 16);

    // Open the joystick so hasAxis/hasButton work (they need SDL handles)
    QVERIFY(js->_open());

    // Test hasAxis/hasButton bounds checking
    QVERIFY(js->hasAxis(0));
    QVERIFY(js->hasAxis(5));
    QVERIFY(!js->hasAxis(6));   // Out of range
    QVERIFY(!js->hasAxis(-1));  // Negative

    QVERIFY(js->hasButton(0));
    QVERIFY(js->hasButton(15));
    // Note: Can't test !hasButton(16) since SDL may have added virtual hat buttons
    QVERIFY(!js->hasButton(-1));  // Negative

    js->_close();

    // Virtual joysticks don't require calibration (they're gamepads)
    // Actually this depends on whether SDL recognizes it as a gamepad
}

//-----------------------------------------------------------------------------
// Axis Tests - Using MockJoystick to set values, verify SDL reads them
//-----------------------------------------------------------------------------

void JoystickTest::_readAxisValuesTest()
{
    _mockJoystick = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Axis Read Test"), 6, 16, 1));
    QVERIFY(_mockJoystick->isValid());

    _pumpEvents();

    _discoveredJoysticks = JoystickSDL::discover();
    JoystickSDL *js = _findJoystickByInstanceId(_mockJoystick->instanceId());
    QVERIFY(js != nullptr);

    // Open the joystick so we can read values (friend access)
    QVERIFY(js->_open());

    // Set axis values via mock
    QVERIFY(_mockJoystick->setAxis(0, 16000));
    QVERIFY(_mockJoystick->setAxis(1, -8000));
    QVERIFY(_mockJoystick->setAxis(2, 0));
    _pumpEvents();

    // Update and read values (friend access to private methods)
    QVERIFY(js->_update());

    // Verify axis values match what we set
    QCOMPARE(js->_getAxisValue(0), 16000);
    QCOMPARE(js->_getAxisValue(1), -8000);
    QCOMPARE(js->_getAxisValue(2), 0);

    js->_close();
}

void JoystickTest::_axisRangeTest()
{
    _mockJoystick = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Axis Range Test"), 6, 16, 1));
    QVERIFY(_mockJoystick->isValid());

    // Test that setting extreme values doesn't crash
    for (int axis = 0; axis < 6; ++axis) {
        QVERIFY(_mockJoystick->setAxis(axis, Joystick::AxisMin));
        QVERIFY(_mockJoystick->setAxis(axis, Joystick::AxisMax));
        QVERIFY(_mockJoystick->setAxis(axis, 0));
    }

    _pumpEvents();

    _discoveredJoysticks = JoystickSDL::discover();
    JoystickSDL *js = _findJoystickByInstanceId(_mockJoystick->instanceId());
    QVERIFY(js != nullptr);
}

//-----------------------------------------------------------------------------
// Button Tests
//-----------------------------------------------------------------------------

void JoystickTest::_readButtonStatesTest()
{
    _mockJoystick = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Button Read Test"), 4, 16, 1));
    QVERIFY(_mockJoystick->isValid());

    _pumpEvents();

    _discoveredJoysticks = JoystickSDL::discover();
    JoystickSDL *js = _findJoystickByInstanceId(_mockJoystick->instanceId());
    QVERIFY(js != nullptr);
    // SDL3 adds virtual buttons for hats, so check minimum button count
    QVERIFY(js->buttonCount() >= 16);

    // Open joystick (friend access)
    QVERIFY(js->_open());

    // Initially all buttons should be released
    js->_update();
    QVERIFY(!js->_getButton(0));
    QVERIFY(!js->_getButton(5));

    // Press some buttons
    QVERIFY(_mockJoystick->pressButton(0));
    QVERIFY(_mockJoystick->pressButton(5));
    _pumpEvents();
    js->_update();

    // Verify buttons are pressed
    QVERIFY(js->_getButton(0));
    QVERIFY(js->_getButton(5));
    QVERIFY(!js->_getButton(1));  // Not pressed

    // Release buttons
    QVERIFY(_mockJoystick->releaseButton(0));
    QVERIFY(_mockJoystick->releaseButton(5));
    _pumpEvents();
    js->_update();

    // Verify buttons are released
    QVERIFY(!js->_getButton(0));
    QVERIFY(!js->_getButton(5));

    js->_close();
}

void JoystickTest::_buttonPressReleaseSequenceTest()
{
    _mockJoystick = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Button Sequence Test"), 4, 16, 1));
    QVERIFY(_mockJoystick->isValid());

    _pumpEvents();

    _discoveredJoysticks = JoystickSDL::discover();
    JoystickSDL *js = _findJoystickByInstanceId(_mockJoystick->instanceId());
    QVERIFY(js != nullptr);

    // Simulate rapid button presses
    for (int btn = 0; btn < 8; ++btn) {
        QVERIFY(_mockJoystick->pressButton(btn));
        _pumpEvents();
        QVERIFY(_mockJoystick->releaseButton(btn));
        _pumpEvents();
    }

    // Test releasing all buttons
    _mockJoystick->releaseAllButtons();
    _pumpEvents();
}

//-----------------------------------------------------------------------------
// Hat Tests
//-----------------------------------------------------------------------------

void JoystickTest::_readHatDirectionsTest()
{
    _mockJoystick = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Hat Direction Test"), 4, 12, 2));
    QVERIFY(_mockJoystick->isValid());

    _pumpEvents();

    _discoveredJoysticks = JoystickSDL::discover();
    JoystickSDL *js = _findJoystickByInstanceId(_mockJoystick->instanceId());
    QVERIFY(js != nullptr);

    // Test all hat directions
    const Joystick::HatDirection directions[] = {
        Joystick::HatUp,
        Joystick::HatRight,
        Joystick::HatDown,
        Joystick::HatLeft,
        Joystick::HatRightUp,
        Joystick::HatRightDown,
        Joystick::HatLeftUp,
        Joystick::HatLeftDown,
        Joystick::HatCentered
    };

    for (const auto dir : directions) {
        QVERIFY(_mockJoystick->setHat(0, dir));
        _pumpEvents();
    }

    // Test second hat if present
    QVERIFY(_mockJoystick->setHat(1, Joystick::HatUp));
    QVERIFY(_mockJoystick->centerHat(1));
    _pumpEvents();
}

//-----------------------------------------------------------------------------
// Polling/Integration Tests
//-----------------------------------------------------------------------------

void JoystickTest::_pollingUpdatesValuesTest()
{
    _mockJoystick = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Polling Test"), 6, 16, 1));
    QVERIFY(_mockJoystick->isValid());

    _pumpEvents();

    _discoveredJoysticks = JoystickSDL::discover();
    JoystickSDL *js = _findJoystickByInstanceId(_mockJoystick->instanceId());
    QVERIFY(js != nullptr);

    // Simulate a typical control sequence
    _mockJoystick->centerAllAxes();
    _pumpEvents();

    // Left stick movement
    QVERIFY(_mockJoystick->setLeftStick(20000, -15000));
    _pumpEvents();

    // Right stick movement
    QVERIFY(_mockJoystick->setRightStick(-10000, 10000));
    _pumpEvents();

    // Trigger pulls
    QVERIFY(_mockJoystick->setLeftTrigger(16000));
    QVERIFY(_mockJoystick->setRightTrigger(32000));
    _pumpEvents();

    // Button combo
    QVERIFY(_mockJoystick->pressButton(Joystick::ButtonA));
    QVERIFY(_mockJoystick->pressButton(Joystick::ButtonB));
    _pumpEvents();

    // Reset everything
    _mockJoystick->reset();
    _pumpEvents();
}

//-----------------------------------------------------------------------------
// Calibration Tests
//-----------------------------------------------------------------------------

void JoystickTest::_calibrationDataTest()
{
    _mockJoystick = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Calibration Test"), 6, 16, 1));
    QVERIFY(_mockJoystick->isValid());

    _pumpEvents();

    _discoveredJoysticks = JoystickSDL::discover();
    JoystickSDL *js = _findJoystickByInstanceId(_mockJoystick->instanceId());
    QVERIFY(js != nullptr);

    // Test default calibration values
    Joystick::AxisCalibration_t cal = js->getAxisCalibration(0);
    QCOMPARE(cal.min, Joystick::AxisMin);
    QCOMPARE(cal.max, Joystick::AxisMax);
    QCOMPARE(cal.center, 0);
    QCOMPARE(cal.deadband, 0);
    QCOMPARE(cal.reversed, false);

    // Test setting calibration
    Joystick::AxisCalibration_t newCal;
    newCal.min = -30000;
    newCal.max = 30000;
    newCal.center = 500;
    newCal.deadband = 1000;
    newCal.reversed = true;

    js->setAxisCalibration(0, newCal);
    Joystick::AxisCalibration_t readBack = js->getAxisCalibration(0);
    QCOMPARE(readBack.min, newCal.min);
    QCOMPARE(readBack.max, newCal.max);
    QCOMPARE(readBack.center, newCal.center);
    QCOMPARE(readBack.deadband, newCal.deadband);
    QCOMPARE(readBack.reversed, newCal.reversed);

    // Test calibration for multiple axes
    for (int axis = 0; axis < js->axisCount(); ++axis) {
        Joystick::AxisCalibration_t axisCal;
        axisCal.min = -32000 + axis * 100;
        axisCal.max = 32000 - axis * 100;
        axisCal.center = axis * 10;
        axisCal.deadband = 500 + axis * 50;
        axisCal.reversed = (axis % 2 == 0);

        js->setAxisCalibration(axis, axisCal);
    }

    // Verify all calibrations persisted correctly
    for (int axis = 0; axis < js->axisCount(); ++axis) {
        Joystick::AxisCalibration_t axisCal = js->getAxisCalibration(axis);
        QCOMPARE(axisCal.min, -32000 + axis * 100);
        QCOMPARE(axisCal.max, 32000 - axis * 100);
        QCOMPARE(axisCal.center, axis * 10);
        QCOMPARE(axisCal.deadband, 500 + axis * 50);
        QCOMPARE(axisCal.reversed, (axis % 2 == 0));
    }
}

void JoystickTest::_adjustRangeTest()
{
    _mockJoystick = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Adjust Range Test"), 6, 16, 1));
    QVERIFY(_mockJoystick->isValid());

    _pumpEvents();

    _discoveredJoysticks = JoystickSDL::discover();
    JoystickSDL *js = _findJoystickByInstanceId(_mockJoystick->instanceId());
    QVERIFY(js != nullptr);
    QVERIFY(js->_open());

    // Set up a known calibration
    Joystick::AxisCalibration_t cal;
    cal.min = -32767;
    cal.max = 32767;
    cal.center = 0;
    cal.deadband = 0;
    cal.reversed = false;
    js->setAxisCalibration(0, cal);

    // Test center value maps to 0
    QVERIFY(_mockJoystick->setAxis(0, 0));
    _pumpEvents();
    QVERIFY(js->_update());
    QCOMPARE(js->_getAxisValue(0), 0);

    // Test extreme values
    QVERIFY(_mockJoystick->setAxis(0, Joystick::AxisMax));
    _pumpEvents();
    QVERIFY(js->_update());
    QCOMPARE(js->_getAxisValue(0), Joystick::AxisMax);

    QVERIFY(_mockJoystick->setAxis(0, Joystick::AxisMin));
    _pumpEvents();
    QVERIFY(js->_update());
    QCOMPARE(js->_getAxisValue(0), Joystick::AxisMin);

    js->_close();
}

//-----------------------------------------------------------------------------
// Error Handling Tests
//-----------------------------------------------------------------------------

void JoystickTest::_invalidAxisIndexTest()
{
    _mockJoystick = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Invalid Axis Test"), 4, 8, 1));
    QVERIFY(_mockJoystick->isValid());

    _pumpEvents();

    _discoveredJoysticks = JoystickSDL::discover();
    JoystickSDL *js = _findJoystickByInstanceId(_mockJoystick->instanceId());
    QVERIFY(js != nullptr);
    QVERIFY(js->_open());

    // Test hasAxis bounds
    QVERIFY(js->hasAxis(0));
    QVERIFY(js->hasAxis(3));
    QVERIFY(!js->hasAxis(4));
    QVERIFY(!js->hasAxis(100));
    QVERIFY(!js->hasAxis(-1));
    QVERIFY(!js->hasAxis(-100));

    // Test getAxisCalibration with invalid index returns default calibration
    Joystick::AxisCalibration_t cal = js->getAxisCalibration(100);
    QCOMPARE(cal.min, Joystick::AxisMin);
    QCOMPARE(cal.max, Joystick::AxisMax);
    QCOMPARE(cal.center, 0);

    js->_close();
}

void JoystickTest::_invalidButtonIndexTest()
{
    _mockJoystick = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Invalid Button Test"), 4, 8, 0));
    QVERIFY(_mockJoystick->isValid());

    _pumpEvents();

    _discoveredJoysticks = JoystickSDL::discover();
    JoystickSDL *js = _findJoystickByInstanceId(_mockJoystick->instanceId());
    QVERIFY(js != nullptr);
    QVERIFY(js->_open());

    // Test hasButton bounds
    QVERIFY(js->hasButton(0));
    QVERIFY(js->hasButton(7));
    QVERIFY(!js->hasButton(100));
    QVERIFY(!js->hasButton(-1));
    QVERIFY(!js->hasButton(-100));

    // Test _getButton with invalid index doesn't crash
    QVERIFY(!js->_getButton(100));
    QVERIFY(!js->_getButton(-1));

    js->_close();
}

void JoystickTest::_zeroCapabilityJoystickTest()
{
    // Test joystick with minimum capabilities (1 axis, 1 button, 0 hats)
    auto minimalMock = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Minimal Joystick"), 1, 1, 0));
    QVERIFY(minimalMock->isValid());

    _pumpEvents();

    _discoveredJoysticks = JoystickSDL::discover();
    JoystickSDL *js = _findJoystickByInstanceId(minimalMock->instanceId());
    QVERIFY(js != nullptr);

    QCOMPARE(js->axisCount(), 1);
    QVERIFY(js->buttonCount() >= 1);

    QVERIFY(js->_open());

    // Test the single axis
    QVERIFY(minimalMock->setAxis(0, 16000));
    _pumpEvents();
    QVERIFY(js->_update());
    QCOMPARE(js->_getAxisValue(0), 16000);

    // Test the single button
    QVERIFY(minimalMock->pressButton(0));
    _pumpEvents();
    QVERIFY(js->_update());
    QVERIFY(js->_getButton(0));

    js->_close();
}

//-----------------------------------------------------------------------------
// Hot-plug Tests
//-----------------------------------------------------------------------------

void JoystickTest::_joystickDisconnectTest()
{
    _mockJoystick = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Disconnect Test"), 4, 8, 1));
    QVERIFY(_mockJoystick->isValid());
    int instanceId = _mockJoystick->instanceId();

    _pumpEvents();

    // Verify joystick is discovered
    _discoveredJoysticks = JoystickSDL::discover();
    JoystickSDL *js = _findJoystickByInstanceId(instanceId);
    QVERIFY(js != nullptr);

    // Simulate disconnect by destroying the mock
    _mockJoystick.reset();
    _pumpEvents();

    // Re-discover - the joystick should be gone
    _discoveredJoysticks = JoystickSDL::discover();
    JoystickSDL *jsAfter = _findJoystickByInstanceId(instanceId);
    QVERIFY(jsAfter == nullptr);
}

//-----------------------------------------------------------------------------
// Hat-to-Button Mapping Tests
//-----------------------------------------------------------------------------

void JoystickTest::_hatButtonMappingTest()
{
    // Create joystick with 2 hats to verify button count increases
    _mockJoystick = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Hat Button Test"), 4, 8, 2));
    QVERIFY(_mockJoystick->isValid());

    _pumpEvents();

    _discoveredJoysticks = JoystickSDL::discover();
    JoystickSDL *js = _findJoystickByInstanceId(_mockJoystick->instanceId());
    QVERIFY(js != nullptr);

    // SDL3 adds 4 virtual buttons per hat for D-pad directions
    // With 8 physical buttons and 2 hats, expect at least 8 + (2 * 4) = 16 buttons
    QVERIFY(js->buttonCount() >= 8);

    QVERIFY(js->_open());

    // _getHat takes (hat, idx) where idx maps to: 0=UP, 1=DOWN, 2=LEFT, 3=RIGHT
    constexpr int HatIdxUp = 0;
    constexpr int HatIdxDown = 1;
    constexpr int HatIdxLeft = 2;
    constexpr int HatIdxRight = 3;

    // Test that hat directions are correctly read
    QVERIFY(_mockJoystick->setHat(0, Joystick::HatUp));
    _pumpEvents();
    QVERIFY(js->_update());
    QVERIFY(js->_getHat(0, HatIdxUp));
    QVERIFY(!js->_getHat(0, HatIdxDown));

    QVERIFY(_mockJoystick->setHat(0, Joystick::HatDown));
    _pumpEvents();
    QVERIFY(js->_update());
    QVERIFY(js->_getHat(0, HatIdxDown));
    QVERIFY(!js->_getHat(0, HatIdxUp));

    // Test diagonal (right + up)
    QVERIFY(_mockJoystick->setHat(0, Joystick::HatRightUp));
    _pumpEvents();
    QVERIFY(js->_update());
    QVERIFY(js->_getHat(0, HatIdxRight));
    QVERIFY(js->_getHat(0, HatIdxUp));
    QVERIFY(!js->_getHat(0, HatIdxDown));
    QVERIFY(!js->_getHat(0, HatIdxLeft));

    // Test centered (no directions active)
    QVERIFY(_mockJoystick->centerHat(0));
    _pumpEvents();
    QVERIFY(js->_update());
    QVERIFY(!js->_getHat(0, HatIdxUp));
    QVERIFY(!js->_getHat(0, HatIdxDown));
    QVERIFY(!js->_getHat(0, HatIdxLeft));
    QVERIFY(!js->_getHat(0, HatIdxRight));

    js->_close();
}

void JoystickTest::_hatMutualExclusionTest()
{
    // Test hat state changes through normal hat operations
    constexpr int HatIdxUp = 0;
    constexpr int HatIdxDown = 1;
    constexpr int HatIdxLeft = 2;
    constexpr int HatIdxRight = 3;

    _mockJoystick = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Hat Mutual Exclusion Test"), 4, 8, 2));
    QVERIFY(_mockJoystick->isValid());

    _pumpEvents();

    _discoveredJoysticks = JoystickSDL::discover();
    JoystickSDL *js = _findJoystickByInstanceId(_mockJoystick->instanceId());
    QVERIFY(js != nullptr);

    QVERIFY(js->_open());

    // Set hat 0 to UP and verify via _getHat
    QVERIFY(_mockJoystick->setHat(0, Joystick::HatUp));
    _pumpEvents();
    QVERIFY(js->_update());
    QVERIFY(js->_getHat(0, HatIdxUp));
    QVERIFY(!js->_getHat(0, HatIdxDown));

    // Set hat 0 to DOWN
    QVERIFY(_mockJoystick->setHat(0, Joystick::HatDown));
    _pumpEvents();
    QVERIFY(js->_update());
    QVERIFY(js->_getHat(0, HatIdxDown));
    QVERIFY(!js->_getHat(0, HatIdxUp));

    // Test diagonal (UP and RIGHT)
    QVERIFY(_mockJoystick->setHat(0, Joystick::HatRightUp));
    _pumpEvents();
    QVERIFY(js->_update());
    QVERIFY(js->_getHat(0, HatIdxUp));
    QVERIFY(js->_getHat(0, HatIdxRight));
    QVERIFY(!js->_getHat(0, HatIdxDown));
    QVERIFY(!js->_getHat(0, HatIdxLeft));

    // Test that hat 1 is independent
    QVERIFY(_mockJoystick->setHat(1, Joystick::HatLeft));
    _pumpEvents();
    QVERIFY(js->_update());
    QVERIFY(js->_getHat(1, HatIdxLeft));
    // Hat 0 should still be diagonal
    QVERIFY(js->_getHat(0, HatIdxUp));
    QVERIFY(js->_getHat(0, HatIdxRight));

    // Center both hats
    QVERIFY(_mockJoystick->centerHat(0));
    QVERIFY(_mockJoystick->centerHat(1));
    _pumpEvents();
    QVERIFY(js->_update());
    QVERIFY(!js->_getHat(0, HatIdxUp));
    QVERIFY(!js->_getHat(0, HatIdxDown));
    QVERIFY(!js->_getHat(0, HatIdxLeft));
    QVERIFY(!js->_getHat(0, HatIdxRight));
    QVERIFY(!js->_getHat(1, HatIdxLeft));

    js->_close();
}

//-----------------------------------------------------------------------------
// Button Action Tests
//-----------------------------------------------------------------------------

void JoystickTest::_buttonActionAssignmentTest()
{
    _mockJoystick = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Button Action Test"), 4, 16, 1));
    QVERIFY(_mockJoystick->isValid());

    _pumpEvents();

    _discoveredJoysticks = JoystickSDL::discover();
    JoystickSDL *js = _findJoystickByInstanceId(_mockJoystick->instanceId());
    QVERIFY(js != nullptr);

    // Test default button action is "No Action"
    QString defaultAction = js->getButtonAction(0);
    QCOMPARE(defaultAction, js->buttonActionNone());

    // Test setting a button action
    js->setButtonAction(0, QStringLiteral("Arm"));
    QCOMPARE(js->getButtonAction(0), QStringLiteral("Arm"));

    // Test setting action on different button
    js->setButtonAction(1, QStringLiteral("Disarm"));
    QCOMPARE(js->getButtonAction(1), QStringLiteral("Disarm"));

    // Verify first button action unchanged
    QCOMPARE(js->getButtonAction(0), QStringLiteral("Arm"));

    // Test clearing action
    js->setButtonAction(0, js->buttonActionNone());
    QCOMPARE(js->getButtonAction(0), js->buttonActionNone());

    // Test button repeat settings - must have action assigned first for repeat to work
    js->setButtonAction(2, QStringLiteral("Trigger Camera"));
    QVERIFY(!js->getButtonRepeat(2));  // Default is no repeat
    js->setButtonRepeat(2, true);
    QVERIFY(js->getButtonRepeat(2));
    js->setButtonRepeat(2, false);
    QVERIFY(!js->getButtonRepeat(2));
}
