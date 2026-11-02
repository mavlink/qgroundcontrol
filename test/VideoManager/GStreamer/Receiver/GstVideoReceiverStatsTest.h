#pragma once

#include "GstVideoReceiverTestBase.h"

/// The stream statistics GstVideoReceiver measures from real H.264 and H.265 streams.
class GstVideoReceiverStatsTest : public GstVideoReceiverTestBase
{
    Q_OBJECT

private slots:
#ifdef QGC_GST_STREAMING
    void _testStatsMeasureStream_data();
    void _testStatsMeasureStream();
#endif
};
