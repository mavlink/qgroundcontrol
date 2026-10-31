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
    void _testStreamLossRestartPolicy_data();
    void _testStreamLossRestartPolicy();
    void _testUnresponsiveRtspServer_data();
    void _testUnresponsiveRtspServer();
#endif
};
