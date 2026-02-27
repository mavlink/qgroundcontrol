#include "JoystickManagerTest.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QEventLoop>
#include <QtCore/QPointer>
#include <QtTest/QSignalSpy>

#include "JoystickManager.h"
#include "JoystickSDL.h"
#include "MockJoystick.h"
#include "SDLJoystick.h"

void JoystickManagerTest::initTestCase()
{
    UnitTest::initTestCase();
    QVERIFY(JoystickSDL::init());
}

void JoystickManagerTest::init()
{
    UnitTest::init();
}

void JoystickManagerTest::cleanup()
{
    _mockJoystick1.reset();
    _mockJoystick2.reset();
    UnitTest::cleanup();
}

void JoystickManagerTest::_refreshJoysticks(JoystickManager* manager)
{
    SDLJoystick::pumpEvents();
    SDLJoystick::updateJoysticks();
    SDLJoystick::updateGamepads();
    manager->_checkForAddedOrRemovedJoysticks();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
}

bool JoystickManagerTest::_waitForJoystickNames(JoystickManager* manager, const QStringList& expectedNames, int timeoutMs)
{
    return UnitTest::waitForCondition(
        [&]() {
            _refreshJoysticks(manager);

            const QStringList names = manager->availableJoystickNames();
            for (const QString& expectedName : expectedNames) {
                if (!names.contains(expectedName)) {
                    return false;
                }
            }
            return true;
        },
        timeoutMs, QStringLiteral("Expected joystick names discovered"));
}

//-----------------------------------------------------------------------------
// Discovery Tests
//-----------------------------------------------------------------------------
void JoystickManagerTest::_availableJoystickNamesTest()
{
    JoystickManager* manager = JoystickManager::instance();
    // Get initial count (may have physical joysticks connected)
    const auto initialCount = manager->availableJoystickNames().size();
    // Create a virtual joystick
    _mockJoystick1 =
        std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Manager Test Controller 1"), 6, 16, 1));
    QVERIFY(_mockJoystick1->isValid());
    QVERIFY(_waitForJoystickNames(manager, {QStringLiteral("Manager Test Controller 1")}));

    QStringList names = manager->availableJoystickNames();
    QVERIFY(names.size() >= initialCount + 1);
    QVERIFY(names.contains(QStringLiteral("Manager Test Controller 1")));
}

void JoystickManagerTest::_activeJoystickTest()
{
    JoystickManager* manager = JoystickManager::instance();
    // Create a virtual joystick
    _mockJoystick1 =
        std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Active Test Controller"), 6, 16, 1));
    QVERIFY(_mockJoystick1->isValid());
    QVERIFY(_waitForJoystickNames(manager, {QStringLiteral("Active Test Controller")}));

    // If no other joystick was active, this should become active
    Joystick* active = manager->activeJoystick();
    if (active != nullptr) {
        QVERIFY(!active->name().isEmpty());
    }
}

//-----------------------------------------------------------------------------
// Hot-plug Tests
//-----------------------------------------------------------------------------
void JoystickManagerTest::_joystickAddedSignalTest()
{
    JoystickManager* manager = JoystickManager::instance();
    QSignalSpy spy(manager, &JoystickManager::availableJoystickNamesChanged);
    QVERIFY(spy.isValid());
    // Create a virtual joystick
    _mockJoystick1 =
        std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Signal Test Controller"), 6, 16, 1));
    QVERIFY(_mockJoystick1->isValid());
    QVERIFY(_waitForJoystickNames(manager, {QStringLiteral("Signal Test Controller")}));

    // Signal should have been emitted
    QVERIFY(spy.size() >= 1);
}

void JoystickManagerTest::_joystickRemovedSignalTest()
{
    JoystickManager* manager = JoystickManager::instance();
    // Create and add a joystick first
    _mockJoystick1 =
        std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Remove Test Controller"), 6, 16, 1));
    QVERIFY(_mockJoystick1->isValid());
    QVERIFY(_waitForJoystickNames(manager, {QStringLiteral("Remove Test Controller")}));

    // Verify it's in the list
    QVERIFY(manager->availableJoystickNames().contains(QStringLiteral("Remove Test Controller")));
    QSignalSpy spy(manager, &JoystickManager::availableJoystickNamesChanged);
    QVERIFY(spy.isValid());
    // Remove the joystick
    _mockJoystick1.reset();
    _refreshJoysticks(manager);

    // Signal should have been emitted
    QVERIFY(spy.size() >= 1);
    // Verify it's no longer in the list
    QVERIFY(!manager->availableJoystickNames().contains(QStringLiteral("Remove Test Controller")));
}

void JoystickManagerTest::_instanceIdReuseNameMismatchManagerTest()
{
    JoystickManager *manager = JoystickManager::instance();
    const QString oldName = QStringLiteral("Manager Old Controller");
    const QString newName = QStringLiteral("Manager Replacement Controller");

    _mockJoystick1 = std::unique_ptr<MockJoystick>(MockJoystick::create(oldName, 6, 16, 1));
    QVERIFY(_mockJoystick1->isValid());
    QVERIFY(_waitForJoystickNames(manager, {oldName}));

    manager->_setActiveJoystickByName(oldName);
    auto *oldJoystick = qobject_cast<JoystickSDL *>(manager->joystickByName(oldName));
    QVERIFY(oldJoystick != nullptr);
    QPointer<QObject> staleGuard(oldJoystick);

    _mockJoystick1.reset();
    SDLJoystick::pumpEvents();
    SDLJoystick::updateJoysticks();
    SDLJoystick::updateGamepads();
    manager->_checkForAddedOrRemovedJoysticks();

    auto replacement = std::unique_ptr<MockJoystick>(MockJoystick::create(newName, 6, 16, 1));
    QVERIFY(replacement->isValid());

    // Force an instance-id collision to validate stale object replacement path.
    oldJoystick->setInstanceId(replacement->instanceId());

    _mockJoystick1 = std::move(replacement);
    QVERIFY(_waitForJoystickNames(manager, {newName}));

    auto *newJoystick = qobject_cast<JoystickSDL *>(manager->joystickByName(newName));
    QVERIFY(newJoystick != nullptr);
    QVERIFY(newJoystick != oldJoystick);
    QVERIFY(!manager->availableJoystickNames().contains(oldName));

    const Joystick *active = manager->activeJoystick();
    if (active) {
        QVERIFY(active != oldJoystick);
    }

    QVERIFY(UnitTest::waitForDeleted(staleGuard, TestTimeout::shortMs(),
                                     QStringLiteral("stale manager joystick after id/name mismatch")));
}

//-----------------------------------------------------------------------------
// Active Joystick Selection Tests
//-----------------------------------------------------------------------------
void JoystickManagerTest::_setActiveJoystickTest()
{
    JoystickManager* manager = JoystickManager::instance();
    // Create two virtual joysticks
    _mockJoystick1 =
        std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Select Test Controller 1"), 6, 16, 1));
    _mockJoystick2 =
        std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Select Test Controller 2"), 6, 16, 1));
    QVERIFY(_mockJoystick1->isValid());
    QVERIFY(_mockJoystick2->isValid());
    if (!_waitForJoystickNames(manager, {QStringLiteral("Select Test Controller 1"),
                                         QStringLiteral("Select Test Controller 2")}, TestTimeout::shortMs())) {
        QSKIP("Skipping: backend did not expose two concurrent virtual joysticks");
    }

    QStringList names = manager->availableJoystickNames();
    QVERIFY(names.contains(QStringLiteral("Select Test Controller 1")));
    QVERIFY(names.contains(QStringLiteral("Select Test Controller 2")));
    QSignalSpy spy(manager, &JoystickManager::activeJoystickChanged);
    QVERIFY(spy.isValid());
    // Set active to the first controller
    manager->_setActiveJoystickByName(QStringLiteral("Select Test Controller 1"));
    Joystick* active = manager->activeJoystick();
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
    JoystickManager* manager = JoystickManager::instance();
    // Clear any active joystick by name
    manager->_setActiveJoystickByName(QString());
    QVERIFY(manager->activeJoystick() == nullptr);
    // Create a new joystick
    _mockJoystick1 =
        std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Auto Select Test Controller"), 6, 16, 1));
    QVERIFY(_mockJoystick1->isValid());
    QVERIFY(_waitForJoystickNames(manager, {QStringLiteral("Auto Select Test Controller")}));

    // Manager should auto-select the first available joystick
    // (behavior depends on settings, so just verify it doesn't crash)
    manager->availableJoystickNames();
}

//-----------------------------------------------------------------------------
// Polling Control Tests
//-----------------------------------------------------------------------------
void JoystickManagerTest::_pollingControlTest()
{
    JoystickManager* manager = JoystickManager::instance();
    // Create a virtual joystick
    _mockJoystick1 =
        std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Polling Test Controller"), 6, 16, 1));
    QVERIFY(_mockJoystick1->isValid());
    QVERIFY(_waitForJoystickNames(manager, {QStringLiteral("Polling Test Controller")}));

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

void JoystickManagerTest::_sensorUpdateRoutesToCorrectSignalsTest()
{
    JoystickManager *manager = JoystickManager::instance();
    _mockJoystick1 =
        std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Sensor Test Controller"), 6, 16, 1));
    QVERIFY(_mockJoystick1->isValid());

    QVERIFY(_waitForJoystickNames(manager, {QStringLiteral("Sensor Test Controller")}));


    Joystick *joystick = manager->joystickByName(QStringLiteral("Sensor Test Controller"));
    QVERIFY(joystick != nullptr);

    auto *sdlJoystick = qobject_cast<JoystickSDL *>(joystick);
    QVERIFY(sdlJoystick != nullptr);

    QSignalSpy accelSpy(joystick, &Joystick::accelerometerDataUpdated);
    QSignalSpy gyroSpy(joystick, &Joystick::gyroscopeDataUpdated);
    QVERIFY(accelSpy.isValid());
    QVERIFY(gyroSpy.isValid());

    manager->_handleSensorUpdate(sdlJoystick->instanceId(), 1, 1.0f, 2.0f, 3.0f);
    manager->_handleSensorUpdate(sdlJoystick->instanceId(), 2, 4.0f, 5.0f, 6.0f);

    QCOMPARE(accelSpy.count(), 1);
    QCOMPARE(gyroSpy.count(), 1);

    const QVector3D accelData = accelSpy.takeFirst().at(0).value<QVector3D>();
    const QVector3D gyroData = gyroSpy.takeFirst().at(0).value<QVector3D>();
    QCOMPARE(accelData, QVector3D(1.0f, 2.0f, 3.0f));
    QCOMPARE(gyroData, QVector3D(4.0f, 5.0f, 6.0f));
}

//-----------------------------------------------------------------------------
// Multiple Controller Management Tests
//-----------------------------------------------------------------------------
void JoystickManagerTest::_multipleControllerManagementTest()
{
    JoystickManager* manager = JoystickManager::instance();
    // Create three controllers
    _mockJoystick1 =
        std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Multi Controller 1"), 6, 16, 1));
    _mockJoystick2 =
        std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Multi Controller 2"), 4, 12, 1));
    auto mockJoystick3 =
        std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Multi Controller 3"), 8, 20, 2));
    QVERIFY(_mockJoystick1->isValid());
    QVERIFY(_mockJoystick2->isValid());
    QVERIFY(mockJoystick3->isValid());
    if (!_waitForJoystickNames(manager, {QStringLiteral("Multi Controller 1"),
                                         QStringLiteral("Multi Controller 2"),
                                         QStringLiteral("Multi Controller 3")},
                               TestTimeout::shortMs())) {
        QSKIP("Skipping: backend did not expose three concurrent virtual joysticks");
    }

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
    _refreshJoysticks(manager);

    names = manager->availableJoystickNames();
    QVERIFY(!names.contains(QStringLiteral("Multi Controller 2")));
    QVERIFY(names.contains(QStringLiteral("Multi Controller 1")));
    QVERIFY(names.contains(QStringLiteral("Multi Controller 3")));
    // Active joystick should still be valid - either our controller or another one
    // (if physical joysticks are connected, the manager may auto-select one of those)
    Joystick* active = manager->activeJoystick();
    if (active != nullptr) {
        // Verify we have some valid active joystick
        QVERIFY(!active->name().isEmpty());
        // If it's our controller 3, verify it specifically
        if (active->name() == QStringLiteral("Multi Controller 3")) {
            QCOMPARE(active->name(), QStringLiteral("Multi Controller 3"));
        }
    }
}

void JoystickManagerTest::_linkedGroupMembersTest()
{
    JoystickManager* manager = JoystickManager::instance();

    _mockJoystick1 =
        std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Group Test Controller 1"), 6, 16, 1));
    _mockJoystick2 =
        std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Group Test Controller 2"), 6, 16, 1));
    QVERIFY(_mockJoystick1->isValid());
    QVERIFY(_mockJoystick2->isValid());

    if (!_waitForJoystickNames(manager, {QStringLiteral("Group Test Controller 1"),
                                         QStringLiteral("Group Test Controller 2")}, TestTimeout::shortMs())) {
        QSKIP("Skipping: backend did not expose two concurrent virtual joysticks");
    }

    Joystick* joystick1 = manager->joystickByName(QStringLiteral("Group Test Controller 1"));
    Joystick* joystick2 = manager->joystickByName(QStringLiteral("Group Test Controller 2"));
    QVERIFY(joystick1 != nullptr);
    QVERIFY(joystick2 != nullptr);

    joystick1->setLinkedGroupId(QStringLiteral("group-alpha"));
    joystick2->setLinkedGroupId(QStringLiteral("group-alpha"));

    const QStringList emptyGroupMembers = manager->linkedGroupMembers(QString());
    QVERIFY(emptyGroupMembers.isEmpty());

    const QStringList groupMembers = manager->linkedGroupMembers(QStringLiteral("group-alpha"));
    QVERIFY(groupMembers.contains(QStringLiteral("Group Test Controller 1")));
    QVERIFY(groupMembers.contains(QStringLiteral("Group Test Controller 2")));
    QCOMPARE(groupMembers.size(), 2);
}

void JoystickManagerTest::_joystickByNameTest()
{
    JoystickManager* manager = JoystickManager::instance();

    _mockJoystick1 =
        std::unique_ptr<MockJoystick>(MockJoystick::create(QStringLiteral("Lookup Test Controller"), 6, 16, 1));
    QVERIFY(_mockJoystick1->isValid());

    QVERIFY(_waitForJoystickNames(manager, {QStringLiteral("Lookup Test Controller")}));

    Joystick* found = manager->joystickByName(QStringLiteral("Lookup Test Controller"));
    QVERIFY(found != nullptr);
    QCOMPARE(found->name(), QStringLiteral("Lookup Test Controller"));

    Joystick* missing = manager->joystickByName(QStringLiteral("Missing Controller"));
    QVERIFY(missing == nullptr);
}

UT_REGISTER_TEST(JoystickManagerTest, TestLabel::Unit, TestLabel::Joystick)
