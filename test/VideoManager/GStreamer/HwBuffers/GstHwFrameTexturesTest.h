#pragma once

#include "GStreamerTestBase.h"

/// The frame-texture bundles a GPU path hands Qt, on the Null QRhi: which old bundle is reused, what invalidates one,
/// and how long it keeps its frame. Built with any GPU path.
class GstHwFrameTexturesTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
    void _testHwFrameTexturesRejectsForeignOldTextures();
    void _testHwFrameTexturesZeroHandleInvalidatesBundle_data();
    void _testHwFrameTexturesZeroHandleInvalidatesBundle();
    void _testHwFrameTexturesFinishMapReusesAcceptedBundle_data();
    void _testHwFrameTexturesFinishMapReusesAcceptedBundle();
    void _testHwFrameTexturesFinishMapRejectsIncompleteBundle();
    void _testHwFrameTexturesKeepResourcesUntilDestroyed();
};
