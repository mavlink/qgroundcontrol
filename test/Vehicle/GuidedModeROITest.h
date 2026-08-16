#pragma once

#include "BaseClasses/VehicleTest.h"

/// Verifies Vehicle::guidedModeROI altitude semantics: relativeAltitudeMeters is meters above
/// home for all firmwares. PX4 converts to AMSL (home + relative) and sends MAV_FRAME_GLOBAL
/// since PX4 treats the ROI altitude as AMSL regardless of frame; ArduPilot honors the frame,
/// so the relative value is sent unconverted in MAV_FRAME_GLOBAL_RELATIVE_ALT.
class GuidedModeROITest : public VehicleTest
{
    Q_OBJECT

public:
    explicit GuidedModeROITest(QObject* parent = nullptr) : VehicleTest(parent) {}

private slots:
    void _px4SendsAmslConvertedAltitude();
};

class GuidedModeROIAPMTest : public VehicleTestAPM
{
    Q_OBJECT

public:
    explicit GuidedModeROIAPMTest(QObject* parent = nullptr) : VehicleTestAPM(parent) {}

private slots:
    void _apmSendsRelativeAltitude();
};
