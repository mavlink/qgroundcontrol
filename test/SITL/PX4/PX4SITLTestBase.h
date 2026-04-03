/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "SITLTestBase.h"

/// PX4-specific SITL test base class.
///
/// Adds flight command helpers (arm, takeoff, land, disarm) and telemetry
/// wait utilities on top of SITLTestBase. All PX4 knowledge (image name,
/// vehicle models, mode names) lives here.
class PX4SITLTestBase : public SITLTestBase
{
    Q_OBJECT

protected:
    // Flight command helpers — return true on success
    bool armVehicle(int timeoutMs = 10000);
    bool disarmVehicle(int timeoutMs = 10000);
    bool takeoff(float altitudeM, int timeoutMs = 30000);
    bool land(int timeoutMs = 60000);
    bool setFlightMode(const QString &mode, int timeoutMs = 5000);

    // Telemetry wait helpers
    bool waitForAltitude(float targetM, float toleranceM, int timeoutMs);
    bool waitForArmedState(bool armed, int timeoutMs);
};
