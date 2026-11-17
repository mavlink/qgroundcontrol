#pragma once

#include "GStreamerTestBase.h"

/// QGCQVideoSinkController binding qgcqvideosink to a QVideoSink: what it hands the element, and what it takes back
/// when its owner, the QVideoSink or the window goes away.
class GstQgcSinkControllerTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
#ifdef QGC_GST_STREAMING
    void _testQVideoSinkControllerReleasesElement_data();
    void _testQVideoSinkControllerReleasesElement();
    void _testQVideoSinkControllerNoWindowStartsInactive();
    void _testQVideoSinkControllerRebindRestoresActive_data();
    void _testQVideoSinkControllerRebindRestoresActive();
    void _testQVideoSinkControllerReleaseClearsVideoFrame();
#endif
};
