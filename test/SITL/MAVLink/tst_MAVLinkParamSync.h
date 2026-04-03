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

/// Tests MAVLink parameter protocol against a real PX4 SITL instance.
/// Validates parameter download completeness and modification round-trip
/// over real UDP (subject to packet loss, reordering, retries).
class SITLParamSyncTest : public SITLTestBase
{
    Q_OBJECT

private slots:
    /// Verify all parameters are downloaded without gaps.
    void testFullDownload();

    /// Verify parameter write → read-back round-trip.
    void testModifyRoundTrip();
};
