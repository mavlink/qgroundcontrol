#pragma once

#include "GStreamerTestBase.h"

/// The pipeline-building helpers of GStreamerHelpers.h.
class GStreamerHelpersTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
#ifdef QGC_GST_STREAMING
    void _testAddToBin();
    void _testAddToBinRejects();
    void _testDisposeChild();
    void _testFlushAndDisposeChildFlushes();
    void _testAddTeeBranch();
    void _testRequestKeyframe();
    void _testSendEventAsync();
    void _testFirstSrcPad();
    void _testHoldPadUntilLinked();
    void _testForEachPlugin();
#endif
};
