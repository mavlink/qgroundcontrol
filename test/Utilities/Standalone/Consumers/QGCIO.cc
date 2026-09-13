#include "DataRateTracker.h"
#include "TimestampedByteBuffer.h"

int main()
{
    DataRateTracker rate;
    rate.recordBytes(1);
    TimestampedByteBuffer buffer;
    buffer.append("x", 10);
    char byte = 0;
    const auto result = buffer.read(&byte, 1, 10, 100);
    return rate.totalBytes() == 1 && result.bytes == 1 && byte == 'x' ? 0 : 1;
}
