#include <chrono>
#include <cstdlib>
#include <memory>
#include <vector>

#include "GPSCommandChannel.h"
#include "GPSEventSink.h"
#include "GPSProtocolRuntime.h"
#include "GPSReceiverFamilies.h"
#include "Quectel/QuectelFamily.h"
#include "Support/GPSTestClock.h"
#include "Support/QuectelReceiverModel.h"
#include "Support/UnicoreReceiverModel.h"
#include "UBX/UBXDecoder.h"
#include "UBX/UBXFamily.h"
#include "Unicore/UnicoreFamily.h"

using namespace std::chrono_literals;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    GPSTestClock clock(1000000);
    GPSRuntimeIO io;
    io.read = [](auto, auto) -> GPSReadResult { std::abort(); };
    io.write = [](auto, auto) -> GPSWriteResult { std::abort(); };
    io.setBaudrate = [](auto) -> GPSBaudStatus { std::abort(); };
    io.nowUs = [&clock] { return clock.nowUs(); };
    bool decoding = false;
    const auto operationalIO = [&decoding](GPSRuntimeIO services) {
        services.read = [&, read = services.read](auto bytes, auto deadline) {
            if (decoding) {
                std::abort();
            }
            return read(bytes, deadline);
        };
        services.write = [&, write = services.write](auto bytes, auto deadline) {
            if (decoding) {
                std::abort();
            }
            return write(bytes, deadline);
        };
        services.setBaudrate = [&, baud = services.setBaudrate](auto value) {
            if (decoding) {
                std::abort();
            }
            return baud(value);
        };
        services.wait = [&, wait = services.wait](auto duration) {
            if (decoding) {
                std::abort();
            }
            return wait(duration);
        };
        return services;
    };
    GPSRuntimeObserver operational;
    operational.decoded = [](const GPSEventBatch& batch) {
        if (batch.events.size() > GPSEventSink::MAX_EVENTS) {
            std::abort();
        }
    };
    GPSConfig fixed;
    fixed.base.mode = GPSBaseStationConfig::Fixed{};
    std::get<GPSBaseStationConfig::Fixed>(fixed.base.mode).position = {
        .latitudeDegrees = 0, .longitudeDegrees = 90, .altitudeMeters = 100};
    const bool fixedMode = size != 0 && (data[0] & 1);
    std::vector<std::unique_ptr<GPSProtocolRuntime>> ubx;
    for (const bool assembleEpochs : {false, true}) {
        auto runtime = std::make_unique<GPSProtocolRuntime>(UBX::FAMILY, io);
        UBX::decoder(runtime->protocol())
            .setMode({.navigation = true, .useNavPvt = true, .corrections = true, .assembleEpochs = assembleEpochs},
                     runtime->stream());
        ubx.push_back(std::move(runtime));
    }
    GPSTest::UnicoreReceiver unicorePeer(clock);
    unicorePeer.chunk = GPSCommandChannel::READ_CHUNK_SIZE;
    auto operationalUnicore =
        std::make_unique<GPSProtocolRuntime>(Unicore::FAMILY, operationalIO(unicorePeer.io()), operational);
    GPSConfig averaging;
    averaging.base.mode = GPSBaseStationConfig::ReceiverAveraging{};
    unsigned unicoreBaud = 115200;
    if (!operationalUnicore->configure(fixedMode ? fixed : averaging, unicoreBaud)) {
        std::abort();
    }
    GPSTest::QuectelReceiver quectelPeer(clock);
    quectelPeer.role = 2;
    quectelPeer.chunk = GPSCommandChannel::READ_CHUNK_SIZE;
    if (fixedMode) {
        quectelPeer.base = "2,0,0,0.0000,6378237.0000,0.0000,0";
    }
    auto operationalQuectel =
        std::make_unique<GPSProtocolRuntime>(Quectel::FAMILY, operationalIO(quectelPeer.io()), operational);
    GPSConfig survey;
    std::get<GPSBaseStationConfig::SurveyIn>(survey.base.mode).accuracyMeters = 15;
    std::get<GPSBaseStationConfig::SurveyIn>(survey.base.mode).duration = 60s;
    unsigned quectelBaud = 460800;
    if (!operationalQuectel->configure(fixedMode ? fixed : survey, quectelBaud)) {
        std::abort();
    }
    // Every family freshly created, then the operational runtimes.
    std::vector<std::unique_ptr<GPSProtocolRuntime>> runtimes;
    for (const auto* family : gpsReceiverFamilies()) {
        runtimes.push_back(std::make_unique<GPSProtocolRuntime>(*family, io));
    }
    for (auto& runtime : ubx) {
        runtimes.push_back(std::move(runtime));
    }
    runtimes.push_back(std::move(operationalUnicore));
    runtimes.push_back(std::move(operationalQuectel));
    decoding = true;
    const auto startedAt = clock.nowUs();
    for (const auto& runtime : runtimes) {
        clock.reset(startedAt);
        const size_t split = size ? data[0] % (size + 1) : 0;
        for (auto bytes :
             {std::span<const uint8_t>(data, split), std::span<const uint8_t>(data + split, size - split)}) {
            do {
                const auto result = runtime->decode(bytes);
                if (result.batch.events.size() > GPSEventSink::MAX_EVENTS) {
                    std::abort();
                }
                bytes = bytes.subspan(result.bytesConsumed);
            } while (!bytes.empty());
        }
        clock.advanceBy(5000001);
        if (runtime->decode({}).batch.events.size() > GPSEventSink::MAX_EVENTS) {
            std::abort();
        }
    }
    return 0;
}
