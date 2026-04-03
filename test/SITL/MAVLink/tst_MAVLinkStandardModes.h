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

/// Tests MAVLink AVAILABLE_MODES protocol against a real PX4 SITL instance.
/// Validates that QGC's StandardModes class correctly requests and populates
/// the flight mode list during the initial connection sequence.
class SITLStandardModesTest : public SITLTestBase
{
    Q_OBJECT

private slots:
    /// Verify AVAILABLE_MODES were populated during initial connect.
    void testAvailableModes();
};
