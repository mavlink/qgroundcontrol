#pragma once

#include "Protocols/Support/GPSProtocolTestBase.h"

/// Decodes slices of real receiver logs (fixtures/*.cap, whose sources fixtures/SOURCES.md lists) and compares what
/// the receiver families report with what the pinned tools decode from the same bytes
/// (fixtures/GPSCaptureExpectations.h). Every capture is decoded whole, byte by byte and in chunks of seven bytes.
class GPSCaptureDecodeTest : public GPSProtocolTestBase
{
    Q_OBJECT

private slots:
    void _ubloxPositions_data();
    void _ubloxPositions();
    void _corrections_data();
    void _corrections();
    void _stationPositions();
    void _surveyStatus_data();
    void _surveyStatus();
    void _identity();
    void _acknowledgements_data();
    void _acknowledgements();
    void _ggaPositions_data();
    void _ggaPositions();
    void _ashtechPositions_data();
    void _ashtechPositions();
};
