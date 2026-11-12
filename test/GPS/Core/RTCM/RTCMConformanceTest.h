#pragma once

#include "RTCMFramer.h"
#include "UnitTest.h"

/// RTCM3 framing against fixed CRC vectors and a corpus that the frame decoder and framer must agree on, the base
/// station position of messages 1005 and 1006, and the header of the MSM observation messages.
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
    void _fragmentReceipt();
    void _repeatedMalformedPreambles();
    void _queuedResultDelivery();
    void _stationPosition_data();
    void _stationPosition();
    void _msmHeader_data();
    void _msmHeader();

signals:
    void decoded(RTCMDecodedFrame result);
};
