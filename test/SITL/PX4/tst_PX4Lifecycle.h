/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "PX4SITLTestBase.h"

/// Tests the full PX4 vehicle lifecycle against a real SITL instance:
/// arm → takeoff → hold → land → disarm → disconnect.
class PX4LifecycleTest : public PX4SITLTestBase
{
    Q_OBJECT

private slots:
    /// Full flight lifecycle: arm, takeoff to 10m, hold 5s, land, auto-disarm.
    void testArmTakeoffLandDisarm();

    /// Verify AUTOPILOT_VERSION identifies PX4 firmware correctly.
    void testFirmwareIdentification();
};
