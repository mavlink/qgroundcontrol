#pragma once

#include <atomic>
#include <memory>
#include <stop_token>

#include <QtCore/QSemaphore>

#include "GPSProvider.h"
#include "GPSTransport.h"

struct BlockedTransportGate
{
    QSemaphore entered;
    QSemaphore release;
    std::atomic_bool sawCancellation = false;
};

inline GPSProvider::TransportFactory blockedTransportFactory(const std::shared_ptr<BlockedTransportGate>& gate)
{
    return [gate](std::stop_token stopToken) {
        gate->entered.release();
        gate->release.acquire();
        gate->sawCancellation = stopToken.stop_requested();
        return std::unique_ptr<GPSTransport>{};
    };
}
