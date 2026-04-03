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

/// Tests PX4 flight mode transitions against a real SITL instance.
/// Verifies that QGC's mode display matches the actual PX4 mode.
class PX4ModesTest : public PX4SITLTestBase
{
    Q_OBJECT

private slots:
    /// Cycle through flight modes and verify QGC display matches.
    void testTransitions();
};
