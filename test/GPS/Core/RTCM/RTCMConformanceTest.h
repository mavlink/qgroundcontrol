#pragma once

#include "RTCMFramer.h"
#include "UnitTest.h"

/// RTCM3 framing against fixed CRC vectors and a corpus that the frame decoder and framer must agree on, and the
/// per-message counts of a stream.
class RTCMConformanceTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _crc24q_data();
    void _crc24q();
    void _sharedCorpus_data();
    void _sharedCorpus();
    void _strictValidation_data();
    void _strictValidation();
    void _frameFields_data();
    void _frameFields();
    void _fragmentReceiptAndFiltering();
    void _repeatedMalformedPreambles();
    void _queuedResultDelivery();
    void _messageCountsChangeInPlace();

signals:
    void decoded(RTCMDecodedFrame result);
};
