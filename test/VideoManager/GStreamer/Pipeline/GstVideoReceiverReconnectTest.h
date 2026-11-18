#pragma once

#include "GstVideoReceiverTestBase.h"

/// How GstVideoReceiver gives up on, or reconnects to, a stream that fails or stops.
class GstVideoReceiverReconnectTest : public GstVideoReceiverTestBase
{
    Q_OBJECT

private slots:
#ifdef QGC_GST_STREAMING
    void _testPermanentFailureStops();
    void _testReconnectBacksOffAfterFailedStart();
    void _testUnresponsiveRtspServer_data();
    void _testUnresponsiveRtspServer();
    void _testRtspUnauthorizedStops();
    void _testRtspWithoutVideoStreamReconnects();
    void _testFailureMessageNamesCause_data();
    void _testFailureMessageNamesCause();
    void _testStuckWorkerIsReported();
    void _testStuckWorkerKeepsMovingPicture();
    void _testBusyWorkerIsNotReported();
#endif
};
