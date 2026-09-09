#include "GPSNMEAPreparation.h"

#include <utility>

#include "GPSDriver.h"
#include "GPSTransport.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSNMEAPreparationLog, "GPS.Driver.GPSNMEAPreparation")

GPSNMEAPreparation::GPSNMEAPreparation(TransportFactory factory, GPSType type, QObject* parent)
    : QThread(parent)
    , _factory(std::move(factory))
    , _type(type)
{
    qCDebug(GPSNMEAPreparationLog) << this;
}

GPSNMEAPreparation::~GPSNMEAPreparation()
{
    qCDebug(GPSNMEAPreparationLog) << this;
    stop();
    wait();
}

void GPSNMEAPreparation::run()
{
    auto factory = std::exchange(_factory, {});
    if (_requestStop || !factory) {
        return;
    }
    auto transport = factory(_requestStop);
    if (_requestStop || !transport || !transport->open() || _requestStop) {
        return;
    }
    GPSReceiverConfig config;
    config.role = GPSReceiverConfig::Role::Position;
    config.outputProtocol = GPSReceiverConfig::OutputProtocol::NMEA;
    GPSDriver driver(_type, *transport, config, {});
    if (driver.configure() && !_requestStop) {
        _baudrate = driver.baudrate();
    }
}
