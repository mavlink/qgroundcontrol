#pragma once

#include "UnitTest.h"

/// Tests VehicleEstimatorStatusFactGroup's handling of ESTIMATOR_STATUS, and its fallback
/// to ArduPilot's EKF_STATUS_REPORT.
class VehicleEstimatorStatusFactGroupTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _estimatorStatus();
    void _ekfStatusReportFallback();
    void _ekfStatusReportUninitializedIsNotGpsGlitch();
    void _estimatorStatusPreferred();
};
