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

/// Tests MAVLink mission protocol against a real PX4 SITL instance.
/// Validates mission upload/download round-trip with retransmission
/// handling over real UDP.
class SITLMissionTest : public SITLTestBase
{
    Q_OBJECT

private slots:
    /// Upload a 5-waypoint mission, download it back, and compare.
    void testUploadDownload();
};
