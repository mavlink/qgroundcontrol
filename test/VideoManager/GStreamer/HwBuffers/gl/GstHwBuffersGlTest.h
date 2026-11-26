#pragma once

#include "GStreamerTestBase.h"

/// The sink fed by gst-gl pipelines: GLMemory from glupload, and software frames through the glupload bin. Each test
/// skips where the host has no GL context for it.
class GstHwBuffersGlTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
    void _testGlMemoryDispatch_data();
    void _testGlMemoryDispatch();
#if defined(QGC_GST_BIN_HAS_GLUPLOAD)
    void _testGlUploadBinTakesSoftwareFrames_data();
    void _testGlUploadBinTakesSoftwareFrames();
#endif
};
