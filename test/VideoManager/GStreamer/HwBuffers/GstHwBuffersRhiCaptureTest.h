#pragma once

#include "GStreamerTestBase.h"

/// QGCRhiCapture under a real scene graph: Qt Quick on OpenGL with the threaded render loop, which CTest asks for (the
/// render loop is chosen once per process). Built on Linux with a GPU path; skips where Qt Quick renders in software or
/// has no OpenGL.
class GstHwBuffersRhiCaptureTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
    void _testDevicePublishedAfterSceneGraphReinitialises();
};
