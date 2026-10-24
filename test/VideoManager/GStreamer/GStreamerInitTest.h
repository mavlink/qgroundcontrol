#pragma once

#include "UnitTest.h"

/// GStreamer init policy: when GStreamer's log is bridged into Qt logging.
class GStreamerInitTest : public UnitTest
{
    Q_OBJECT

private slots:
#ifdef QGC_GST_STREAMING
    void _testNativeDebugLogRequested_data();
    void _testNativeDebugLogRequested();
    void _testNativeDebugLogIsNotBridged();
#else
    void _testGStreamerDisabled();
#endif
};
