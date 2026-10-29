#pragma once

#include <QtCore/QList>
#include <QtPositioning/QGeoCoordinate>

#include "UnitTest.h"

class RallyPoint;

/// Unit tests for RallyPoint — the small value object that wraps a
/// QGeoCoordinate as three Facts (latitude / longitude / altitude) so it can
/// be exposed through a QmlObjectListModel.
///
/// Previously untested despite being used by RallyPointController to manage
/// the in-memory rally-point list. Tests cover construction, coordinate
/// round-trips, copy/assign, dirty-flag semantics, the
/// `coordinateChanged` signal, and the static
/// `getDefaultFactAltitude` helper.
class RallyPointTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testCoordinateConstruction();
    void _testCopyConstruction();
    void _testAssignmentUpdatesAllAxes();
    void _testSetCoordinateEmitsSignalAndMarksDirty();
    void _testSetCoordinateIdenticalIsNoOp();
    void _testDirtyFlagTransitions();
    void _testFactsExposeEachAxis();
    void _testGetDefaultFactAltitudeNonNegative();
    void _testMavFrameMapping_data();
    void _testMavFrameMapping();
    void _testAltitudeFrameSurvivesConstructionCopyAndAssignment();
    void _testSetAltitudeFrameMarksDirty();
    void _testChangeAltitudeFrameResetsAltitude_data();
    void _testChangeAltitudeFrameResetsAltitude();
    void _testTerrainAltitudeSeedsAbsoluteFrame();
    void _testTerrainFailureLeavesPointUnchanged();
    void _testEditingAltitudeStopsFollowingTerrain();
    void _testEditingAltitudeDiscardsPendingTerrainAnswer();
    void _testTerrainAnswerForMovedPointIsDiscarded();
    void _testUseTerrainAltitudeSupersedesPendingMoveRequest();
    void _testTerrainQueryPending();
    void _testSelectingFrameCancelsPendingTerrainAnswer();
    void _testExplicitAbsoluteRequestSwitchesFrameWhenTerrainFails();

private:
    /// Delivers a terrain answer as if it were for the point's current position
    static void _answerTerrainQuery(RallyPoint& rp, bool success, const QList<double>& heights);
};
