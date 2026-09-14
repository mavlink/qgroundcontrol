#include <QtCore/QBuffer>

#include "DataRateTracker.h"
#include "ReadTimestamp.h"

class TimestampedBuffer : public QBuffer, public ReadTimestamp
{
public:
    quint64 receipt = 0;

    quint64 lastReadTimestampUs() const override { return receipt; }
};

int main()
{
    DataRateTracker rate;
    rate.recordBytes(1);
    QBuffer ordinary;
    TimestampedBuffer timestamped;
    constexpr quint64 FALLBACK = 123;
    if (rate.totalBytes() != 1 || ReadTimestamp::from(&ordinary, FALLBACK) != FALLBACK ||
        ReadTimestamp::from(nullptr, FALLBACK) != FALLBACK || ReadTimestamp::from(&timestamped, FALLBACK) != 0)
        return 1;
    timestamped.receipt = 42;
    if (ReadTimestamp::from(&timestamped, FALLBACK) != timestamped.receipt)
        return 1;
    const auto before = MonotonicClock::nowUs();
    const auto receipt = ReadTimestamp::from(&ordinary);
    return receipt >= before && receipt <= MonotonicClock::nowUs() ? 0 : 1;
}
