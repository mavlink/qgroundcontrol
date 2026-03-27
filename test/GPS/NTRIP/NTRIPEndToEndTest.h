#pragma once

#include "UnitTest.h"

#include <QtNetwork/QTcpServer>

// End-to-end pipeline tests with no mocks in the middle: a local TCP caster
// feeds the real NTRIPHttpTransport -> RTCMParser -> RTCMMavlink chain and
// asserts against the MAVLink bytes emitted at the far end. For mock-backed
// manager-API tests see NTRIPIntegrationTest.
class NTRIPEndToEndTest : public UnitTest
{
    Q_OBJECT

private slots:
    // Full pipeline: local TCP caster -> NTRIPHttpTransport -> RTCMParser -> RTCMMavlink -> MAVLink
    void testRtcmThroughFullPipeline();
    void testMultipleRtcmFramesBatched();
    void testLargeRtcmFragmented();
    void testAuthFailureEndToEnd();
    void testNmeaGgaSentToCaster();
    void testWhitelistFiltersMessages();
};
