#pragma once

#include "BaseClasses/VehicleTest.h"

/// Regression tests for Vehicle::setEstimatorOrigin.
///
/// setEstimatorOrigin prefers MAV_CMD_DO_SET_GLOBAL_ORIGIN (sent as a COMMAND_INT)
/// and falls back to the deprecated SET_GPS_GLOBAL_ORIGIN message when the command
/// is reported unsupported by the vehicle. These tests exercise all three branches
/// of the command-support cache: cached-unsupported, cached-supported, and the
/// unknown -> probe -> unsupported-ack -> fallback path.
class SetEstimatorOriginTest : public VehicleTest
{
    Q_OBJECT

public:
    explicit SetEstimatorOriginTest(QObject* parent = nullptr) : VehicleTest(parent) {}

private slots:
    void _cachedUnsupported_sendsLegacyMessageOnly();
    void _cachedSupported_sendsCommandIntOnly();
    void _probeUnsupported_fallsBackAndCachesUnsupported();
};
