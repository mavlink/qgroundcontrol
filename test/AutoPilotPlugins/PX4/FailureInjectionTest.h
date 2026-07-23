#pragma once

#include "UnitTest.h"

/// Tests for the FailureInjection QML singleton's data model: the MAVLink-derived
/// FAILURE_UNIT/FAILURE_TYPE catalog, the activity log, and injected-unit tracking.
class FailureInjectionTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _catalogPopulatedFromMavlinkEnums();
    void _logRowAddsPendingEntryWithoutTracking();
    void _logInjectionTracksUnitOnce();
    void _resolveResultResolvesOldestPendingRow();
    void _resolveResultIgnoresInProgress();
    void _resolveResultUnknownCodeFallsBackToMavResultString();
    void _clearInjectedUnitsForgetsTrackedUnits();
    void _detailParamsMapCombos();
};
