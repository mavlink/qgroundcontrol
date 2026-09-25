#include <cstdlib>

#include "Ashtech/AshtechProtocol.h"
#include "Femto/FemtoProtocol.h"
#include "GPSAsciiProtocol.h"
#include "GPSProtocolTestIO.h"
#include "Quectel/QuectelProtocol.h"
#include "SBF/SBFProtocol.h"
#include "Support/QuectelReceiverModel.h"
#include "Support/UnicoreReceiverModel.h"
#include "UBX/UBXProtocol.h"
#include "Unicore/UnicoreProtocol.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    GPSTestClock clock(1000000);
    GPSProtocolIO io;
    io.read = [](auto, auto) -> GPSReadResult { std::abort(); };
    io.write = [](auto, auto) -> GPSWriteResult { std::abort(); };
    io.setBaudrate = [](auto) -> GPSBaudStatus { std::abort(); };
    io.nowUs = [&clock] { return clock.nowUs(); };
    bool decoding = false;
    const auto operationalIO = [&decoding](GPSProtocolIO services) {
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
        services.commandFinished = {};
        services.decoded = [](const GPSDecodedBatch& batch) {
            if (batch.events.size() > GPSDecodedBatch::MAX_EVENTS) {
                std::abort();
            }
        };
        services.log = {};
        return services;
    };
    GPSProtocol::GPSConfig fixed;
    fixed.base.mode = GPSBaseStationConfig::Fixed{};
    std::get<GPSBaseStationConfig::Fixed>(fixed.base.mode).position = {
        .latitudeDegrees = 0, .longitudeDegrees = 90, .altitudeMeters = 100};
    const bool fixedMode = size != 0 && (data[0] & 1);
    UBXProtocol ubx(io);
    UBXProtocol operationalUbx(io);
    operationalUbx.setDecodeContext({true, true, true});
    UBXProtocol epochUbx(io);
    epochUbx.setDecodeContext({true, true, true, true});
    AshtechProtocol ashtech(io);
    SBFProtocol sbf(io);
    FemtoProtocol femto(io);
    UnicoreProtocol unicore(io);
    GPSTest::UnicoreReceiver unicorePeer(clock);
    unicorePeer.chunk = GPS_READ_BUFFER_SIZE;
    UnicoreProtocol operationalUnicore(operationalIO(unicorePeer.io()));
    GPSProtocol::GPSConfig averaging;
    averaging.base.mode = GPSBaseStationConfig::ReceiverAveraging{};
    unsigned unicoreBaud = 115200;
    if (!operationalUnicore.configure(unicoreBaud, fixedMode ? fixed : averaging)) {
        std::abort();
    }
    QuectelProtocol quectel(io);
    GPSTest::QuectelReceiver quectelPeer(clock);
    quectelPeer.role = 2;
    quectelPeer.chunk = GPS_READ_BUFFER_SIZE;
    if (fixedMode) {
        quectelPeer.base = "2,0,0,0.0000,6378237.0000,0.0000,0";
    }
    QuectelProtocol operationalQuectel(operationalIO(quectelPeer.io()));
    GPSProtocol::GPSConfig survey;
    std::get<GPSBaseStationConfig::SurveyIn>(survey.base.mode).accuracyMeters = 15;
    std::get<GPSBaseStationConfig::SurveyIn>(survey.base.mode).durationSecs = 60;
    unsigned quectelBaud = 460800;
    if (!operationalQuectel.configure(quectelBaud, fixedMode ? fixed : survey)) {
        std::abort();
    }
    PassiveProtocol passive(io);
    GPSProtocol* protocols[] = {
        &ubx,     &operationalUbx,     &epochUbx,
        &ashtech,
        &sbf,
        &femto,
        &unicore, &operationalUnicore,
        &quectel, &operationalQuectel,
        &passive,
    };
    decoding = true;
    const auto startedAt = clock.nowUs();
    for (GPSProtocol* protocol : protocols) {
        clock.reset(startedAt);
        const size_t split = size ? data[0] % (size + 1) : 0;
        for (auto bytes :
             {std::span<const uint8_t>(data, split), std::span<const uint8_t>(data + split, size - split)}) {
            do {
                auto result = protocol->decode(bytes);
                if (result.batch.events.size() > GPSDecodedBatch::MAX_EVENTS) {
                    std::abort();
                }
                bytes = bytes.subspan(result.bytesConsumed);
            } while (!bytes.empty());
        }
        clock.advanceBy(5000001);
        if (protocol->decode({}).batch.events.size() > GPSDecodedBatch::MAX_EVENTS) {
            std::abort();
        }
    }
    return 0;
}
