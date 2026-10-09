#pragma once

#include "Protocols/Support/GPSProtocolTestBase.h"

/// Receiver detection: baud candidates, signature and probe detection, and cancellation.
class GPSReceiverDetectorTest : public GPSProtocolTestBase
{
    Q_OBJECT

private slots:
    void _baudCandidates();
    void _signatureDetection_data();
    void _signatureDetection();
    void _probeDetection_data();
    void _probeDetection();
    void _standardNMEAEndsSearch();
    void _fixedRateProbesEveryFamily();
    void _cancellation();
};
