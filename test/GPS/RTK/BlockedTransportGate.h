#pragma once

#include <atomic>
#include <memory>

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
    return [gate](const std::atomic_bool& stop) {
        gate->entered.release();
        gate->release.acquire();
        gate->sawCancellation = stop.load();
        return std::unique_ptr<GPSTransport>{};
    };
}
