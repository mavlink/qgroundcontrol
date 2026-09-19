#include <type_traits>

#include "GPSTransport.h"

#if defined(QT_NETWORK_LIB) || defined(QT_POSITIONING_LIB) || defined(QT_QML_LIB) || defined(QT_SERIALPORT_LIB)
#error Transport contracts must not inherit concrete transport or application dependencies.
#endif

static_assert(std::is_enum_v<GPSOpenStatus>);
static_assert(std::is_enum_v<GPSReadStatus>);
static_assert(std::is_enum_v<GPSWriteStatus>);
static_assert(std::is_enum_v<GPSBaudStatus>);
static_assert(GPSReadStatus::Data != GPSReadStatus::TimedOut);
static_assert(GPSWriteStatus::Completed != GPSWriteStatus::Unsupported);

class UnsupportedTransport final : public GPSTransport
{
public:
    using GPSTransport::GPSTransport;

    GPSOpenResult open() override { return {GPSOpenStatus::Unsupported}; }

    bool fatalError() const override { return true; }

    GPSReadResult read(uint8_t*, int, int) override { return {GPSReadStatus::Closed}; }

    bool setBaudrate(unsigned) override { return false; }
};

int main()
{
    std::atomic_bool stop = false;
    UnsupportedTransport transport(stop);
    const uint8_t byte = 1;
    if (transport.writeConfiguration(&byte, 1, QDeadlineTimer(transport.configurationWriteTimeout())).status !=
        GPSWriteStatus::Unsupported) {
        return 1;
    }
    stop = true;
    if (transport.writeConfiguration(&byte, 1, QDeadlineTimer(transport.configurationWriteTimeout())).status !=
        GPSWriteStatus::Cancelled) {
        return 2;
    }
    struct Allowance
    {
        int length;
        qint64 baud;
        qint64 expectedMs;
    };

    const Allowance cases[] = {{1029, 9600, 1172}, {1029, 38400, 368}, {0, 9600, 200},   {-1, 9600, 200},
                               {1029, 0, 200},     {1029, -1, 200},    {1, 115200, 101}, {1000000, 9600, 3000}};
    for (const auto& value : cases) {
        if (GPSTransport::serialCorrectionWriteTimeout(value.length, value.baud).count() != value.expectedMs) {
            return 3;
        }
    }
    return 0;
}
