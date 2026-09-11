#include <QtCore/QCoreApplication>

#include "GPSDriver.h"
#include "GPSReceiverFamily.h"
#include "GPSTransport.h"

class NoDevice final : public GPSTransport
{
public:
    using GPSTransport::GPSTransport;

    OpenResult open() override { return {OpenStatus::Error}; }

    bool fatalError() const override { return true; }

    ReadResult read(uint8_t*, int, int) override { return {ReadStatus::Error}; }

    bool setBaudrate(unsigned) override { return false; }
};

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    std::atomic_bool stopped = true;
    NoDevice transport(stopped);
    for (const auto& family : gpsReceiverFamilies()) {
        GPSReceiverConfig config;
        config.role = GPSReceiverConfig::Role::Position;
        GPSDriver driver(family.type, transport, config, {});
        if (driver.configure())
            return 1;
    }
    return gpsReceiverFamilies().empty() ? 1 : 0;
}
