#pragma once

#include "UnitTest.h"

/// Unit tests for ImageProtocolManager — the MAVLink image transmission
/// reassembly protocol (DATA_TRANSMISSION_HANDSHAKE + ENCAPSULATED_DATA).
///
/// ImageProtocolManager is self-contained: it consumes raw mavlink_message_t
/// frames and emits `imageReady` / `flowImageIndexChanged`. No Vehicle or
/// MockLink is needed — tests drive it by synthesizing encoded messages
/// directly, which is why it is an ideal unit test target.
class ImageProtocolManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testInitialState();
    void _testRequestImageEncodesHandshake();
    void _testRequestImageRejectedWhileInProgress();
    void _testCancelRequestEncodesZeroedHandshake();

    void _testRejectHandshakeWithZeroFields();
    void _testRejectHandshakeExceedingMaxImageSize();
    void _testRejectHandshakeWithOversizedPayload();

    void _testEncapsulatedDataWithoutHandshakeIgnored();
    void _testEncapsulatedDataPastEndIgnored();

    void _testCompleteImageEmitsImageReadyAndIndex();
    void _testMultipleImagesIncrementIndexIndependently();

    void _testUnrelatedMessageIdIgnored();
};
