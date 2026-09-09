#pragma once

#include <QtCore/QThread>

#include <atomic>
#include <functional>
#include <memory>

#include "GPSType.h"

class GPSTransport;

/// One-shot worker that releases its transport before the NMEA connection opens it.
class GPSNMEAPreparation : public QThread
{
    Q_OBJECT

public:
    using TransportFactory = std::function<std::unique_ptr<GPSTransport>(const std::atomic_bool&)>;

    GPSNMEAPreparation(TransportFactory factory, GPSType type, QObject* parent = nullptr);
    ~GPSNMEAPreparation() override;

    void stop() { _requestStop = true; }

    unsigned baudrate() const { return _baudrate.load(); }

private:
    void run() override;

    TransportFactory _factory;
    GPSType _type;
    std::atomic_bool _requestStop = false;
    std::atomic_uint _baudrate = 0;
};
