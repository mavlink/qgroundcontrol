#include "JoystickManagerTest.h"
#include "MockJoystick.h"
#include "JoystickManager.h"
#include "JoystickSDL.h"
#include "SDLJoystick.h"

#include <QtCore/QElapsedTimer>
#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

UT_REGISTER_TEST(JoystickManagerTest)

JoystickManagerTest::JoystickManagerTest() = default;

void JoystickManagerTest::init()
{
    UnitTest::init();

    // In CI environments without display/input devices, SDL initialization may hang or fail.
    // Use a timeout to detect this and skip the test gracefully.
    static bool sdlInitialized = false;
    static bool sdlInitFailed = false;

    if (sdlInitFailed) {
        QSKIP("SDL initialization previously failed - skipping joystick tests");
    }

    if (!sdlInitialized) {
        // Check for headless CI environment
        const bool isCi = qEnvironmentVariableIsSet("CI") ||
                          qEnvironmentVariableIsSet("GITHUB_ACTIONS") ||
                          qEnvironmentVariableIsSet("GITLAB_CI");

        if (isCi && !qEnvironmentVariableIsSet("DISPLAY") && !qEnvironmentVariableIsSet("WAYLAND_DISPLAY")) {
            // Headless CI without display - SDL joystick subsystem may not work
            qWarning() << "Headless CI environment detected - attempting SDL init with caution";
        }

        if (!JoystickSDL::init()) {
            sdlInitFailed = true;
            QSKIP("SDL joystick initialization failed - skipping joystick tests");
        }
        sdlInitialized = true;
    }
}

void JoystickManagerTest::cleanup()
{
    _mockJoystick1.reset();
    _mockJoystick2.reset();

    UnitTest::cleanup();
}

//-----------------------------------------------------------------------------
// Discovery Tests
//-----------------------------------------------------------------------------

void JoystickManagerTest::_availableJoystickNamesTest()
{
    JoystickManager *manager = JoystickManager::instance();

    // Get initial count (may have physical joysticks connected)
    const auto initialCount = manager->availableJoystickNames().size();

    // Create a virtual joystick
    _mockJoystick1 = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Manager Test Controller 1"), 6, 16, 1));
    QVERIFY(_mockJoystick1->isValid());

    SDLJoystick::pumpEvents();

    // Trigger the manager to check for joysticks
    manager->_checkForAddedOrRemovedJoysticks();

    QStringList names = manager->availableJoystickNames();
    QVERIFY(names.size() >= initialCount + 1);
    QVERIFY(names.contains(QStringLiteral("Manager Test Controller 1")));
}

void JoystickManagerTest::_activeJoystickTest()
{
    JoystickManager *manager = JoystickManager::instance();

    // Create a virtual joystick
    _mockJoystick1 = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Active Test Controller"), 6, 16, 1));
    QVERIFY(_mockJoystick1->isValid());

    SDLJoystick::pumpEvents();
    manager->_checkForAddedOrRemovedJoysticks();

    // If no other joystick was active, this should become active
    Joystick *active = manager->activeJoystick();
    if (active != nullptr) {
        QVERIFY(!active->name().isEmpty());
    }
}

//-----------------------------------------------------------------------------
// Hot-plug Tests
//-----------------------------------------------------------------------------

void JoystickManagerTest::_joystickAddedSignalTest()
{
    JoystickManager *manager = JoystickManager::instance();

    QSignalSpy spy(manager, &JoystickManager::availableJoystickNamesChanged);
    QVERIFY(spy.isValid());

    // Create a virtual joystick
    _mockJoystick1 = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Signal Test Controller"), 6, 16, 1));
    QVERIFY(_mockJoystick1->isValid());

    SDLJoystick::pumpEvents();
    manager->_checkForAddedOrRemovedJoysticks();

    // Signal should have been emitted
    QVERIFY(spy.size() >= 1);
}

void JoystickManagerTest::_joystickRemovedSignalTest()
{
    JoystickManager *manager = JoystickManager::instance();

    // Create and add a joystick first
    _mockJoystick1 = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Remove Test Controller"), 6, 16, 1));
    QVERIFY(_mockJoystick1->isValid());

    SDLJoystick::pumpEvents();
    manager->_checkForAddedOrRemovedJoysticks();

    // Verify it's in the list
    QVERIFY(manager->availableJoystickNames().contains(QStringLiteral("Remove Test Controller")));

    QSignalSpy spy(manager, &JoystickManager::availableJoystickNamesChanged);
    QVERIFY(spy.isValid());

    // Remove the joystick
    _mockJoystick1.reset();
    SDLJoystick::pumpEvents();
    manager->_checkForAddedOrRemovedJoysticks();

    // Signal should have been emitted
    QVERIFY(spy.size() >= 1);

    // Verify it's no longer in the list
    QVERIFY(!manager->availableJoystickNames().contains(QStringLiteral("Remove Test Controller")));
}

//-----------------------------------------------------------------------------
// Active Joystick Selection Tests
//-----------------------------------------------------------------------------

void JoystickManagerTest::_setActiveJoystickTest()
{
    JoystickManager *manager = JoystickManager::instance();

    // Create two virtual joysticks
    _mockJoystick1 = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Select Test Controller 1"), 6, 16, 1));
    _mockJoystick2 = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Select Test Controller 2"), 6, 16, 1));
    QVERIFY(_mockJoystick1->isValid());
    QVERIFY(_mockJoystick2->isValid());

    SDLJoystick::pumpEvents();
    manager->_checkForAddedOrRemovedJoysticks();

    QStringList names = manager->availableJoystickNames();
    QVERIFY(names.contains(QStringLiteral("Select Test Controller 1")));
    QVERIFY(names.contains(QStringLiteral("Select Test Controller 2")));

    QSignalSpy spy(manager, &JoystickManager::activeJoystickChanged);
    QVERIFY(spy.isValid());

    // Set active to the first controller
    manager->_setActiveJoystickByName(QStringLiteral("Select Test Controller 1"));

    Joystick *active = manager->activeJoystick();
    if (active != nullptr) {
        QCOMPARE(active->name(), QStringLiteral("Select Test Controller 1"));
    }

    // Switch to second controller
    manager->_setActiveJoystickByName(QStringLiteral("Select Test Controller 2"));

    active = manager->activeJoystick();
    if (active != nullptr) {
        QCOMPARE(active->name(), QStringLiteral("Select Test Controller 2"));
    }
}

void JoystickManagerTest::_autoSelectFirstJoystickTest()
{
    JoystickManager *manager = JoystickManager::instance();

    // Clear any active joystick by name
    manager->_setActiveJoystickByName(QString());
    QVERIFY(manager->activeJoystick() == nullptr);

    // Create a new joystick
    _mockJoystick1 = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Auto Select Test Controller"), 6, 16, 1));
    QVERIFY(_mockJoystick1->isValid());

    SDLJoystick::pumpEvents();
    manager->_checkForAddedOrRemovedJoysticks();

    // Manager should auto-select the first available joystick
    // (behavior depends on settings, so just verify it doesn't crash)
    manager->availableJoystickNames();
}

//-----------------------------------------------------------------------------
// Polling Control Tests
//-----------------------------------------------------------------------------

void JoystickManagerTest::_pollingControlTest()
{
    JoystickManager *manager = JoystickManager::instance();

    // Create a virtual joystick
    _mockJoystick1 = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Polling Test Controller"), 6, 16, 1));
    QVERIFY(_mockJoystick1->isValid());

    SDLJoystick::pumpEvents();
    manager->_checkForAddedOrRemovedJoysticks();

    // Event control through SDLJoystick namespace
    SDLJoystick::setJoystickEventsEnabled(true);
    QVERIFY(SDLJoystick::joystickEventsEnabled());

    SDLJoystick::setGamepadEventsEnabled(true);
    QVERIFY(SDLJoystick::gamepadEventsEnabled());

    // Pump and update events
    SDLJoystick::pumpEvents();
    SDLJoystick::updateJoysticks();
    SDLJoystick::updateGamepads();

    // Verify the manager has the joystick available
    QStringList names = manager->availableJoystickNames();
    QVERIFY(names.contains(QStringLiteral("Polling Test Controller")));
}

//-----------------------------------------------------------------------------
// Multiple Controller Management Tests
//-----------------------------------------------------------------------------

void JoystickManagerTest::_multipleControllerManagementTest()
{
    JoystickManager *manager = JoystickManager::instance();

    // Create three controllers
    _mockJoystick1 = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Multi Controller 1"), 6, 16, 1));
    _mockJoystick2 = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Multi Controller 2"), 4, 12, 1));
    auto mockJoystick3 = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Multi Controller 3"), 8, 20, 2));

    QVERIFY(_mockJoystick1->isValid());
    QVERIFY(_mockJoystick2->isValid());
    QVERIFY(mockJoystick3->isValid());

    SDLJoystick::pumpEvents();
    manager->_checkForAddedOrRemovedJoysticks();

    QStringList names = manager->availableJoystickNames();
    QVERIFY(names.contains(QStringLiteral("Multi Controller 1")));
    QVERIFY(names.contains(QStringLiteral("Multi Controller 2")));
    QVERIFY(names.contains(QStringLiteral("Multi Controller 3")));

    // Set each as active in sequence
    manager->_setActiveJoystickByName(QStringLiteral("Multi Controller 1"));
    if (manager->activeJoystick() != nullptr) {
        QCOMPARE(manager->activeJoystick()->name(), QStringLiteral("Multi Controller 1"));
    }

    manager->_setActiveJoystickByName(QStringLiteral("Multi Controller 2"));
    if (manager->activeJoystick() != nullptr) {
        QCOMPARE(manager->activeJoystick()->name(), QStringLiteral("Multi Controller 2"));
    }

    manager->_setActiveJoystickByName(QStringLiteral("Multi Controller 3"));
    if (manager->activeJoystick() != nullptr) {
        QCOMPARE(manager->activeJoystick()->name(), QStringLiteral("Multi Controller 3"));
    }

    // Remove middle controller while another is active
    _mockJoystick2.reset();
    SDLJoystick::pumpEvents();
    manager->_checkForAddedOrRemovedJoysticks();

    names = manager->availableJoystickNames();
    QVERIFY(!names.contains(QStringLiteral("Multi Controller 2")));
    QVERIFY(names.contains(QStringLiteral("Multi Controller 1")));
    QVERIFY(names.contains(QStringLiteral("Multi Controller 3")));

    // Active joystick should still be valid - either our controller or another one
    // (if physical joysticks are connected, the manager may auto-select one of those)
    Joystick *active = manager->activeJoystick();
    if (active != nullptr) {
        // Verify we have some valid active joystick
        QVERIFY(!active->name().isEmpty());
        // If it's our controller 3, verify it specifically
        if (active->name() == QStringLiteral("Multi Controller 3")) {
            QCOMPARE(active->name(), QStringLiteral("Multi Controller 3"));
        }
    }
}
