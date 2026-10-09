#pragma once

#include "UnitTest.h"

/// The stream demultiplexer: family routing, resynchronization, interleaving and chunking.
class GPSStreamDemuxTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _familyRouting_data();
    void _familyRouting();
    void _resync();
    void _longSBFBlockIsDiscarded();
    void _rtcmPayloadsAreOpaque();
    void _interleaved();
    void _chunkingIndependence_data();
    void _chunkingIndependence();
    void _enableRestartsFramer();
};
