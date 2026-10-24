#pragma once

#include "GStreamerTestBase.h"

/// qgcqvideosink and qgcvideosinkbin as elements, without a pipeline: construction, caps, allocation, the format and
/// colorimetry tables, the QVideoSink controller, and frames pushed straight into the sink.
class GstQgcSinkTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
#ifdef QGC_GST_STREAMING
    void _testQgcVideoSinkBinGpuZeroCopyProperty();
    void _testQgcVideoSinkBinGpuBinVariant_data();
    void _testQgcVideoSinkBinGpuBinVariant();
    void _testQgcVideoSinkBinRejectsFailedAdopt();
    void _testQgcBinChainFailedAdoptDropsFloatingRefOnce();
    void _testQgcQVideoSinkDeliversLatestFrameOnly();
    void _testQgcQVideoSinkRenegotiatesMidStream();
    void _testQgcQVideoSinkAppliesCropMeta();
    void _testQgcQVideoSinkAppliesOrientationTag_data();
    void _testQgcQVideoSinkAppliesOrientationTag();
    void _testQgcQVideoSinkStatsArePerSink();
    void _testQgcQVideoSinkKeepsBaseSinkStats();
    void _testQgcQVideoSinkGpuPathSelection_data();
    void _testQgcQVideoSinkGpuPathSelection();
    void _testQgcQVideoSinkCapsFollowRenderTarget_data();
    void _testQgcQVideoSinkCapsFollowRenderTarget();
    void _testQgcQVideoSinkReconfiguresOnRenderTargetChange();
    void _testQgcQVideoSinkReconfiguresOnDmaBufImportChange();
    void _testQgcQVideoSinkDropsUndeliveredFrame_data();
    void _testQgcQVideoSinkDropsUndeliveredFrame();
    void _testQgcQVideoSinkForwardsTimestamps();
    void _testQgcQVideoSinkMapFailuresPostElementError();
    void _testQgcVideoSinkBinQVideoSinkAccessor();
    void _testQVideoSinkControllerClearsElementOnDestroy();
    void _testQVideoSinkControllerClearsElementWhenVideoSinkDestroyed();
    void _testQVideoSinkControllerNullSinkStillDeactivatesOnDestroy();
    void _testQVideoSinkControllerRepeatedSetupKeepsNewBindingActive();
    void _testQVideoSinkControllerNoWindowStartsInactive();
    void _testQVideoSinkControllerReleaseClearsVideoFrame();
    void _testCpuZeroCopyFrameRejectsWritableMap();
    void _testCpuZeroCopyPinnedFramesCappedPerSink();
    void _testColorimetryPixelFormatMapping();
    void _testCpuCapsFormatsRoundTripToQt();
    void _testAllocationQueryHwMemoryPoolHint_data();
    void _testAllocationQueryHwMemoryPoolHint();
    void _testQgcVideoSinkBinAllocationQueryAdvertisesVideoMeta();
    void _testQgcQVideoSinkSystemMemoryAllocation_data();
    void _testQgcQVideoSinkSystemMemoryAllocation();
    void _testColorimetryColorSpaceMapping();
    void _testColorimetryResolutionHeuristicMatchesQt();
    void _testColorimetryTransferMapping();
    void _testApplyOrientationToFrameMapping();
    void _testAdapterFlushDropsInFlightSamples();
    void _testColorimetryColorRangeMapping();
    void _testPixelFormatAcceptedButNotAdvertised();
    void _testAdvertisedFormatListMatchesTable();
    void _testAdvertisedFormatListForOffer_data();
    void _testAdvertisedFormatListForOffer();
#endif
};
