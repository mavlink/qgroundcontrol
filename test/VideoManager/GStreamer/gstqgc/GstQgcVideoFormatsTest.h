#pragma once

#include "GStreamerTestBase.h"

/// qgcqvideosink's format tables, without an element: GStreamer formats, colorimetry and orientations mapped to Qt's,
/// and the formats the sink advertises.
class GstQgcVideoFormatsTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
#ifdef QGC_GST_STREAMING
    void _testColorimetryPixelFormatMapping();
    void _testCpuCapsFormatsRoundTripToQt();
    void _testColorimetryColorSpaceMapping();
    void _testColorimetryResolutionHeuristicMatchesQt();
    void _testColorimetryTransferMapping();
    void _testColorimetryColorRangeMapping();
    void _testPixelFormatAcceptedButNotAdvertised();
    void _testAdvertisedFormatListMatchesTable();
    void _testAdvertisedFormatListForOffer_data();
    void _testAdvertisedFormatListForOffer();
    void _testApplyOrientationToFrameMapping();
#endif
};
