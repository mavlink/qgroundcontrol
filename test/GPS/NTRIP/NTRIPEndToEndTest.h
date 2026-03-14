#pragma once

#include "UnitTest.h"

#include <QtNetwork/QTcpServer>

class NTRIPEndToEndTest : public UnitTest
{
    Q_OBJECT

private slots:
    // Full pipeline: local TCP caster -> NTRIPHttpTransport -> RTCMParser -> RTCMRouter -> MAVLink
    void testRtcmThroughFullPipeline();
    void testMultipleRtcmFramesBatched();
    void testLargeRtcmFragmented();
    void testAuthFailureEndToEnd();
    void testNmeaGgaSentToCaster();
    void testWhitelistFiltersMessages();
};
