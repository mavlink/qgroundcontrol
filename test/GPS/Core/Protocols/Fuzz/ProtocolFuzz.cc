#include <chrono>
#include <cstdlib>
#include <memory>
#include <vector>

#include "GPSCommandChannel.h"
#include "GPSProtocolRuntime.h"
#include "GPSReceiverFamilies.h"
#include "Protocols/Support/GPSModelViolations.h"
#include "Protocols/Support/GPSTestClock.h"
#include "Protocols/Support/QuectelReceiverModel.h"
#include "Protocols/Support/ReceiverBench.h"
#include "Protocols/Support/UnicoreReceiverModel.h"

using namespace std::chrono_literals;
using namespace GPSTest;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    GPSTestClock clock(GPSTestClock::START_US);
    GPSRuntimeIO io;
    io.read = [](auto, auto) -> GPSReadResult { std::abort(); };
    io.write = [](auto, auto) -> GPSWriteResult { std::abort(); };
    io.setBaudrate = [](auto) -> GPSBaudStatus { std::abort(); };
    io.clock.nowUs = [&clock] { return clock.nowUs(); };
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
        services.clock.wait = [&, wait = services.clock.wait](auto duration) {
            if (decoding) {
                std::abort();
            }
            return wait(duration);
        };
        return services;
    };
    GPSConfig fixed;
    fixed.base.mode = GPSBaseStationConfig::Fixed{};
    std::get<GPSBaseStationConfig::Fixed>(fixed.base.mode).position = {
        .latitudeDegrees = 0, .longitudeDegrees = 90, .altitudeMeters = 100};
    const bool fixedMode = size != 0 && (data[0] & 1);
    auto ubx = std::make_unique<GPSProtocolRuntime>(UBX::FAMILY, io);
    ubx->armNavigationDecode({.corrections = true});
    ModelReceiver<UnicoreReceiverModel> unicorePeer(clock);
    unicorePeer.model.chunk = GPSCommandChannel::READ_CHUNK_SIZE;
    auto operationalUnicore = std::make_unique<GPSProtocolRuntime>(Unicore::FAMILY, operationalIO(unicorePeer.io()));
    GPSConfig averaging;
    averaging.base.mode = GPSBaseStationConfig::ReceiverAveraging{};
    unsigned unicoreBaud = 115200;
    if (!operationalUnicore->configure(fixedMode ? fixed : averaging, unicoreBaud)) {
        std::abort();
    }
    ModelReceiver<QuectelReceiverModel> quectelPeer(clock);
    quectelPeer.model.role = 2;
    quectelPeer.model.chunk = GPSCommandChannel::READ_CHUNK_SIZE;
    if (fixedMode) {
        quectelPeer.model.base = "2,0,0,0.0000,6378237.0000,0.0000,0";
    }
    auto operationalQuectel = std::make_unique<GPSProtocolRuntime>(Quectel::FAMILY, operationalIO(quectelPeer.io()));
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
    runtimes.push_back(std::move(ubx));
    runtimes.push_back(std::move(operationalUnicore));
    runtimes.push_back(std::move(operationalQuectel));
    decoding = true;
    const auto startedAt = clock.nowUs();
    for (const auto& runtime : runtimes) {
        clock.reset(startedAt);
        const size_t split = size ? data[0] % (size + 1) : 0;
        (void) runtime->decode({data, split});
        (void) runtime->decode({data + split, size - split});
        clock.advanceBy(5000001);
        (void) runtime->decode({});
    }
    if (!GPSTest::ModelViolations::take().empty()) {
        std::abort();
    }
    return 0;
}
