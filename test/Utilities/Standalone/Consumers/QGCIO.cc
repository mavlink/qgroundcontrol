#include "DataRateTracker.h"

int main()
{
    DataRateTracker rate;
    rate.recordBytes(1);
    return rate.totalBytes() == 1 ? 0 : 1;
}
