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

/// Tests MAVLink COMMAND_LONG / COMMAND_ACK protocol against a real PX4 SITL.
class SITLCommandTest : public SITLTestBase
{
    Q_OBJECT

private slots:
    /// Verify that a simple command receives a proper COMMAND_ACK.
    void testAckHandling();

    /// Verify that attempting to arm before preflight checks pass
    /// results in a DENIED ACK that QGC surfaces correctly.
    void testRejection();
};
