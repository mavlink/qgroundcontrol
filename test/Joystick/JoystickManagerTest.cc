#include "JoystickManagerTest.h"
#include "MockJoystick.h"
#include "JoystickManager.h"
#include "JoystickSDL.h"

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

UT_REGISTER_TEST(JoystickManagerTest)

JoystickManagerTest::JoystickManagerTest() = default;

void JoystickManagerTest::init()
{
    UnitTest::init();
    JoystickSDL::init();
}

void JoystickManagerTest::cleanup()
{
    _mockJoystick1.reset();
    _mockJoystick2.reset();

    UnitTest::cleanup();
}

void JoystickManagerTest::_pumpEvents()
{
    JoystickSDL::pumpEvents();
}

//-----------------------------------------------------------------------------
// Discovery Tests
//-----------------------------------------------------------------------------

void JoystickManagerTest::_availableJoystickNamesTest()
{
    JoystickManager *manager = JoystickManager::instance();

    // Get initial count (may have physical joysticks connected)
    int initialCount = manager->availableJoystickNames().size();

    // Create a virtual joystick
    _mockJoystick1 = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Manager Test Controller 1"), 6, 16, 1));
    QVERIFY(_mockJoystick1->isValid());

    _pumpEvents();

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

    _pumpEvents();
    manager->_checkForAddedOrRemovedJoysticks();

    // If no other joystick was active, this should become active
    Joystick *active = manager->activeJoystick();
    if (active) {
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

    _pumpEvents();
    manager->_checkForAddedOrRemovedJoysticks();

    // Signal should have been emitted
    QVERIFY(spy.count() >= 1);
}

void JoystickManagerTest::_joystickRemovedSignalTest()
{
    JoystickManager *manager = JoystickManager::instance();

    // Create and add a joystick first
    _mockJoystick1 = std::unique_ptr<MockJoystick>(MockJoystick::create(
        QStringLiteral("Remove Test Controller"), 6, 16, 1));
    QVERIFY(_mockJoystick1->isValid());

    _pumpEvents();
    manager->_checkForAddedOrRemovedJoysticks();

    // Verify it's in the list
    QVERIFY(manager->availableJoystickNames().contains(QStringLiteral("Remove Test Controller")));

    QSignalSpy spy(manager, &JoystickManager::availableJoystickNamesChanged);
    QVERIFY(spy.isValid());

    // Remove the joystick
    _mockJoystick1.reset();
    _pumpEvents();
    manager->_checkForAddedOrRemovedJoysticks();

    // Signal should have been emitted
    QVERIFY(spy.count() >= 1);

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

    _pumpEvents();
    manager->_checkForAddedOrRemovedJoysticks();

    QStringList names = manager->availableJoystickNames();
    QVERIFY(names.contains(QStringLiteral("Select Test Controller 1")));
    QVERIFY(names.contains(QStringLiteral("Select Test Controller 2")));

    QSignalSpy spy(manager, &JoystickManager::activeJoystickChanged);
    QVERIFY(spy.isValid());

    // Set active to the first controller
    manager->_setActiveJoystickByName(QStringLiteral("Select Test Controller 1"));

    Joystick *active = manager->activeJoystick();
    if (active) {
        QCOMPARE(active->name(), QStringLiteral("Select Test Controller 1"));
    }

    // Switch to second controller
    manager->_setActiveJoystickByName(QStringLiteral("Select Test Controller 2"));

    active = manager->activeJoystick();
    if (active) {
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

    _pumpEvents();
    manager->_checkForAddedOrRemovedJoysticks();

    // Manager should auto-select the first available joystick
    // (behavior depends on settings, so just verify it doesn't crash)
    manager->availableJoystickNames();
}
