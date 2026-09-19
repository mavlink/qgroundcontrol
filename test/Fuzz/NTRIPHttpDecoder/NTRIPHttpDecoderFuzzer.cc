#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <QtCore/QByteArrayView>
#include <QtCore/QDateTime>

#include "NTRIPHttpDecoder.h"

namespace {
constexpr size_t MAX_INPUT_BYTES = 65536;

void require(bool condition)
{
    if (!condition) {
        std::abort();
    }
}

void verifyReset(NTRIPHttpDecoder& decoder, const QDateTime& now)
{
    decoder.reset();
    const auto fresh = decoder.feed("HTTP/1.1 200 OK\r\nContent-Length: 1\r\n\r\nx", now);
    require(fresh.connected && fresh.complete && !fresh.failure && fresh.body == "x");
}
}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size > MAX_INPUT_BYTES) {
        return 0;
    }
    const QByteArrayView wire(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size));
    static const auto now = QDateTime::fromString(QStringLiteral("2026-09-18T12:00:00Z"), Qt::ISODate);
    NTRIPHttpDecoder decoder;
    const std::array<qsizetype, 4> fragments{1, 7, std::max<qsizetype>(1, wire.size()), 0};
    for (const qsizetype fragment : fragments) {
        decoder.reset();
        qsizetype bodyBytes = 0;
        int connections = 0;
        int failures = 0;
        const auto consume = [&](const NTRIPHttpDecoder::Result& result) {
            bodyBytes += result.body.size();
            connections += result.connected;
            require(bodyBytes <= wire.size() && connections <= 1);
            if (result.failure) {
                ++failures;
                require(failures == 1 && !result.complete);
                require(result.failure->retryAfter >= std::chrono::milliseconds::zero());
                require(result.failure->retryAfter <= std::chrono::minutes(5));
            }
        };
        consume(decoder.feed({}, now));
        for (qsizetype offset = 0; offset < wire.size();) {
            const qsizetype stride = fragment ? fragment : 1 + static_cast<unsigned char>(wire[offset]);
            const qsizetype length = std::min(stride, wire.size() - offset);
            consume(decoder.feed(wire.sliced(offset, length), now));
            offset += length;
        }
        consume(decoder.finish());
        const auto finishedAgain = decoder.finish();
        require(!finishedAgain.connected && !finishedAgain.failure && finishedAgain.body.isEmpty());
        verifyReset(decoder, now);
    }
    decoder.reset();
    decoder.feed(wire.first(wire.size() / 2), now);
    verifyReset(decoder, now);
    return 0;
}
