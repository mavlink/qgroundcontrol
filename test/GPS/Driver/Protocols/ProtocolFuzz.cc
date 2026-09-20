#include <cstdlib>

#include "Ashtech/GPSDriverAshtech.h"
#include "Femto/GPSDriverFemto.h"
#include "GPSProtocolFeatures.h"
#include "GPSProtocolTestIO.h"
#include "SBF/GPSDriverSBF.h"
#include "UBX/GPSDriverUBX.h"
#if QGC_GPS_ENABLE_UNICORE
#include "Support/UnicoreReceiverModel.h"
#include "Unicore/GPSDriverUnicore.h"
#endif
#if QGC_GPS_ENABLE_QUECTEL
#include "Quectel/GPSDriverQuectel.h"
#include "Support/QuectelReceiverModel.h"
#endif
#if QGC_GPS_ENABLE_PASSIVE
#include "Passive/GPSDriverPassive.h"
#endif

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    gps_test_warnings.clear();
    gps_test_time = 1000000;
    GPSNativePositionReport position{};
    GPSNativeSatelliteReport satellites{};
    GPSProtocolIO io;
    io.read = [](auto, auto) -> GPSReadResult { std::abort(); };
    io.write = [](auto, auto) -> GPSWriteResult { std::abort(); };
    io.setBaudrate = [](auto) -> GPSBaudStatus { std::abort(); };
    uint64_t& clock = gps_test_time;
    io.nowUs = [&clock] { return clock; };
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
    fixed.output_mode = GPSProtocol::OutputMode::RTCM;
    fixed.base.useFixedBase = true;
    fixed.base.fixedPosition = {.latitudeDegrees = 0, .longitudeDegrees = 90, .altitudeMeters = 100};
    const bool fixedMode = size != 0 && (data[0] & 1);
#if QGC_GPS_ENABLE_UBX
    GPSNativeUBX ubx(io, &position, &satellites);
    GPSNativeUBX operationalUbx(io, &position, &satellites);
    operationalUbx.setDecodeContext({true, true, true});
    GPSNativeUBX epochUbx(io, &position, &satellites);
    epochUbx.setDecodeContext({true, true, true, true});
#endif
#if QGC_GPS_ENABLE_ASHTECH
    GPSNativeAshtech ashtech(io, &position, &satellites);
#endif
#if QGC_GPS_ENABLE_SBF
    GPSNativeSBF sbf(io, &position, &satellites);
#endif
#if QGC_GPS_ENABLE_FEMTO
    GPSNativeFemto femto(io, &position, &satellites);
#endif
#if QGC_GPS_ENABLE_UNICORE
    GPSNativeUnicore unicore(io, &position, &satellites);
    GPSTest::UnicoreReceiver unicorePeer;
    unicorePeer.chunk = GPS_READ_BUFFER_SIZE;
    GPSNativeUnicore operationalUnicore(operationalIO(unicorePeer.io()), &position, &satellites);
    GPSProtocol::GPSConfig averaging;
    averaging.output_mode = GPSProtocol::OutputMode::RTCM;
    averaging.base.surveyMode = GPSBaseStationConfig::SurveyMode::ReceiverManaged;
    unsigned unicoreBaud = 115200;
    if (operationalUnicore.configure(unicoreBaud, fixedMode ? fixed : averaging)) {
        std::abort();
    }
#endif
#if QGC_GPS_ENABLE_QUECTEL
    GPSNativeQuectel quectel(io, &position, &satellites);
    GPSTest::QuectelReceiver quectelPeer;
    quectelPeer.role = 2;
    quectelPeer.chunk = GPS_READ_BUFFER_SIZE;
    if (fixedMode) {
        quectelPeer.base = "2,0,0,0.0000,6378237.0000,0.0000,0";
    }
    GPSNativeQuectel operationalQuectel(operationalIO(quectelPeer.io()), &position, &satellites);
    GPSProtocol::GPSConfig survey;
    survey.output_mode = GPSProtocol::OutputMode::RTCM;
    survey.base.surveyInAccMeters = 15;
    survey.base.surveyInDurationSecs = 60;
    unsigned quectelBaud = 460800;
    if (operationalQuectel.configure(quectelBaud, fixedMode ? fixed : survey)) {
        std::abort();
    }
#endif
#if QGC_GPS_ENABLE_PASSIVE
    GPSNativePassive passive(io, &position, &satellites);
#endif
    GPSProtocol* protocols[] = {
#if QGC_GPS_ENABLE_UBX
        &ubx,     &operationalUbx,     &epochUbx,
#endif
#if QGC_GPS_ENABLE_ASHTECH
        &ashtech,
#endif
#if QGC_GPS_ENABLE_SBF
        &sbf,
#endif
#if QGC_GPS_ENABLE_FEMTO
        &femto,
#endif
#if QGC_GPS_ENABLE_UNICORE
        &unicore, &operationalUnicore,
#endif
#if QGC_GPS_ENABLE_QUECTEL
        &quectel, &operationalQuectel,
#endif
#if QGC_GPS_ENABLE_PASSIVE
        &passive,
#endif
    };
    decoding = true;
    const auto startedAt = clock;
    for (GPSProtocol* protocol : protocols) {
        clock = startedAt;
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
        clock += 5000001;
        if (protocol->decode({}).batch.events.size() > GPSDecodedBatch::MAX_EVENTS) {
            std::abort();
        }
    }
    return 0;
}
