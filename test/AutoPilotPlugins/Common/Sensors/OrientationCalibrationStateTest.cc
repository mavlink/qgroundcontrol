#include "OrientationCalibrationStateTest.h"
#include "OrientationCalibrationState.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void OrientationCalibrationStateTest::_testInitialState()
{
    OrientationCalibrationState state;

    // All sides should start as not done, not visible, not in progress
    QVERIFY(!state.isSideDone(CalibrationSide::Down));
    QVERIFY(!state.isSideDone(CalibrationSide::Up));
    QVERIFY(!state.isSideDone(CalibrationSide::Left));
    QVERIFY(!state.isSideDone(CalibrationSide::Right));
    QVERIFY(!state.isSideDone(CalibrationSide::Front));
    QVERIFY(!state.isSideDone(CalibrationSide::Back));

    QVERIFY(!state.isSideVisible(CalibrationSide::Down));
    QVERIFY(!state.isSideInProgress(CalibrationSide::Down));
    QVERIFY(!state.isSideRotate(CalibrationSide::Down));
}

void OrientationCalibrationStateTest::_testReset()
{
    OrientationCalibrationState state;

    // Set some state
    state.setVisibleSides(CalibrationSideMask::MaskAll);
    state.setSideInProgress(CalibrationSide::Down, true);
    state.setSideDone(CalibrationSide::Up);

    // Reset
    state.reset();

    // All should be reset
    QVERIFY(!state.isSideDone(CalibrationSide::Up));
    QVERIFY(!state.isSideVisible(CalibrationSide::Down));
    QVERIFY(!state.isSideInProgress(CalibrationSide::Down));
}

void OrientationCalibrationStateTest::_testResetAllDone()
{
    OrientationCalibrationState state;

    // Reset with allDone = true
    state.reset(true);

    // All sides should be done
    QVERIFY(state.isSideDone(CalibrationSide::Down));
    QVERIFY(state.isSideDone(CalibrationSide::Up));
    QVERIFY(state.isSideDone(CalibrationSide::Left));
    QVERIFY(state.isSideDone(CalibrationSide::Right));
    QVERIFY(state.isSideDone(CalibrationSide::Front));
    QVERIFY(state.isSideDone(CalibrationSide::Back));
}

void OrientationCalibrationStateTest::_testSetVisibleSides()
{
    OrientationCalibrationState state;

    // Set only Down and Up visible
    state.setVisibleSides(CalibrationSideMask::MaskDown | CalibrationSideMask::MaskUp);

    QVERIFY(state.isSideVisible(CalibrationSide::Down));
    QVERIFY(state.isSideVisible(CalibrationSide::Up));
    QVERIFY(!state.isSideVisible(CalibrationSide::Left));
    QVERIFY(!state.isSideVisible(CalibrationSide::Right));
    QVERIFY(!state.isSideVisible(CalibrationSide::Front));
    QVERIFY(!state.isSideVisible(CalibrationSide::Back));

    QCOMPARE(state.visibleSideCount(), 2);
}

void OrientationCalibrationStateTest::_testSetSideInProgress()
{
    OrientationCalibrationState state;

    state.setSideInProgress(CalibrationSide::Left, true);
    QVERIFY(state.isSideInProgress(CalibrationSide::Left));

    state.setSideInProgress(CalibrationSide::Left, false);
    QVERIFY(!state.isSideInProgress(CalibrationSide::Left));
}

void OrientationCalibrationStateTest::_testSetSideDone()
{
    OrientationCalibrationState state;

    // Set side in progress and rotating
    state.setSideInProgress(CalibrationSide::Right, true);
    state.setSideRotate(CalibrationSide::Right, true);

    QVERIFY(state.isSideInProgress(CalibrationSide::Right));
    QVERIFY(state.isSideRotate(CalibrationSide::Right));

    // Mark as done - should clear in progress and rotate
    state.setSideDone(CalibrationSide::Right);

    QVERIFY(state.isSideDone(CalibrationSide::Right));
    QVERIFY(!state.isSideInProgress(CalibrationSide::Right));
    QVERIFY(!state.isSideRotate(CalibrationSide::Right));
}

void OrientationCalibrationStateTest::_testSetSideRotate()
{
    OrientationCalibrationState state;

    state.setSideRotate(CalibrationSide::Front, true);
    QVERIFY(state.isSideRotate(CalibrationSide::Front));

    state.setSideRotate(CalibrationSide::Front, false);
    QVERIFY(!state.isSideRotate(CalibrationSide::Front));
}

void OrientationCalibrationStateTest::_testAllVisibleSidesDone()
{
    OrientationCalibrationState state;

    // Set 3 sides visible
    state.setVisibleSides(CalibrationSideMask::MaskDown | CalibrationSideMask::MaskUp | CalibrationSideMask::MaskLeft);

    QVERIFY(!state.allVisibleSidesDone());

    // Mark all visible sides as done
    state.setSideDone(CalibrationSide::Down);
    QVERIFY(!state.allVisibleSidesDone());

    state.setSideDone(CalibrationSide::Up);
    QVERIFY(!state.allVisibleSidesDone());

    state.setSideDone(CalibrationSide::Left);
    QVERIFY(state.allVisibleSidesDone());
}

void OrientationCalibrationStateTest::_testCompletedSideCount()
{
    OrientationCalibrationState state;

    // Set all sides visible
    state.setVisibleSides(CalibrationSideMask::MaskAll);

    QCOMPARE(state.completedSideCount(), 0);
    QCOMPARE(state.visibleSideCount(), 6);

    state.setSideDone(CalibrationSide::Down);
    QCOMPARE(state.completedSideCount(), 1);

    state.setSideDone(CalibrationSide::Up);
    state.setSideDone(CalibrationSide::Left);
    QCOMPARE(state.completedSideCount(), 3);
}

void OrientationCalibrationStateTest::_testCurrentSide()
{
    OrientationCalibrationState state;

    // Default should be Down
    QCOMPARE(state.currentSide(), CalibrationSide::Down);

    // Set Left in progress
    state.setSideInProgress(CalibrationSide::Left, true);
    QCOMPARE(state.currentSide(), CalibrationSide::Left);

    // Set multiple in progress - returns first
    state.setSideInProgress(CalibrationSide::Right, true);
    // Left comes before Right in enum, so Left should be returned
    QCOMPARE(state.currentSide(), CalibrationSide::Left);
}

void OrientationCalibrationStateTest::_testSignals()
{
    OrientationCalibrationState state;

    QSignalSpy doneChangedSpy(&state, &OrientationCalibrationState::sidesDoneChanged);
    QSignalSpy visibleChangedSpy(&state, &OrientationCalibrationState::sidesVisibleChanged);
    QSignalSpy inProgressChangedSpy(&state, &OrientationCalibrationState::sidesInProgressChanged);
    QSignalSpy rotateChangedSpy(&state, &OrientationCalibrationState::sidesRotateChanged);

    // Set visible - should emit sidesVisibleChanged
    state.setVisibleSides(CalibrationSideMask::MaskAll);
    QCOMPARE(visibleChangedSpy.count(), 1);

    // Set in progress - should emit sidesInProgressChanged
    state.setSideInProgress(CalibrationSide::Down, true);
    QCOMPARE(inProgressChangedSpy.count(), 1);

    // Set rotate - should emit sidesRotateChanged
    state.setSideRotate(CalibrationSide::Down, true);
    QCOMPARE(rotateChangedSpy.count(), 1);

    // Set done - should emit multiple signals
    state.setSideDone(CalibrationSide::Down);
    QCOMPARE(doneChangedSpy.count(), 1);
    QCOMPARE(inProgressChangedSpy.count(), 2); // Once more for clearing in progress
    QCOMPARE(rotateChangedSpy.count(), 2);     // Once more for clearing rotate
}
