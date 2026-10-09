#pragma once

#include "UnitTest.h"

/// Decoded receiver data projected into position, integrity, satellite and survey reports.
class GPSDecodedDataTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _unreportedPosition();
    void _positionValues_data();
    void _positionValues();
    void _velocityValidity_data();
    void _velocityValidity();
    void _fixTypes_data();
    void _fixTypes();
    void _integrityStates_data();
    void _integrityStates();
    void _integrityReceipts();
    void _optionalValues_data();
    void _optionalValues();
    void _satelliteCounts_data();
    void _satelliteCounts();
    void _satelliteSnapshotScopes();
    void _satelliteSnapshotBounds();
    void _satelliteSnapshotExpiry();
    void _satelliteReceipts();
    void _satelliteUsageCombination();
    void _satelliteUsageExpiryFallback();
};
