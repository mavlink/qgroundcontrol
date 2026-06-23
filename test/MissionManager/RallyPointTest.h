#pragma once

#include <QtPositioning/QGeoCoordinate>

#include "UnitTest.h"

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
    void _testTextFieldFactsExposeThreeAxes();
    void _testGetDefaultFactAltitudeNonNegative();
};
