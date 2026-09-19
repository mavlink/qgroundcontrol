#include <atomic>
#include <iostream>

#include "GPSDriver.h"
#include "GPSTransport.h"

namespace {
class UnopenedTransport final : public GPSTransport
{
public:
    explicit UnopenedTransport(const std::atomic_bool& cancelled)
        : GPSTransport(cancelled)
    {}

    GPSOpenResult open() override
    {
        ++operations;
        return {GPSOpenStatus::Error};
    }

    bool fatalError() const override { return false; }

    GPSReadResult read(uint8_t*, int, int) override
    {
        ++operations;
        return {GPSReadStatus::Error};
    }

    GPSWriteResult write(const uint8_t*, int) override
    {
        ++operations;
        return {GPSWriteStatus::Error};
    }

    bool setBaudrate(unsigned) override
    {
        ++operations;
        return false;
    }

    int operations = 0;
};
}  // namespace

int main()
{
    const std::atomic_bool cancelled{false};
    UnopenedTransport transport(cancelled);
    GPSDriver driver(GPSType::septentrio, transport, {.role = GPSReceiverConfig::Role::Position}, {});
    if (driver.receive(0) >= 0 || transport.operations != 0) {
        std::cerr << "An unconfigured driver must not receive or touch the transport\n";
        return 1;
    }
    if (driver.configure() || transport.operations != 0 || driver.receive(0) >= 0) {
        std::cerr << "An unsupported Position role must fail before receiver I/O\n";
        return 2;
    }
    return 0;
}
