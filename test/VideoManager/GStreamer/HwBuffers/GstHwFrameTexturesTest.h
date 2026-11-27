#pragma once

#include "GStreamerTestBase.h"

/// The textures a GPU path hands Qt through its native handles (GstHwVideoBuffer::wrapTextures()), on the Null QRhi:
/// when they fail, and how long they keep the frame. Built with any GPU path.
class GstHwFrameTexturesTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
    void _testHwFrameTexturesNeedEveryPlane_data();
    void _testHwFrameTexturesNeedEveryPlane();
    void _testHwFrameTexturesKeepSampleUntilDestroyed();
};
