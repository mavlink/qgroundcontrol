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

/// Tests MAVLink heartbeat detection and communication loss/recovery
/// against a real PX4 SITL instance.
class SITLHeartbeatTest : public SITLTestBase
{
    Q_OBJECT

private slots:
    /// Verify that QGC detects the SITL vehicle heartbeat and creates
    /// a Vehicle object with correct firmware type identification.
    void testDetection();

    /// Verify that QGC detects communication loss when the container
    /// stops, and recovers when a new container starts on the same port.
    void testLossAndReconnect();
};
