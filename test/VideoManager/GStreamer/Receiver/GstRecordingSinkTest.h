#pragma once

#include "GStreamerTestBase.h"

/// The recording branch's file sink fed by running pipelines, with each file played back to check it.
class GstRecordingSinkTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
#ifdef QGC_GST_STREAMING
    void _testRecordingSinkFinalizesMidStreamH265Mp4();
    void _testRecordingSinkTimestampsDtsOnlyFrames();
#endif
};
