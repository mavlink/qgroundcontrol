#pragma once

#include "Protocols/Support/GPSProtocolTestBase.h"

/// Every receiver family's wire exchange, decode output and failure handling, pinned against reviewed transcripts.
class GPSGoldenTranscriptTest : public GPSProtocolTestBase
{
    Q_OBJECT

private slots:
    void _scenario_data();
    void _scenario();
    void _decode_data();
    void _decode();
    void _nmeaMatchesPassive_data();
    void _nmeaMatchesPassive();
    void _decodeOnlyArming_data();
    void _decodeOnlyArming();
    void _inventory();
};
