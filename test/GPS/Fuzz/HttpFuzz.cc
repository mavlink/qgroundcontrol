#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include "NTRIPHttpDecoder.h"

namespace {
struct Decoded
{
    QByteArray body;
    int connections = 0;
    bool complete = false;
    int error = -1;
    int status = 0;
    bool operator==(const Decoded&) const = default;
};

Decoded decode(QByteArrayView data, qsizetype fragment)
{
    NTRIPHttpDecoder parser;
    Decoded decoded;
    const auto collect = [&](const NTRIPHttpDecoder::Result& result) {
        decoded.body += result.body;
        decoded.connections += result.connected;
        decoded.complete |= result.complete;
        if (result.failure) {
            decoded.error = static_cast<int>(result.failure->code);
            decoded.status = result.failure->httpStatus;
        }
        if (parser.bufferedBytes() > NTRIPHttpDecoder::MAX_LINE_BYTES || decoded.body.size() > data.size()) {
            std::abort();
        }
    };
    for (qsizetype offset = 0; offset < data.size() && decoded.error < 0; offset += fragment) {
        collect(parser.feed(data.sliced(offset, std::min(fragment, data.size() - offset))));
    }
    if (decoded.error < 0) {
        collect(parser.finish());
    }
    return decoded;
}
}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* bytes, size_t size)
{
    if (size > 65536) {
        return 0;
    }
    const QByteArrayView data(reinterpret_cast<const char*>(bytes), static_cast<qsizetype>(size));
    const auto whole = decode(data, qMax(qsizetype(1), data.size()));
    const auto fragmented = decode(data, size ? 1 + bytes[size - 1] % 127 : 1);
    if (!(whole == fragmented)) {
        std::abort();
    }
    return 0;
}
