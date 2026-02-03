#include "Viewer3DManagerTest.h"
#include "Viewer3DManager.h"

#include <QtTest/QSignalSpy>

void Viewer3DManagerTest::_testInitialState()
{
    Viewer3DManager mgr;

    QCOMPARE(mgr.displayMode(), Viewer3DManager::Map);
    QVERIFY(mgr.mapProvider() != nullptr);
    QVERIFY(!mgr.gpsRef().isValid());
}

void Viewer3DManagerTest::_testSetDisplayMode()
{
    Viewer3DManager mgr;
    QSignalSpy spy(&mgr, &Viewer3DManager::displayModeChanged);
    QVERIFY(spy.isValid());

    // Map -> Map should be a no-op
    mgr.setDisplayMode(Viewer3DManager::Map);
    QCOMPARE(spy.count(), 0);
}

void Viewer3DManagerTest::_testSetDisplayModeNoop()
{
    Viewer3DManager mgr;
    QSignalSpy spy(&mgr, &Viewer3DManager::displayModeChanged);
    QVERIFY(spy.isValid());

    // Switching to same mode does not emit
    mgr.setDisplayMode(Viewer3DManager::Map);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(mgr.displayMode(), Viewer3DManager::Map);
}

UT_REGISTER_TEST(Viewer3DManagerTest, TestLabel::Unit)
