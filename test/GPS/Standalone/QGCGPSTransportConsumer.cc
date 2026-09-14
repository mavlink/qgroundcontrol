#include "GPSTransport.h"

class UnsupportedTransport final : public GPSTransport
{
public:
    using GPSTransport::GPSTransport;

    OpenResult open() override { return {OpenStatus::Unsupported}; }

    bool fatalError() const override { return true; }

    ReadResult read(uint8_t*, int, int) override { return {ReadStatus::Closed}; }

    bool setBaudrate(unsigned) override { return false; }
};

int main()
{
    std::atomic_bool stop = false;
    UnsupportedTransport transport(stop);
    const uint8_t byte = 1;
    if (transport.write(&byte, 1).status != GPSTransport::WriteStatus::Unsupported) {
        return 1;
    }
    stop = true;
    if (transport.write(&byte, 1).status != GPSTransport::WriteStatus::Cancelled) {
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
