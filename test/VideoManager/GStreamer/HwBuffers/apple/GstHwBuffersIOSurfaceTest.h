#pragma once

#include "GStreamerTestBase.h"

/// The IOSurface path fed by VideoToolbox's decoder, read back through a Metal QRhi. Built on macOS with the path; the
/// test skips where the host has no Metal device or decodes no frame.
class GstHwBuffersIOSurfaceTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
    void _testIOSurfaceFrameToImage();
};
