#pragma once

#include "GStreamerTestBase.h"

/// The pipeline-building helpers of GStreamerHelpers.h, and the TeeBranchHead every tee branch starts with.
class GStreamerHelpersTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
#ifdef QGC_GST_STREAMING
    void _testAddToBin();
    void _testAddToBinRejects();
    void _testDisposeChild();
    void _testFlushAndDisposeChildFlushes();
    void _testTeeBranchHead();
    void _testRequestKeyframe();
    void _testSendEventAsync();
    void _testFirstSrcPad();
    void _testHoldPadUntilLinked();
    void _testForEachPlugin();
#endif
};
