#include "QGCCompressionFuzzTargets.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    return CompressionFuzz::fuzzStream(data, size);
}
