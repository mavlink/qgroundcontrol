#pragma once

#include "GStreamerTestBase.h"

/// qgcqvideosink and the sink bin as elements, without a pipeline: construction, caps, allocation, CPU zero-copy
/// frames, and frames pushed straight into the sink. And what the sink is made of, on its own: the GPU bin's choice and
/// a frame's geometry.
class GstQgcSinkTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
    void _testSinkBinConfigReachesSink();
#if defined(QGC_HAS_ANY_GPU_PATH)
    void _testQgcVideoSinkBinGpuBinVariant_data();
    void _testQgcVideoSinkBinGpuBinVariant();
#endif
    void _testQgcVideoSinkBinRefusesDrawingD3D11Converter_data();
    void _testQgcVideoSinkBinRefusesDrawingD3D11Converter();
    void _testQgcQVideoSinkDeliversLatestFrameOnly();
    void _testQgcQVideoSinkDisplayedCountNeverFalls();
    void _testQgcQVideoSinkHoldsTwoPoolBuffersWhileGuiStalls();
    void _testRenderBackendNames();
    void _testRenderTargetSharesGlContext();
    void _testCurrentRenderTarget();
    void _testQgcQVideoSinkAppliesCropMeta();
    void _testQgcQVideoSinkShowsPictureOfLargerBuffer();
    void _testQgcQVideoSinkAppliesAffineTransformationMeta_data();
    void _testQgcQVideoSinkAppliesAffineTransformationMeta();
    void _testQgcQVideoSinkStatsArePerSink();
    void _testQgcQVideoSinkKeepsBaseSinkStats();
#if defined(QGC_HAS_ANY_GPU_PATH)
    void _testQgcQVideoSinkGpuPathSelection_data();
    void _testQgcQVideoSinkGpuPathSelection();
#endif
    void _testQgcQVideoSinkCapsFollowRenderTarget_data();
    void _testQgcQVideoSinkCapsFollowRenderTarget();
    void _testQgcQVideoSinkReconfiguresOnRenderTargetChange();
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    void _testQgcQVideoSinkReconfiguresOnDmaBufImportChange();
#endif
    void _testQgcQVideoSinkDropsUndeliveredFrame_data();
    void _testQgcQVideoSinkDropsUndeliveredFrame();
    void _testQgcQVideoSinkForwardsTimestamps();
    void _testQgcQVideoSinkMapFailuresPostElementError_data();
    void _testQgcQVideoSinkMapFailuresPostElementError();
    void _testCpuZeroCopyFrameRejectsWritableMap();
    void _testAllocationQueryHwMemoryPoolHint_data();
    void _testAllocationQueryHwMemoryPoolHint();
    void _testAllocationQueryOffersCropMeta_data();
    void _testAllocationQueryOffersCropMeta();
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    void _testQgcVideoSinkBinAllocationQueryAdvertisesVideoMeta();
#endif
    void _testQgcQVideoSinkSystemMemoryAllocation_data();
    void _testQgcQVideoSinkSystemMemoryAllocation();
    void _testQgcQVideoSinkAllocationNeedsCaps();
    void _testAdapterFlushDropsInFlightSamples();
    void _testFrameDeliveryKeepsTheLatestFrame();
    void _testPathSessionFailurePolicy();
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    void _testGpuBinPreferenceParse_data();
    void _testGpuBinPreferenceParse();
#endif
    void _testGpuBinSelection_data();
    void _testGpuBinSelection();
    void _testFrameGeometryFollowsBufferMetas_data();
    void _testFrameGeometryFollowsBufferMetas();
};
