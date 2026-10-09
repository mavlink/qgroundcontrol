#pragma once

#include <chrono>
#include <memory>

#include "GPSCancellation.h"
#include "UDPGPSTransport.h"
#include "UnitTest.h"

class UDPGPSTransportTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _receivesSelectedSender();
    void _receivesIPv6Sender();
    void _idleSenderIsReplaced();
    void _receiveOnlyLink();
    void _cancelledRead();
    void _portInUse();

private:
    /// A transport open on a port that was free, which @a port reports; nullptr when no free port could be bound.
    std::unique_ptr<UDPGPSTransport> _openUnusedPort(
        quint16& port, GPSCancelToken cancelToken,
        std::chrono::milliseconds peerIdleTimeout = UDPGPSTransport::PEER_IDLE_TIMEOUT);
};
