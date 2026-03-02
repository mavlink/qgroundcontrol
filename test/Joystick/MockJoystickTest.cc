#include "MockJoystickTest.h"

#include "JoystickSDL.h"
#include "MockJoystick.h"
#include "SDLJoystick.h"

void MockJoystickTest::initTestCase()
{
    UnitTest::initTestCase();
    QVERIFY(JoystickSDL::init());
}

void MockJoystickTest::init()
{
    UnitTest::init();
}

void MockJoystickTest::cleanup()
{
    _mockJoystick = nullptr;
    UnitTest::cleanup();
}

//-----------------------------------------------------------------------------
// Creation Tests
//-----------------------------------------------------------------------------
void MockJoystickTest::_createMockJoystickTest()
{
    // Test default creation
    _mockJoystick = std::unique_ptr<MockJoystick>(MockJoystick::create());
    QVERIFY(_mockJoystick != nullptr);
    QVERIFY(_mockJoystick->isValid());
    // Verify default counts
    QCOMPARE(_mockJoystick->mockAxisCount(), 6);
    QCOMPARE(_mockJoystick->mockButtonCount(), 16);
    QCOMPARE(_mockJoystick->mockHatCount(), 1);
}

void MockJoystickTest::_createWithCustomCountsTest()
{
    // Test custom counts
    _mockJoystick =
        std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Custom Controller"), 8, 24, 2, 1, 1));
    QVERIFY(_mockJoystick != nullptr);
    QVERIFY(_mockJoystick->isValid());
    QCOMPARE(_mockJoystick->mockAxisCount(), 8);
    QCOMPARE(_mockJoystick->mockButtonCount(), 24);
    QCOMPARE(_mockJoystick->mockHatCount(), 2);
    QCOMPARE(_mockJoystick->mockBallCount(), 1);
    QCOMPARE(_mockJoystick->mockTouchpadCount(), 1);
}

//-----------------------------------------------------------------------------
// Axis State Tracking Tests
//-----------------------------------------------------------------------------
void MockJoystickTest::_axisStateTrackingTest()
{
    _mockJoystick = std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Axis Tracking Test"), 6, 16, 1));
    QVERIFY(_mockJoystick->isValid());
    // Initial state should be 0
    for (int i = 0; i < 6; ++i) {
        QCOMPARE(_mockJoystick->getAxisValue(i), 0);
    }
    // Set axis and verify tracking
    QVERIFY(_mockJoystick->setAxis(0, 16000));
    QCOMPARE(_mockJoystick->getAxisValue(0), 16000);
    QVERIFY(_mockJoystick->setAxis(1, -16000));
    QCOMPARE(_mockJoystick->getAxisValue(1), -16000);
    // Verify other axes unchanged
    QCOMPARE(_mockJoystick->getAxisValue(2), 0);
    // Test extreme values
    QVERIFY(_mockJoystick->setAxis(2, Joystick::AxisMax));
    QCOMPARE(_mockJoystick->getAxisValue(2), Joystick::AxisMax);
    QVERIFY(_mockJoystick->setAxis(3, Joystick::AxisMin));
    QCOMPARE(_mockJoystick->getAxisValue(3), Joystick::AxisMin);
    // Test out of range index returns 0
    QCOMPARE(_mockJoystick->getAxisValue(100), 0);
    QCOMPARE(_mockJoystick->getAxisValue(-1), 0);
}

void MockJoystickTest::_stickConvenienceMethodsTest()
{
    _mockJoystick = std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Stick Methods Test"), 6, 16, 1));
    QVERIFY(_mockJoystick->isValid());
    // Test left stick
    QVERIFY(_mockJoystick->setLeftStick(10000, -5000));
    QCOMPARE(_mockJoystick->getAxisValue(Joystick::AxisLeftX), 10000);
    QCOMPARE(_mockJoystick->getAxisValue(Joystick::AxisLeftY), -5000);
    // Test right stick
    QVERIFY(_mockJoystick->setRightStick(-20000, 15000));
    QCOMPARE(_mockJoystick->getAxisValue(Joystick::AxisRightX), -20000);
    QCOMPARE(_mockJoystick->getAxisValue(Joystick::AxisRightY), 15000);
    // Test center all
    _mockJoystick->centerAllAxes();
    for (int i = 0; i < 6; ++i) {
        QCOMPARE(_mockJoystick->getAxisValue(i), 0);
    }
}

void MockJoystickTest::_triggerMethodsTest()
{
    _mockJoystick =
        std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Trigger Methods Test"), 6, 16, 1));
    QVERIFY(_mockJoystick->isValid());
    // Test left trigger
    QVERIFY(_mockJoystick->setLeftTrigger(20000));
    QCOMPARE(_mockJoystick->getAxisValue(Joystick::AxisTriggerLeft), 20000);
    // Test right trigger
    QVERIFY(_mockJoystick->setRightTrigger(30000));
    QCOMPARE(_mockJoystick->getAxisValue(Joystick::AxisTriggerRight), 30000);
}

//-----------------------------------------------------------------------------
// Button State Tracking Tests
//-----------------------------------------------------------------------------
void MockJoystickTest::_buttonStateTrackingTest()
{
    _mockJoystick =
        std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Button Tracking Test"), 6, 16, 1));
    QVERIFY(_mockJoystick->isValid());
    // Initial state should be false
    for (int i = 0; i < 16; ++i) {
        QVERIFY(!_mockJoystick->getButtonState(i));
    }
    // Press button and verify
    QVERIFY(_mockJoystick->pressButton(0));
    QVERIFY(_mockJoystick->getButtonState(0));
    QVERIFY(_mockJoystick->pressButton(5));
    QVERIFY(_mockJoystick->getButtonState(5));
    // Release and verify
    QVERIFY(_mockJoystick->releaseButton(0));
    QVERIFY(!_mockJoystick->getButtonState(0));
    // Button 5 still pressed
    QVERIFY(_mockJoystick->getButtonState(5));
    // Release all
    _mockJoystick->releaseAllButtons();
    for (int i = 0; i < 16; ++i) {
        QVERIFY(!_mockJoystick->getButtonState(i));
    }
    // Test out of range
    QVERIFY(!_mockJoystick->getButtonState(100));
    QVERIFY(!_mockJoystick->getButtonState(-1));
}

void MockJoystickTest::_gamepadButtonConvenienceTest()
{
    _mockJoystick =
        std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Gamepad Button Test"), 6, 16, 1));
    QVERIFY(_mockJoystick->isValid());
    // Test face buttons
    QVERIFY(_mockJoystick->pressA());
    QVERIFY(_mockJoystick->getButtonState(Joystick::ButtonA));
    QVERIFY(_mockJoystick->releaseA());
    QVERIFY(!_mockJoystick->getButtonState(Joystick::ButtonA));
    QVERIFY(_mockJoystick->pressB());
    QVERIFY(_mockJoystick->getButtonState(Joystick::ButtonB));
    QVERIFY(_mockJoystick->releaseB());
    QVERIFY(_mockJoystick->pressX());
    QVERIFY(_mockJoystick->getButtonState(Joystick::ButtonX));
    QVERIFY(_mockJoystick->releaseX());
    QVERIFY(_mockJoystick->pressY());
    QVERIFY(_mockJoystick->getButtonState(Joystick::ButtonY));
    QVERIFY(_mockJoystick->releaseY());
    // Test menu buttons
    QVERIFY(_mockJoystick->pressStart());
    QVERIFY(_mockJoystick->getButtonState(Joystick::ButtonStart));
    QVERIFY(_mockJoystick->releaseStart());
    QVERIFY(_mockJoystick->pressBack());
    QVERIFY(_mockJoystick->getButtonState(Joystick::ButtonBack));
    QVERIFY(_mockJoystick->releaseBack());
    QVERIFY(_mockJoystick->pressGuide());
    QVERIFY(_mockJoystick->getButtonState(Joystick::ButtonGuide));
    QVERIFY(_mockJoystick->releaseGuide());
    // Test shoulder buttons
    QVERIFY(_mockJoystick->pressLeftShoulder());
    QVERIFY(_mockJoystick->getButtonState(Joystick::ButtonLeftShoulder));
    QVERIFY(_mockJoystick->releaseLeftShoulder());
    QVERIFY(_mockJoystick->pressRightShoulder());
    QVERIFY(_mockJoystick->getButtonState(Joystick::ButtonRightShoulder));
    QVERIFY(_mockJoystick->releaseRightShoulder());
    // Test stick buttons
    QVERIFY(_mockJoystick->pressLeftStickButton());
    QVERIFY(_mockJoystick->getButtonState(Joystick::ButtonLeftStick));
    QVERIFY(_mockJoystick->releaseLeftStickButton());
    QVERIFY(_mockJoystick->pressRightStickButton());
    QVERIFY(_mockJoystick->getButtonState(Joystick::ButtonRightStick));
    QVERIFY(_mockJoystick->releaseRightStickButton());
}

void MockJoystickTest::_multiButtonPressTest()
{
    _mockJoystick = std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Multi Button Test"), 6, 16, 1));
    QVERIFY(_mockJoystick->isValid());
    // Press multiple buttons at once
    QVector<int> buttons = {0, 2, 5, 8};
    QVERIFY(_mockJoystick->pressButtons(buttons));
    for (int btn : buttons) {
        QVERIFY(_mockJoystick->getButtonState(btn));
    }
    // Verify other buttons not pressed
    QVERIFY(!_mockJoystick->getButtonState(1));
    QVERIFY(!_mockJoystick->getButtonState(3));
    // Release multiple buttons
    QVERIFY(_mockJoystick->releaseButtons(buttons));
    for (int btn : buttons) {
        QVERIFY(!_mockJoystick->getButtonState(btn));
    }
}

//-----------------------------------------------------------------------------
// Hat State Tracking Tests
//-----------------------------------------------------------------------------
void MockJoystickTest::_hatStateTrackingTest()
{
    _mockJoystick = std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Hat Tracking Test"), 6, 16, 2));
    QVERIFY(_mockJoystick->isValid());
    // Initial state should be centered
    QCOMPARE(_mockJoystick->getHatDirection(0), Joystick::HatCentered);
    QCOMPARE(_mockJoystick->getHatDirection(1), Joystick::HatCentered);
    // Test all directions
    QVERIFY(_mockJoystick->setHat(0, Joystick::HatUp));
    QCOMPARE(_mockJoystick->getHatDirection(0), Joystick::HatUp);
    QVERIFY(_mockJoystick->setHat(0, Joystick::HatRight));
    QCOMPARE(_mockJoystick->getHatDirection(0), Joystick::HatRight);
    QVERIFY(_mockJoystick->setHat(0, Joystick::HatDown));
    QCOMPARE(_mockJoystick->getHatDirection(0), Joystick::HatDown);
    QVERIFY(_mockJoystick->setHat(0, Joystick::HatLeft));
    QCOMPARE(_mockJoystick->getHatDirection(0), Joystick::HatLeft);
    // Test diagonals
    QVERIFY(_mockJoystick->setHat(0, Joystick::HatRightUp));
    QCOMPARE(_mockJoystick->getHatDirection(0), Joystick::HatRightUp);
    QVERIFY(_mockJoystick->setHat(0, Joystick::HatRightDown));
    QCOMPARE(_mockJoystick->getHatDirection(0), Joystick::HatRightDown);
    QVERIFY(_mockJoystick->setHat(0, Joystick::HatLeftUp));
    QCOMPARE(_mockJoystick->getHatDirection(0), Joystick::HatLeftUp);
    QVERIFY(_mockJoystick->setHat(0, Joystick::HatLeftDown));
    QCOMPARE(_mockJoystick->getHatDirection(0), Joystick::HatLeftDown);
    // Test center
    QVERIFY(_mockJoystick->centerHat(0));
    QCOMPARE(_mockJoystick->getHatDirection(0), Joystick::HatCentered);
    // Test second hat independence
    QVERIFY(_mockJoystick->setHat(1, Joystick::HatUp));
    QCOMPARE(_mockJoystick->getHatDirection(1), Joystick::HatUp);
    QCOMPARE(_mockJoystick->getHatDirection(0), Joystick::HatCentered);
    // Test out of range
    QCOMPARE(_mockJoystick->getHatDirection(100), Joystick::HatCentered);
}

void MockJoystickTest::_dpadConvenienceTest()
{
    _mockJoystick =
        std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("DPad Convenience Test"), 6, 16, 1));
    QVERIFY(_mockJoystick->isValid());
    // Test cardinal directions
    QVERIFY(_mockJoystick->pressDPadUp());
    QCOMPARE(_mockJoystick->getHatDirection(0), Joystick::HatUp);
    QVERIFY(_mockJoystick->pressDPadDown());
    QCOMPARE(_mockJoystick->getHatDirection(0), Joystick::HatDown);
    QVERIFY(_mockJoystick->pressDPadLeft());
    QCOMPARE(_mockJoystick->getHatDirection(0), Joystick::HatLeft);
    QVERIFY(_mockJoystick->pressDPadRight());
    QCOMPARE(_mockJoystick->getHatDirection(0), Joystick::HatRight);
    // Test diagonals
    QVERIFY(_mockJoystick->pressDPadUpRight());
    QCOMPARE(_mockJoystick->getHatDirection(0), Joystick::HatRightUp);
    QVERIFY(_mockJoystick->pressDPadUpLeft());
    QCOMPARE(_mockJoystick->getHatDirection(0), Joystick::HatLeftUp);
    QVERIFY(_mockJoystick->pressDPadDownRight());
    QCOMPARE(_mockJoystick->getHatDirection(0), Joystick::HatRightDown);
    QVERIFY(_mockJoystick->pressDPadDownLeft());
    QCOMPARE(_mockJoystick->getHatDirection(0), Joystick::HatLeftDown);
    // Test release
    QVERIFY(_mockJoystick->releaseDPad());
    QCOMPARE(_mockJoystick->getHatDirection(0), Joystick::HatCentered);
}

//-----------------------------------------------------------------------------
// Input Sequence Tests
//-----------------------------------------------------------------------------
void MockJoystickTest::_buttonTapSequenceTest()
{
    _mockJoystick = std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Button Tap Test"), 6, 16, 1));
    QVERIFY(_mockJoystick->isValid());
    // Tap should leave button released
    QVERIFY(_mockJoystick->tapButton(0));
    QVERIFY(!_mockJoystick->getButtonState(0));
    QVERIFY(_mockJoystick->tapButton(5));
    QVERIFY(!_mockJoystick->getButtonState(5));
}

void MockJoystickTest::_stickDeflectionSequenceTest()
{
    _mockJoystick =
        std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Stick Deflection Test"), 6, 16, 1));
    QVERIFY(_mockJoystick->isValid());
    // Deflect should leave stick centered
    QVERIFY(_mockJoystick->deflectLeftStick(20000, -15000));
    QCOMPARE(_mockJoystick->getAxisValue(Joystick::AxisLeftX), 0);
    QCOMPARE(_mockJoystick->getAxisValue(Joystick::AxisLeftY), 0);
    QVERIFY(_mockJoystick->deflectRightStick(-10000, 25000));
    QCOMPARE(_mockJoystick->getAxisValue(Joystick::AxisRightX), 0);
    QCOMPARE(_mockJoystick->getAxisValue(Joystick::AxisRightY), 0);
}

void MockJoystickTest::_triggerPullSequenceTest()
{
    _mockJoystick = std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Trigger Pull Test"), 6, 16, 1));
    QVERIFY(_mockJoystick->isValid());
    // Pull should leave trigger at 0
    QVERIFY(_mockJoystick->pullLeftTrigger());
    QCOMPARE(_mockJoystick->getAxisValue(Joystick::AxisTriggerLeft), 0);
    QVERIFY(_mockJoystick->pullRightTrigger());
    QCOMPARE(_mockJoystick->getAxisValue(Joystick::AxisTriggerRight), 0);
}

void MockJoystickTest::_dpadTapSequenceTest()
{
    _mockJoystick = std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("DPad Tap Test"), 6, 16, 1));
    QVERIFY(_mockJoystick->isValid());
    // Tap should leave hat centered
    QVERIFY(_mockJoystick->tapDPad(Joystick::HatUp));
    QCOMPARE(_mockJoystick->getHatDirection(0), Joystick::HatCentered);
    QVERIFY(_mockJoystick->tapDPad(Joystick::HatRightDown));
    QCOMPARE(_mockJoystick->getHatDirection(0), Joystick::HatCentered);
}

//-----------------------------------------------------------------------------
// Multi-Input Tests
//-----------------------------------------------------------------------------
void MockJoystickTest::_simultaneousAxesTest()
{
    _mockJoystick =
        std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Simultaneous Axes Test"), 6, 16, 1));
    QVERIFY(_mockJoystick->isValid());
    // Set multiple axes at once
    QVector<QPair<int, int>> axisValues = {{0, 10000}, {1, -5000}, {2, 20000}, {3, -15000}};
    QVERIFY(_mockJoystick->setAxes(axisValues));
    QCOMPARE(_mockJoystick->getAxisValue(0), 10000);
    QCOMPARE(_mockJoystick->getAxisValue(1), -5000);
    QCOMPARE(_mockJoystick->getAxisValue(2), 20000);
    QCOMPARE(_mockJoystick->getAxisValue(3), -15000);
}

//-----------------------------------------------------------------------------
// Reset Test
//-----------------------------------------------------------------------------
void MockJoystickTest::_resetTest()
{
    _mockJoystick = std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Reset Test"), 6, 16, 2));
    QVERIFY(_mockJoystick->isValid());
    // Set various inputs
    _mockJoystick->setLeftStick(20000, -15000);
    _mockJoystick->setRightStick(-10000, 25000);
    _mockJoystick->setLeftTrigger(30000);
    _mockJoystick->pressA();
    _mockJoystick->pressB();
    _mockJoystick->pressLeftShoulder();
    _mockJoystick->setHat(0, Joystick::HatUp);
    _mockJoystick->setHat(1, Joystick::HatRight);
    // Verify inputs are set
    QVERIFY(_mockJoystick->getAxisValue(0) != 0);
    QVERIFY(_mockJoystick->getButtonState(Joystick::ButtonA));
    QCOMPARE(_mockJoystick->getHatDirection(0), Joystick::HatUp);
    // Reset the joystick state (not the smart pointer)
    _mockJoystick.get()->reset();
    // Verify all reset
    for (int i = 0; i < 6; ++i) {
        QCOMPARE(_mockJoystick->getAxisValue(i), 0);
    }
    for (int i = 0; i < 16; ++i) {
        QVERIFY(!_mockJoystick->getButtonState(i));
    }
    QCOMPARE(_mockJoystick->getHatDirection(0), Joystick::HatCentered);
    QCOMPARE(_mockJoystick->getHatDirection(1), Joystick::HatCentered);
}

UT_REGISTER_TEST(MockJoystickTest, TestLabel::Unit, TestLabel::Joystick)
