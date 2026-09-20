#include <algorithm>
#include <cmath>
#include <cstdio>
#include <deque>
#include <iterator>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "GPSProtocolTestIO.h"
#include "UBX/GPSDriverUBX.h"
#include "UBX/UBXMessageCodec.h"
#include "UnitTest.h"

// Keep checks active in Release, too.
#define CHECK(condition)                                                                                 \
    do {                                                                                                 \
        if (!(condition)) {                                                                              \
            throw std::runtime_error(std::string("line ") + std::to_string(__LINE__) + ": " #condition); \
        }                                                                                                \
    } while (0)

namespace {
using Bytes = std::vector<uint8_t>;

static uint32_t littleEndian(const Bytes& bytes, size_t offset, size_t width)
{
    CHECK(offset + width <= bytes.size());
    uint32_t value = 0;

    for (size_t i = 0; i < width; ++i) {
        value |= uint32_t(bytes[offset + i]) << (8 * i);
    }

    return value;
}

static Bytes packet(uint16_t message, const Bytes& payload)
{
    Bytes bytes{
        0xb5, 0x62, uint8_t(message), uint8_t(message >> 8), uint8_t(payload.size()), uint8_t(payload.size() >> 8)};
    bytes.insert(bytes.end(), payload.begin(), payload.end());
    uint8_t a = 0, b = 0;

    for (size_t i = 2; i < bytes.size(); ++i) {
        a += bytes[i];
        b += a;
    }

    bytes.push_back(a);
    bytes.push_back(b);
    return bytes;
}

enum class SurveyReply
{
    stopped,
    active,
    valid,
    silent,
    bad_checksum,
    bad_length
};

class Receiver
{
public:
    GPSNativeIntegrityReport integrity;
    unsigned integrityCount = 0;

    int readback_mode = 0;
    unsigned readback_requests = 0;
    std::vector<SurveyReply> replies{SurveyReply::stopped};
    bool reject_disable = false;
    bool reject_constellations = false;
    bool timeout_constellations = false;
    bool timeout_constellation_retry = false;
    unsigned constellation_requests = 0;
    unsigned transport_operations = 0;
    uint16_t legacy_measurement_interval = 0;
    uint8_t legacy_dynamic_model = 0;
    uint32_t legacy_fixed_accuracy = 0;
    unsigned legacy_constellation_requests = 0;
    bool legacy = false;
    std::string module = "ZED-F9P";
    std::string hardware;
    std::string protocol;
    unsigned receiverBaud = 0;
    unsigned hostBaud = 0;
    bool usb = false;
    bool loseBaudAck = false;
    bool ignoreBaudChange = false;
    bool corruptIdentity = false;
    bool silencePortConfiguration = false;
    bool rejectAfterBaudChange = false;
    bool nakAfterBaudChange = false;
    bool lateBaudAckDelivered = false;
    unsigned configurationWrites = 0;
    unsigned unidentifiedWrites = 0;
    std::vector<unsigned> identityBauds;
    std::vector<unsigned> hostBauds;
    bool reject_start = false;
    bool fail_poll_write = false;
    int poll_read_error = 0;
    QString poll_read_detail = QStringLiteral("Receiver link lost: Gerät");
    unsigned failed_reads = 0;
    size_t read_chunk = 7;  // Exercise packet fragmentation through the real parser.
    unsigned comms_polls = 0;
    bool fail_comms_write = false;
    unsigned polls = 0;
    unsigned starts = 0;
    unsigned status_callbacks = 0;
    unsigned rtcm_enables = 0;
    std::vector<uint32_t> modes;
    std::map<uint32_t, uint32_t> start_settings;
    std::map<uint32_t, uint32_t> current_settings;
    std::map<uint16_t, uint8_t> message_rates;
    uint64_t disabled_at = 0;
    uint64_t started_at = 0;

    void survey(SurveyReply reply)
    {
        if (reply == SurveyReply::silent) {
            return;
        }

        // NAV-SVIN has a 40-byte payload: validity at offset 36, active at 37.
        Bytes payload(40, 0);
        payload[36] = reply == SurveyReply::valid;
        payload[37] = reply == SurveyReply::active;

        if (reply == SurveyReply::bad_length) {
            payload.push_back(0);
        }

        Bytes bytes = packet(UBX_MSG_NAV_SVIN, payload);

        if (reply == SurveyReply::bad_checksum) {
            bytes.back() ^= 0xff;
        }

        queue(bytes);
    }

    void bufferWarning(bool valid_checksum = true)
    {
        const std::string warning = "txbuf alloc";
        Bytes bytes = packet(UBX_MSG_INF_ERROR, Bytes(warning.begin(), warning.end()));

        if (!valid_checksum) {
            bytes.back() ^= 0xff;
        }

        queue(bytes);
    }

    void queue(const Bytes& bytes) { incoming.insert(incoming.end(), bytes.begin(), bytes.end()); }

private:
    Bytes outgoing;
    std::deque<uint8_t> incoming;
    Bytes heldBaudAck;
    bool identityDelivered = false;

    void process(const Bytes& bytes)
    {
        const auto message = uint16_t(littleEndian(bytes, 2, 2));
        const Bytes payload(bytes.begin() + 6, bytes.end() - 2);
        CHECK(bytes == packet(message, payload));  // Validate outgoing framing/checksum.
        if (message == UBX_MSG_MON_VER) {
            identityBauds.push_back(hostBaud);
        }
        if ((message & 0xff) == UBX_CLASS_CFG && message != UBX_MSG_CFG_VALGET &&
            !(message == UBX_MSG_CFG_TMODE3 && payload.empty())) {
            ++configurationWrites;
            unidentifiedWrites += !identityDelivered;
        }
        if (receiverBaud && !usb && hostBaud != receiverBaud) {
            return;
        }

        if (message == UBX_MSG_MON_COMMS) {
            CHECK(payload.empty());
            ++comms_polls;
            return;
        }
        if (message == UBX_MSG_CFG_TMODE3 && payload.empty()) {
            ++readback_requests;
            if (readback_mode == 1) {
                return;
            }
            if (readback_mode == 2) {
                queue(packet(UBX_MSG_ACK_ACK, {uint8_t(message), uint8_t(message >> 8)}));
                return;
            }
            Bytes response(40, 0);
            response[0] = readback_mode == 3 ? 1 : 0;
            response[2] = modes.empty() ? 0 : modes.back();
            queue(packet(message, response));
            return;
        }
        if (message == UBX_MSG_CFG_VALGET) {
            ++readback_requests;
            CHECK(payload.size() >= 4 && payload.size() <= 40 && (payload.size() - 4) % 4 == 0);
            CHECK(payload[0] == 0 && payload[1] == 0 && payload[2] == 0 && payload[3] == 0);
            if (readback_mode == 1) {
                return;
            }
            if (readback_mode == 2) {
                queue(packet(UBX_MSG_ACK_ACK, {uint8_t(message), uint8_t(message >> 8)}));
                return;
            }
            Bytes response{1, uint8_t(readback_mode == 3 ? 1 : 0), 0, 0};
            for (size_t offset = 4; offset < payload.size(); offset += 4) {
                const uint32_t key = littleEndian(payload, offset, 4);
                const auto found = current_settings.find(key);
                const uint32_t value = found == current_settings.end() ? 0 : found->second;
                response.insert(response.end(), payload.begin() + offset, payload.begin() + offset + 4);
                const unsigned code = key >> 28;
                const unsigned width = code <= 2 ? 1 : 1u << (code - 2);
                for (unsigned byte = 0; byte < width; ++byte) {
                    response.push_back(uint8_t(value >> (8 * byte)));
                }
            }
            if (readback_mode == 4) {
                const Bytes duplicate(response.begin() + 4, response.begin() + 9);
                response.insert(response.end(), duplicate.begin(), duplicate.end());
            } else if (readback_mode == 5) {
                response.pop_back();
            }
            Bytes message_bytes = packet(message, response);
            if (readback_mode == 6) {
                message_bytes.back() ^= 1;
            }
            queue(message_bytes);
            return;
        }

        if (message == UBX_MSG_MON_VER) {
            CHECK(payload.empty());
            Bytes version(70, 0);
            memcpy(version.data(), "HPG 1.32", 8);
            const std::string hardwareVersion = hardware.empty() ? (legacy                 ? "00080000"
                                                                    : module == "ZED-X20P" ? "000B0000"
                                                                                           : "00190000")
                                                                 : hardware;
            CHECK(hardwareVersion.size() == 8);
            memcpy(version.data() + 30, hardwareVersion.data(), 8);
            const std::string identity = "MOD=" + module;
            CHECK(identity.size() < 30);
            memcpy(version.data() + 40, identity.c_str(), identity.size());
            if (!protocol.empty()) {
                const std::string extension = "PROTVER=" + protocol;
                CHECK(extension.size() < 30);
                version.resize(100);
                std::copy(extension.begin(), extension.end(), version.begin() + 70);
            }
            auto response = packet(message, version);
            if (corruptIdentity) {
                response.back() ^= 1;
            } else {
                identityDelivered = true;
            }
            queue(response);
            return;
        }

        if (message == UBX_MSG_NAV_SVIN) {
            CHECK(payload.empty());
            CHECK(!modes.empty() && modes.back() == 0);
            ++polls;
            survey(replies.at(std::min<size_t>(polls - 1, replies.size() - 1)));
            return;
        }

        if (legacy) {
            if (message == UBX_MSG_CFG_RATE) {
                legacy_measurement_interval = littleEndian(payload, 0, 2);
            }
            if (message == UBX_MSG_CFG_NAV5) {
                legacy_dynamic_model = payload.at(2);
            }
            if (message == UBX_MSG_CFG_GNSS) {
                ++legacy_constellation_requests;
            }
            if (message == UBX_MSG_CFG_PRT) {
                CHECK(payload.size() == 40);
                CHECK(payload[0] == UBX_TX_CFG_PRT_PORTID && payload[20] == UBX_TX_CFG_PRT_PORTID_USB);
                const auto rate = littleEndian(payload, 8, 4);
                CHECK(rate == littleEndian(payload, 28, 4));
                if (receiverBaud && rate != receiverBaud) {
                    receiverBaud = rate;
                    if (loseBaudAck) {
                        return;
                    }
                }
            }
            if (message == UBX_MSG_CFG_MSG) {
                message_rates[uint16_t(littleEndian(payload, 0, 2))] = payload.at(2);
            }

            bool reject = message == UBX_MSG_CFG_VALSET;

            if (message == UBX_MSG_CFG_MSG && littleEndian(payload, 0, 2) == UBX_MSG_RTCM3_1005 && payload.at(2) > 0) {
                ++rtcm_enables;
            }

            if (message == UBX_MSG_CFG_TMODE3) {
                const uint32_t mode = littleEndian(payload, 2, 2);
                modes.push_back(mode);
                legacy_fixed_accuracy = littleEndian(payload, 20, 4);
                disabled_at = gps_test_time;
                reject = reject_disable && mode == 0;
            }

            queue(packet(reject ? UBX_MSG_ACK_NAK : UBX_MSG_ACK_ACK, {uint8_t(message), uint8_t(message >> 8)}));
            return;
        }

        CHECK(message == UBX_MSG_CFG_VALSET);
        CHECK(payload.size() >= 4);
        if (silencePortConfiguration) {
            return;
        }
        std::map<uint32_t, uint32_t> settings;

        for (size_t i = 4; i < payload.size();) {
            const uint32_t key = littleEndian(payload, i, 4);
            // Host connections must leave the receiver's I2C protocol configuration alone.
            CHECK((key & 0xffff0000u) != 0x10710000u);
            CHECK((key & 0xffff0000u) != 0x10720000u);
            i += 4;
            const unsigned size_code = (key >> 28) & 7;
            CHECK(size_code >= 1 && size_code <= 4);
            const size_t width = size_code <= 2 ? 1 : size_code == 3 ? 2 : 4;
            settings[key] = littleEndian(payload, i, width);
            i += width;
        }
        if (!heldBaudAck.empty()) {
            queue(heldBaudAck);
            heldBaudAck.clear();
            lateBaudAckDelivered = true;
            if (rejectAfterBaudChange) {
                return;
            }
            if (nakAfterBaudChange) {
                queue(packet(UBX_MSG_ACK_NAK, {uint8_t(message), uint8_t(message >> 8)}));
            }
        }

        if (settings.count(UBX_CFG_KEY_SIGNAL_GPS_ENA)) {
            ++constellation_requests;
            if (timeout_constellations || (timeout_constellation_retry && constellation_requests == 2)) {
                return;
            }
        }
        bool reject = reject_constellations && settings.count(UBX_CFG_KEY_SIGNAL_GPS_ENA);
        const auto mode = settings.find(UBX_CFG_KEY_TMODE_MODE);

        if (mode != settings.end()) {
            modes.push_back(mode->second);

            if (mode->second == 0) {
                disabled_at = gps_test_time;
                reject = reject_disable;
            } else if (mode->second == 1) {
                ++starts;
                started_at = gps_test_time;
                start_settings = settings;
                reject = reject_start;
            }
        }

        if (settings[UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1005_I2C + 1] == 1) {
            ++rtcm_enables;
        }

        // M9 SPG has RTCM input, but no RTCM output protocol keys. Unknown keys
        // reject the entire VALSET, even when the requested value is zero.
        if (module == "NEO-M9N" && (settings.count(UBX_CFG_KEY_CFG_UART1OUTPROT_RTCM3X) ||
                                    settings.count(UBX_CFG_KEY_CFG_USBOUTPROT_RTCM3X))) {
            reject = true;
        }

        if (!reject) {
            for (const auto& setting : settings) {
                current_settings[setting.first] = setting.second;
            }
        }
        if (const auto rate = settings.find(UBX_CFG_KEY_CFG_UART1_BAUDRATE);
            rate != settings.end() && receiverBaud && rate->second != receiverBaud) {
            if (!ignoreBaudChange) {
                receiverBaud = rate->second;
            } else {
                current_settings[rate->first] = receiverBaud;
            }
            if (loseBaudAck) {
                heldBaudAck = packet(UBX_MSG_ACK_ACK, {uint8_t(message), uint8_t(message >> 8)});
                return;
            }
        }

        queue(packet(reject ? UBX_MSG_ACK_NAK : UBX_MSG_ACK_ACK, {uint8_t(message), uint8_t(message >> 8)}));
    }

public:
    GPSProtocolIO io()
    {
        auto result = makeGPSProtocolTestIO();
        result.read = [this](std::span<uint8_t> bytes, GPSDeadline deadline) -> GPSReadResult {
            ++transport_operations;
            const int timeout = deadline.remainingMilliseconds(gps_test_time);
            auto* data = bytes.data();
            const int size = static_cast<int>(bytes.size());
            CHECK(timeout >= 0);

            if (polls > 0 && poll_read_error < 0) {
                ++failed_reads;
                gps_test_time += 1000;
                return {poll_read_error == GPSProtocol::ReadCancelled ? GPSReadStatus::Cancelled : GPSReadStatus::Error,
                        0, poll_read_detail};
            }

            if (incoming.empty()) {
                // The driver's deadlines use strict comparisons; move past the timeout.
                gps_test_time += uint64_t(timeout) * 1000 + 1;
                return {GPSReadStatus::TimedOut};
            }

            gps_test_time += 1000;
            const size_t count = std::min({incoming.size(), size_t(size), read_chunk});

            for (size_t i = 0; i < count; ++i) {
                static_cast<uint8_t*>(data)[i] = incoming.front();
                incoming.pop_front();
            }

            return {GPSReadStatus::Data, static_cast<int>(count)};
        };
        result.write = [this](std::span<const uint8_t> input, GPSDeadline) -> GPSWriteResult {
            ++transport_operations;
            const auto* data = input.data();
            const int size = static_cast<int>(input.size());

            const auto* bytes = static_cast<const uint8_t*>(data);

            if (fail_comms_write && outgoing.empty() && size >= 4 && bytes[2] == 0x0a && bytes[3] == 0x36) {
                ++comms_polls;
                return {GPSWriteStatus::Unsupported};
            }

            if (fail_poll_write && outgoing.empty() && size >= 4 && bytes[2] == 0x01 && bytes[3] == 0x3b) {
                return {GPSWriteStatus::Unsupported};
            }

            outgoing.insert(outgoing.end(), bytes, bytes + size);

            if (outgoing.size() >= 6 && outgoing.size() == littleEndian(outgoing, 4, 2) + 8) {
                const Bytes complete = std::move(outgoing);
                outgoing.clear();
                process(complete);
            }

            return {GPSWriteStatus::Completed, size, size};
        };
        result.setBaudrate = [this](unsigned rate) {
            ++transport_operations;
            hostBaud = rate;
            hostBauds.push_back(rate);
            return GPSBaudStatus::Configured;
        };
        result.wait = [this](std::chrono::microseconds delay) {
            ++transport_operations;
            gps_test_time += delay.count();
            return true;
        };
        result.decoded = [this](const GPSDecodedBatch& batch) {
            for (const auto& event : batch.events) {
                if (const auto* report = std::get_if<GPSNativeIntegrityReport>(&event)) {
                    integrity = *report;
                    ++integrityCount;
                }
            }
            for (const auto& event : batch.events) {
                if (std::holds_alternative<GPSNativeSurveyReport>(event)) {
                    ++status_callbacks;
                }
            }
        };
        return result;
    }
};

struct Fixture
{
    Receiver receiver;
    GPSNativePositionReport position{};
    GPSNativeUBX driver;
    GPSBaseStationConfig base;

    Fixture()
        : driver(receiver.io(), &position, nullptr)
    {
        gps_test_time = 0;
        gps_test_warnings.clear();
        base.surveyInAccMeters = 1.25;
        base.surveyInDurationSecs = 60;
    }

    int configure(GPSProtocol::OutputMode output = GPSProtocol::OutputMode::RTCM)
    {
        unsigned baudrate = 115200;
        GPSProtocol::GPSConfig config{};
        config.output_mode = output;
        config.base = base;
        return driver.configure(baudrate, config);
    }

    void success(unsigned expected_polls)
    {
        CHECK(configure() == 0);
        CHECK(driver.receiverReady());
        CHECK(receiver.modes == std::vector<uint32_t>({0, 1}));
        CHECK(receiver.polls == expected_polls);
        CHECK(receiver.starts == 1);
        CHECK(receiver.start_settings.at(UBX_CFG_KEY_TMODE_SVIN_MIN_DUR) == 60);
        CHECK(receiver.start_settings.at(UBX_CFG_KEY_TMODE_SVIN_ACC_LIMIT) == 12500);
        CHECK(receiver.start_settings.at(UBX_CFG_KEY_MSGOUT_UBX_NAV_SVIN_I2C + 1) == 5);
        CHECK(receiver.start_settings.at(UBX_CFG_KEY_MSGOUT_UBX_NAV_SVIN_I2C + 3) == 5);
        CHECK(receiver.status_callbacks == 0);
        CHECK(receiver.rtcm_enables == 0);
    }

    void timeout()
    {
        CHECK(configure() < 0);
        CHECK(!driver.receiverReady());
        CHECK(receiver.modes == std::vector<uint32_t>({0}));
        CHECK(receiver.starts == 0);
        CHECK(receiver.polls > 1 && receiver.polls <= 31);
        CHECK(gps_test_time - receiver.disabled_at >= 3000000);
        CHECK(gps_test_time - receiver.disabled_at < 3300000);
        CHECK(receiver.status_callbacks == 0);
        CHECK(receiver.rtcm_enables == 0);
    }

    void readFailure(int error)
    {
        receiver.poll_read_error = error;
        CHECK(configure() < 0);
        CHECK(!driver.receiverReady());
        CHECK(receiver.failed_reads == 1);
        CHECK(receiver.polls == 1);
        CHECK(receiver.modes == std::vector<uint32_t>({0}));
        CHECK(receiver.starts == 0);
        CHECK(receiver.status_callbacks == 0);
        CHECK(receiver.rtcm_enables == 0);
        CHECK(gps_test_time - receiver.disabled_at < 100000);

        const auto warnings = gps_test_warnings;
        if (error == GPSProtocol::ReadCancelled) {
            CHECK(warnings.empty());
        } else {
            CHECK(warnings == QStringList{QStringLiteral("Receiver read failed (status %1, code %2): %3")
                                              .arg(static_cast<int>(GPSReadStatus::Error))
                                              .arg(-EIO)
                                              .arg(receiver.poll_read_detail)});
        }
        CHECK(driver.ioErrorDetail() == receiver.poll_read_detail);
        CHECK(driver.receive(10) < 0);
        CHECK(receiver.failed_reads == 1);
        CHECK(gps_test_warnings == warnings);
    }
};

static void receiveFailureLogging()
{
    for (const int error : {GPSProtocol::ReadCancelled, -EIO}) {
        Fixture f;
        CHECK(f.configure() == 0);
        f.receiver.poll_read_error = error;
        gps_test_warnings.clear();
        CHECK(f.driver.receive(10) == error);
        CHECK(f.driver.ioErrorDetail() == f.receiver.poll_read_detail);
        const auto warnings = gps_test_warnings;
        if (error == GPSProtocol::ReadCancelled) {
            CHECK(warnings.empty());
        } else {
            CHECK(warnings == QStringList{QStringLiteral("Receiver read failed (status %1, code %2): %3")
                                              .arg(static_cast<int>(GPSReadStatus::Error))
                                              .arg(-EIO)
                                              .arg(f.receiver.poll_read_detail)});
        }
        CHECK(f.driver.receive(10) == error);
        CHECK(f.receiver.failed_reads == 1);
        CHECK(gps_test_warnings == warnings);
    }
}

static void positionMode(bool legacy, bool base_capable)
{
    Fixture f;
    f.receiver.legacy = legacy;
    f.receiver.module = legacy ? (base_capable ? "NEO-M8P" : "NEO-M8N") : (base_capable ? "ZED-F9P" : "NEO-M9N");

    f.receiver.replies = {SurveyReply::active, SurveyReply::valid, SurveyReply::stopped};
    if (base_capable) {
        // Model settings left behind by a previous base session.
        f.receiver.current_settings[UBX_CFG_KEY_CFG_USBOUTPROT_RTCM3X] = 1;
        f.receiver.current_settings[UBX_CFG_KEY_CFG_UART1OUTPROT_RTCM3X] = 1;
        f.receiver.current_settings[UBX_CFG_KEY_MSGOUT_UBX_NAV_SVIN_I2C + 1] = 5;
        f.receiver.current_settings[UBX_CFG_KEY_MSGOUT_UBX_NAV_SVIN_I2C + 3] = 5;
        f.receiver.message_rates[UBX_MSG_NAV_SVIN] = 5;
    }
    // A configured fixed base must not be re-applied when selecting GPS output.
    f.base = {.useFixedBase = true,
              .fixedPosition = {.latitudeDegrees = 47.0, .longitudeDegrees = 8.0, .altitudeMeters = 500.0f},
              .fixedBaseAccuracyMeters = 1.0f};
    CHECK(f.configure(GPSProtocol::OutputMode::GPS) == 0);
    CHECK(f.driver.receiverReady());
    CHECK(f.receiver.modes == (base_capable ? std::vector<uint32_t>{0} : std::vector<uint32_t>{}));
    CHECK(f.receiver.polls == 0);
    CHECK(f.receiver.readback_requests == (base_capable ? 1u : 0u));
    CHECK(f.receiver.starts == 0);
    CHECK(f.receiver.rtcm_enables == 0);
    CHECK(f.receiver.status_callbacks == 0);
    CHECK(gps_test_warnings.empty());
    if (base_capable) {
        if (legacy) {
            CHECK(f.receiver.message_rates.at(UBX_MSG_NAV_SVIN) == 0);
        } else {
            CHECK(f.receiver.current_settings.at(UBX_CFG_KEY_MSGOUT_UBX_NAV_SVIN_I2C + 1) == 0);
            CHECK(f.receiver.current_settings.at(UBX_CFG_KEY_MSGOUT_UBX_NAV_SVIN_I2C + 3) == 0);
        }
    }
    if (!legacy && base_capable) {
        CHECK(f.receiver.current_settings.at(UBX_CFG_KEY_CFG_USBOUTPROT_RTCM3X) == 0);
        CHECK(f.receiver.current_settings.at(UBX_CFG_KEY_CFG_UART1OUTPROT_RTCM3X) == 0);

    } else if (!legacy) {
        CHECK(f.receiver.current_settings.count(UBX_CFG_KEY_CFG_USBOUTPROT_RTCM3X) == 0);
        CHECK(f.receiver.current_settings.count(UBX_CFG_KEY_CFG_UART1OUTPROT_RTCM3X) == 0);
    }

    Bytes pvt(UBX::WIRE_SIZE<ubx_payload_rx_nav_pvt_t>, 0);
    pvt[20] = 3;
    pvt[21] = 1;
    pvt[23] = 12;
    const auto store = [&](size_t offset, uint32_t value) {
        for (size_t i = 0; i < 4; ++i) {
            pvt[offset + i] = uint8_t(value >> (8 * i));
        }
    };
    store(24, 100000000);
    store(28, 200000000);
    store(40, 800);
    f.receiver.queue(packet(UBX_MSG_NAV_PVT, pvt));
    CHECK(f.driver.receive(500) & 1);
    CHECK(f.position.fix_type == GPSPositionReport::FixType::Fix3D);
    CHECK(f.position.latitude_deg == 20.0);
    CHECK(f.position.longitude_deg == 10.0);
    CHECK(f.position.satellites_used == 12);
}

static void positionModeFailure()
{
    for (bool legacy : {false, true}) {
        for (int failure = 0; failure < (legacy ? 4 : 7); ++failure) {
            Fixture f;
            f.receiver.legacy = legacy;
            f.receiver.module = legacy ? "NEO-M8P" : "ZED-F9P";
            f.receiver.reject_disable = failure == 0;
            f.receiver.readback_mode = failure;
            CHECK(f.configure(GPSProtocol::OutputMode::GPS) < 0);
            CHECK(!f.driver.receiverReady());
            CHECK(f.receiver.modes == std::vector<uint32_t>{0});
            CHECK(f.receiver.starts == 0 && f.receiver.rtcm_enables == 0);
            CHECK(f.receiver.status_callbacks == 0);
            CHECK(gps_test_warnings.empty());
        }
    }
}

static Bytes commsPayload()
{
    Bytes payload(88, 0);
    payload[1] = 2;
    payload[2] = 2;
    // USB: 11800 bytes pending, current usage 100%, historical peak 101%.
    payload[9] = 3;
    payload[10] = 0x18;
    payload[11] = 0x2e;
    payload[16] = 100;
    payload[17] = 101;
    payload[18] = 12;
    payload[24] = 3;
    payload[26] = 4;
    payload[44] = 0x40;
    payload[45] = 0xe2;
    payload[46] = 1;
    // UART2: no current congestion, despite a historical peak of 108%.
    payload[48] = 1;
    payload[49] = 2;
    payload[57] = 108;
    return payload;
}

static void integrityReceipts()
{
    Fixture f;
    CHECK(f.configure(GPSProtocol::OutputMode::GPS) == 0);
    // This test inspects individual decoder mutations; epoch assembly has separate coverage.
    f.driver.setDecodeContext({.navigation = true});
    Bytes mon_rf(UBX::WIRE_SIZE<ubx_payload_rx_mon_rf_t>, 0);
    mon_rf[1] = 1;
    mon_rf[5] = 3;
    f.receiver.queue(packet(UBX_MSG_MON_RF, mon_rf));
    f.driver.receive(100);
    CHECK(f.receiver.integrityCount == 1);
    CHECK(f.position.timestamp == 0);
    CHECK(f.receiver.integrity.jamming_state == GPSIntegrityReport::JammingState::Critical);
    const auto rf_stamp = f.receiver.integrity.jamming_state_timestamp;
    CHECK(rf_stamp != 0);

    Bytes nav_status(UBX::WIRE_SIZE<ubx_payload_rx_nav_status_t>, 0);
    nav_status[7] = 1 << UBX_RX_NAV_STATUS_SPOOFDETSTATE_SHIFT;
    f.receiver.queue(packet(UBX_MSG_NAV_STATUS, nav_status));
    f.driver.receive(100);
    const auto spoof_stamp = f.receiver.integrity.spoofing_state_timestamp;
    CHECK(spoof_stamp != 0);
    CHECK(f.receiver.integrity.jamming_state_timestamp == rf_stamp);

    Bytes pvt(UBX::WIRE_SIZE<ubx_payload_rx_nav_pvt_t>, 0);
    pvt[20] = 3;
    pvt[21] = 1;
    for (int i = 0; i < 10; ++i) {
        gps_test_time += 1000000;
        f.receiver.queue(packet(UBX_MSG_NAV_PVT, pvt));
        CHECK(f.driver.receive(100) & 1);
        CHECK(f.position.timestamp > rf_stamp);
        CHECK(f.receiver.integrity.jamming_state_timestamp == rf_stamp);
        CHECK(f.receiver.integrity.spoofing_state_timestamp == spoof_stamp);
    }
    Bytes corrupt = packet(UBX_MSG_MON_RF, mon_rf);
    corrupt.back() ^= 0xff;
    f.receiver.queue(corrupt);
    f.driver.receive(100);
    CHECK(f.receiver.integrity.jamming_state_timestamp == rf_stamp);
    f.receiver.queue(packet(UBX_MSG_MON_RF, mon_rf));
    f.driver.receive(100);
    CHECK(f.receiver.integrity.jamming_state == GPSIntegrityReport::JammingState::Critical);
    CHECK(f.receiver.integrity.jamming_state_timestamp > rf_stamp);

    Bytes sec_sig(4, 0);
    sec_sig[0] = 2;
    sec_sig[1] = 1 | (3 << 1);
    f.receiver.queue(packet(UBX_MSG_SEC_SIG, sec_sig));
    f.driver.receive(100);
    CHECK(f.receiver.integrity.jamming_state == GPSIntegrityReport::JammingState::Critical);
    const auto sec_stamp = f.receiver.integrity.jamming_state_timestamp;
    gps_test_time += 6000000;
    f.receiver.queue(packet(UBX_MSG_NAV_PVT, pvt));
    CHECK(f.driver.receive(100) & 1);
    CHECK(f.receiver.integrity.jamming_state_timestamp == sec_stamp);
    f.receiver.queue(packet(UBX_MSG_SEC_SIG, sec_sig));
    f.driver.receive(100);
    CHECK(f.receiver.integrity.jamming_state == GPSIntegrityReport::JammingState::Critical);
    CHECK(f.receiver.integrity.jamming_state_timestamp > sec_stamp);

    Bytes rtcm(UBX::WIRE_SIZE<ubx_payload_rx_rxm_rtcm_t>, 0);
    rtcm[1] = 2 << UBX_RX_RXM_RTCM_MSGUSED_SHIFT;
    f.receiver.queue(packet(UBX_MSG_RXM_RTCM, rtcm));
    f.driver.receive(100);
    CHECK(f.receiver.integrity.corrections_msg_used == GPSIntegrityReport::CorrectionUse::Used);
    const auto correction_stamp = f.receiver.integrity.corrections_timestamp;
    CHECK(correction_stamp != 0);
    gps_test_time += 6000000;
    f.receiver.queue(packet(UBX_MSG_NAV_PVT, pvt));
    CHECK(f.driver.receive(100) & 1);
    CHECK(f.receiver.integrity.corrections_timestamp == correction_stamp);
    Bytes cor(UBX::WIRE_SIZE<ubx_payload_rx_rxm_cor_t>, 0);
    cor[0] = 1;
    cor[4] = 29;
    cor[5] = 1;  // msgUsed=2 in statusInfo bits 8..7.
    f.receiver.queue(packet(UBX_MSG_RXM_COR, cor));
    f.driver.receive(100);
    CHECK(f.receiver.integrity.corrections_protocol == GPSNativeIntegrityReport::CORRECTIONS_PROTOCOL_PMP);
    CHECK(f.receiver.integrity.corrections_msg_used == GPSIntegrityReport::CorrectionUse::Used);
    CHECK(f.receiver.integrity.corrections_timestamp > correction_stamp);
}

static void commsDiagnostics()
{
    Fixture f;
    const Bytes reply = packet(UBX_MSG_MON_COMMS, commsPayload());
    f.receiver.queue(reply);
    f.driver.receive(100);
    CHECK(gps_test_warnings.empty());
    CHECK(f.receiver.comms_polls == 0);
    f.receiver.bufferWarning();
    f.driver.receive(100);
    CHECK(f.receiver.comms_polls == 1);
    CHECK(gps_test_warnings == QStringList{"ubx msg: txbuf alloc"});
    gps_test_warnings.clear();
    f.receiver.queue(reply);
    CHECK(f.driver.receive(100) < 0);  // Diagnostic traffic alone is not a position update.
    const QStringList expected{"MON-COMMS after txbuf: txErrors=0x02 ports=2 (snapshot after warning)",
                               "MON-COMMS USB port=0x0300 txPending=11800 txUsage=100% txPeakUsage=101% "
                               "rxPending=12 rxUsage=3% overrunErrs=4 skipped=123456",
                               "MON-COMMS UART2 port=0x0201 txPending=0 txUsage=0% txPeakUsage=108% "
                               "rxPending=0 rxUsage=0% overrunErrs=0 skipped=0"};
    CHECK(gps_test_warnings == expected);
    gps_test_warnings.clear();
    f.receiver.queue(reply);
    f.driver.receive(100);
    CHECK(gps_test_warnings.empty());
}

static void invalidCommsDiagnostics()
{
    Bytes payload = commsPayload();
    Bytes corrupt = packet(UBX_MSG_MON_COMMS, payload);
    corrupt.back() ^= 0xff;
    std::vector<Bytes> invalid{corrupt, packet(UBX_MSG_MON_COMMS, Bytes(7, 0)), packet(UBX_MSG_MON_COMMS, Bytes(87, 0)),
                               packet(UBX_MSG_MON_COMMS, Bytes(368, 0))};
    payload[0] = 1;
    invalid.push_back(packet(UBX_MSG_MON_COMMS, payload));
    payload[0] = 0;
    payload[1] = 3;
    invalid.push_back(packet(UBX_MSG_MON_COMMS, payload));
    payload[1] = 255;
    invalid.push_back(packet(UBX_MSG_MON_COMMS, payload));

    for (const auto& reply : invalid) {
        Fixture f;
        f.receiver.bufferWarning();
        f.driver.receive(100);
        gps_test_warnings.clear();
        f.receiver.queue(reply);
        f.driver.receive(100);
        CHECK(gps_test_warnings.empty());
        // Malformed input must not consume the pending reply or lose framing.
        f.receiver.queue(packet(UBX_MSG_MON_COMMS, Bytes(8, 0)));
        f.driver.receive(100);
        CHECK(gps_test_warnings ==
              QStringList{"MON-COMMS after txbuf: txErrors=0x00 ports=0 (snapshot after warning)"});
    }
}

static void expiredCommsDiagnostics()
{
    Fixture f;
    f.receiver.bufferWarning();
    f.driver.receive(100);
    CHECK(f.receiver.comms_polls == 1);
    gps_test_warnings.clear();
    gps_test_time += 2000000;
    f.receiver.queue(packet(UBX_MSG_MON_COMMS, commsPayload()));
    f.driver.receive(100);
    CHECK(gps_test_warnings.empty());
    // Expiration must allow a later warning to obtain a fresh snapshot.
    gps_test_time += 5000000;
    f.receiver.bufferWarning();
    f.driver.receive(100);
    CHECK(f.receiver.comms_polls == 2);
    gps_test_warnings.clear();
    f.receiver.queue(packet(UBX_MSG_MON_COMMS, Bytes(8, 0)));
    f.driver.receive(100);
    CHECK(gps_test_warnings.size() == 1);
}

static void receiverSettings()
{
    for (int scenario = 0; scenario < 6; ++scenario) {
        gps_test_time = 0;
        gps_test_warnings.clear();
        Receiver receiver;
        receiver.legacy = scenario == 2;
        receiver.module = receiver.legacy ? "NEO-M8P" : scenario == 1 ? "NEO-M9N" : "ZED-F9P";
        receiver.reject_constellations = scenario == 3 || scenario == 5;
        receiver.timeout_constellations = scenario == 4;
        receiver.timeout_constellation_retry = scenario == 5;
        GPSNativePositionReport position{};
        std::map<GPSReceiverSetting, std::vector<GPSCommandOutcome>> outcomes;
        auto io = receiver.io();
        io.commandFinished = [&](const GPSCommandResult& command) {
            for (auto setting : {GPSReceiverSetting::DynamicModel, GPSReceiverSetting::OutputRateHz,
                                 GPSReceiverSetting::ConstellationMask}) {
                if (command.affectedSettings.contains(setting)) {
                    outcomes[setting].push_back(command.evidence.outcome);
                }
            }
        };
        GPSNativeUBX driver(io, &position, nullptr);
        GPSProtocol::GPSConfig config{};
        config.dynamicModel = 4;
        config.output_mode = GPSProtocol::OutputMode::GPS;
        config.gnss_systems = static_cast<GPSProtocol::GNSSSystemsMask>(5);
        unsigned baudrate = 115200;
        CHECK((driver.configure(baudrate, config) == 0) == (scenario < 4));
        if (scenario >= 3) {
            CHECK(outcomes.at(GPSReceiverSetting::DynamicModel).back() == GPSCommandOutcome::Acknowledged);
            CHECK(outcomes.at(GPSReceiverSetting::OutputRateHz).back() == GPSCommandOutcome::Acknowledged);
            const auto& constellationOutcomes = outcomes.at(GPSReceiverSetting::ConstellationMask);
            const auto expected = scenario == 3 ? GPSCommandOutcome::Rejected : GPSCommandOutcome::TimedOut;
            CHECK(std::find(constellationOutcomes.begin(), constellationOutcomes.end(), expected) !=
                  constellationOutcomes.end());
        }
        if (scenario < 2) {
            CHECK(receiver.current_settings.at(UBX_CFG_KEY_NAVSPG_DYNMODEL) == 4);
            CHECK(receiver.current_settings.at(UBX_CFG_KEY_RATE_MEAS) == (scenario == 1 ? 125 : 200));
            CHECK(receiver.current_settings.at(UBX_CFG_KEY_SIGNAL_GPS_ENA) == 1);
            CHECK(receiver.current_settings.at(UBX_CFG_KEY_SIGNAL_GAL_ENA) == 1);
            CHECK(receiver.current_settings.at(UBX_CFG_KEY_SIGNAL_BDS_ENA) == 0);
            CHECK(gps_test_warnings.empty());
        } else if (scenario >= 4) {
            CHECK(!driver.receiverReady());
            CHECK(receiver.constellation_requests == (scenario == 4 ? 1 : 2));
            CHECK(receiver.current_settings.count(UBX_CFG_KEY_SIGNAL_SBAS_ENA) == 0);
        } else {
            CHECK(driver.receiverReady());
            if (receiver.legacy) {
                CHECK(receiver.legacy_measurement_interval == 200);
                CHECK(receiver.legacy_dynamic_model == 4);
                CHECK(receiver.legacy_constellation_requests == 1);
            } else {
                CHECK(receiver.current_settings.count(UBX_CFG_KEY_SIGNAL_GPS_ENA) == 0);
                CHECK(receiver.current_settings.count(UBX_CFG_KEY_SIGNAL_GAL_ENA) == 0);
                CHECK(receiver.current_settings.at(UBX_CFG_KEY_SIGNAL_SBAS_ENA) == 0);
            }
        }
    }
}

static void invalidConfiguration()
{
    using Config = GPSProtocol::GPSConfig;
    const Config fixed{.base = {.useFixedBase = true,
                                .fixedPosition = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500},
                                .fixedBaseAccuracyMeters = 1},
                       .output_mode = GPSProtocol::OutputMode::RTCM};
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto infinity = std::numeric_limits<float>::infinity();
    std::vector<Config> invalid;
    const auto addFixed = [&](auto mutate) {
        auto config = fixed;
        mutate(config.base);
        invalid.push_back(config);
    };
    addFixed([](auto& base) { base = {.useFixedBase = true}; });
    for (double value : {nan, double(infinity), 91.0, -91.0}) {
        addFixed([&](auto& base) { base.fixedPosition.latitudeDegrees = value; });
    }
    for (double value : {nan, double(infinity), 181.0, -181.0}) {
        addFixed([&](auto& base) { base.fixedPosition.longitudeDegrees = value; });
    }
    for (float value : {float(nan), infinity, 21474838.0f, -21474838.0f}) {
        addFixed([&](auto& base) { base.fixedPosition.altitudeMeters = value; });
    }
    for (float value : {float(nan), infinity, -1.0f, std::nextafter(429496.71875f, infinity)}) {
        addFixed([&](auto& base) { base.fixedBaseAccuracyMeters = value; });
    }
    for (double accuracy : {nan, double(infinity), 0.0, -1.0, 429496.7296}) {
        invalid.push_back({.base = {.surveyInAccMeters = accuracy, .surveyInDurationSecs = 60},
                           .output_mode = GPSProtocol::OutputMode::RTCM});
    }
    for (int64_t duration : {int64_t(0), int64_t(-1), int64_t(UINT32_MAX) + 1}) {
        invalid.push_back({.base = {.surveyInAccMeters = 1, .surveyInDurationSecs = duration},
                           .output_mode = GPSProtocol::OutputMode::RTCM});
    }
    invalid.push_back({.output_mode = static_cast<GPSProtocol::OutputMode>(99)});
    for (const auto& config : invalid) {
        for (bool wasReady : {false, true}) {
            Fixture f;
            if (wasReady) {
                CHECK(f.configure() == 0);
                CHECK(f.driver.receiverReady());
            }
            f.receiver.transport_operations = 0;
            gps_test_warnings.clear();
            unsigned baudrate = 115200;
            CHECK(f.driver.configure(baudrate, config) < 0);
            CHECK(f.receiver.transport_operations == 0);
            CHECK(!f.driver.receiverReady());
            CHECK(f.driver.ioError() == 0);
            CHECK(baudrate == 115200);
            CHECK(gps_test_warnings.size() == 1);
        }
    }

    for (bool legacy : {false, true}) {
        Fixture f;
        f.receiver.legacy = legacy;
        f.receiver.module = legacy ? "NEO-M8P" : "ZED-F9P";
        f.base = fixed.base;
        f.base.fixedBaseAccuracyMeters = 429496.71875f;
        CHECK(f.configure() == 0);
        CHECK(f.driver.receiverReady());
        CHECK((legacy ? f.receiver.legacy_fixed_accuracy
                      : f.receiver.current_settings.at(UBX_CFG_KEY_TMODE_FIXED_POS_ACC)) == 4294967040u);
    }
    Fixture position;
    position.base = {.useFixedBase = true};
    CHECK(position.configure(GPSProtocol::OutputMode::GPS) == 0);
}

static void explicitNoFix()
{
    Receiver receiver;
    GPSNativePositionReport position;
    GPSNativeUBX driver(receiver.io(), &position, nullptr);
    CHECK(position.fix_type == GPSPositionReport::FixType::Unknown);
    driver.setDecodeContext({.navigation = true});
    Bytes payload(UBX::WIRE_SIZE<ubx_payload_rx_nav_pvt_t>, 0);
    payload[20] = 3;
    for (const uint8_t flags : std::array<uint8_t, 4>{0, 2, 0x40, 0x80}) {
        payload[21] = UBX_RX_NAV_PVT_FLAGS_GNSSFIXOK;
        CHECK(driver.decode(packet(UBX_MSG_NAV_PVT, payload)).batch.events.size() == 1);
        CHECK(position.fix_type == GPSPositionReport::FixType::Fix3D);
        payload[21] = flags;
        const auto decoded = driver.decode(packet(UBX_MSG_NAV_PVT, payload));
        CHECK(decoded.batch.events.size() == 1);
        CHECK(std::get<GPSNativePositionReport>(decoded.batch.events.front()).fix_type ==
              GPSPositionReport::FixType::NoFix);
        CHECK(!position.vel_ned_valid);
    }
}

static void baudDiscovery()
{
    for (const unsigned initialBaud : {9600U, 115200U}) {
        for (const bool fixed : {false, true}) {
            for (const bool usb : {false, true}) {
                for (const bool loseAck : {false, true}) {
                    gps_test_time = 1000000;
                    Receiver receiver;
                    receiver.receiverBaud = initialBaud;
                    receiver.usb = usb;
                    receiver.loseBaudAck = loseAck;
                    receiver.protocol = "27.31";
                    GPSNativePositionReport position;
                    GPSNativeUBX driver(receiver.io(), &position, nullptr);
                    GPSProtocol::GPSConfig config{};
                    unsigned baud = fixed ? initialBaud : 0;
                    CHECK(driver.configure(baud, config) == 0);
                    CHECK(driver.receiverReady());
                    CHECK(receiver.unidentifiedWrites == 0);
                    CHECK(baud == (fixed ? initialBaud : 115200));
                    CHECK(receiver.receiverBaud == baud);
                    CHECK(receiver.hostBaud == baud);
                    CHECK(receiver.current_settings.at(UBX_CFG_KEY_NAVSPG_DYNMODEL) == 0);
                    CHECK(receiver.current_settings.at(UBX_CFG_KEY_RATE_MEAS) == 200);
                    if (fixed) {
                        CHECK(receiver.hostBauds == std::vector<unsigned>{initialBaud});
                    } else if (!usb) {
                        const std::vector<unsigned> probes = initialBaud == 9600
                                                                 ? std::vector<unsigned>{38400, 57600, 9600}
                                                                 : std::vector<unsigned>{38400, 57600, 9600, 115200};
                        CHECK(receiver.identityBauds.size() >= probes.size());
                        CHECK(std::equal(probes.begin(), probes.end(), receiver.identityBauds.begin()));
                    }
                    CHECK(gps_test_time < 15000000);
                }
            }
        }
    }
    for (const bool oldReceiver : {false, true}) {
        for (const bool loseAck : {false, true}) {
            gps_test_time = 1000000;
            Receiver receiver;
            receiver.legacy = true;
            receiver.module = oldReceiver ? "u-blox6" : "NEO-M8N";
            receiver.hardware = oldReceiver ? "00040007" : "00080000";
            receiver.protocol = oldReceiver ? "" : "15.00";
            receiver.receiverBaud = 9600;
            receiver.loseBaudAck = loseAck;
            GPSNativePositionReport position;
            GPSNativeUBX driver(receiver.io(), &position, nullptr);
            unsigned baud = 0;
            CHECK(driver.configure(baud, {}) == 0);
            CHECK(receiver.unidentifiedWrites == 0);
            CHECK(baud == (oldReceiver ? 38400U : 115200U));
            CHECK(receiver.receiverBaud == baud);
            CHECK(receiver.legacy_measurement_interval == 200);
            CHECK(receiver.current_settings.empty());
        }
    }
}

static void discoveryFailures()
{
    for (unsigned scenario = 0; scenario < 10; ++scenario) {
        gps_test_time = 1000000;
        Receiver receiver;
        receiver.receiverBaud = 9600;
        receiver.hardware = scenario == 0 ? "UNKN0WN!" : "00190000";
        receiver.protocol = scenario == 1 ? "invalid" : "27.31";
        receiver.corruptIdentity = scenario == 2;
        receiver.silencePortConfiguration = scenario == 3;
        receiver.loseBaudAck = scenario >= 4;
        receiver.ignoreBaudChange = scenario == 4 || scenario == 5;
        receiver.usb = scenario == 5;
        receiver.rejectAfterBaudChange = scenario == 6;
        receiver.nakAfterBaudChange = scenario == 8;
        receiver.readback_mode = scenario == 9 ? 1 : 0;
        receiver.module = scenario == 7 ? "NEO-M9N" : "ZED-F9P";
        GPSNativePositionReport position;
        GPSNativeUBX driver(receiver.io(), &position, nullptr);
        GPSProtocol::GPSConfig config{};
        if (scenario == 7) {
            config.output_mode = GPSProtocol::OutputMode::RTCM;
            config.base = {.surveyInAccMeters = 1, .surveyInDurationSecs = 60};
        }
        unsigned baud = 0;
        CHECK(driver.configure(baud, config) < 0);
        CHECK(!driver.receiverReady());
        CHECK(receiver.unidentifiedWrites == 0);
        if (scenario < 3 || scenario == 7) {
            CHECK(receiver.configurationWrites == 0);
        } else if (scenario == 3) {
            CHECK(receiver.configurationWrites == 1);
        } else if (scenario == 6) {
            CHECK(receiver.lateBaudAckDelivered);
            CHECK(receiver.current_settings.count(UBX_CFG_KEY_CFG_USBOUTPROT_UBX) == 0);
        } else if (scenario == 8) {
            CHECK(receiver.lateBaudAckDelivered);
            CHECK(receiver.current_settings.at(UBX_CFG_KEY_CFG_USBOUTPROT_UBX) == 1);
        }
        CHECK(gps_test_time < 20000000);
    }
}

static void navigationFixFlags()
{
    using Fix = GPSPositionReport::FixType;
    constexpr std::array<Fix, 8> FIX_3D{Fix::Fix3D,    Fix::Differential, Fix::RTKFloat, Fix::RTKFloat,
                                        Fix::RTKFixed, Fix::RTKFixed,     Fix::Fix3D,    Fix::Differential};
    constexpr std::array<Fix, 6> UNCORRECTED{Fix::NoFix, Fix::Extrapolated, Fix::Fix2D,
                                             Fix::Fix3D, Fix::Fix3D,        Fix::NoFix};
    for (const bool legacy : {false, true}) {
        for (const unsigned rawFix : {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 255U}) {
            for (unsigned flags = 0; flags <= UINT8_MAX; ++flags) {
                Fix expected = rawFix < UNCORRECTED.size() ? UNCORRECTED[rawFix] : Fix::Unknown;
                if (!(flags & 1)) {
                    expected = Fix::NoFix;
                } else if (rawFix == 3 || rawFix == 4) {
                    expected = legacy ? FIX_3D[(flags >> 1) & 1] : FIX_3D[((flags >> 6) * 2) + ((flags >> 1) & 1)];
                }
                std::array<unsigned, 3> order{0, 1, 2};
                do {
                    gps_test_time = 1000000;
                    GPSNativePositionReport position;
                    GPSNativeUBX driver(makeGPSProtocolTestIO(), &position, nullptr);
                    driver.setDecodeContext({.navigation = true, .useNavPvt = !legacy, .assembleEpochs = true});
                    Bytes pvt(92);
                    (void) LittleEndian::write<uint32_t>(pvt, 0, 1000);
                    pvt[20] = static_cast<uint8_t>(rawFix);
                    pvt[21] = static_cast<uint8_t>(flags);
                    (void) LittleEndian::write<int32_t>(pvt, 24, 80000000);
                    (void) LittleEndian::write<int32_t>(pvt, 28, 470000000);
                    (void) LittleEndian::write<int32_t>(pvt, 60, 12000);
                    if (!legacy) {
                        CHECK(driver.decode(packet(UBX_MSG_NAV_PVT, pvt)).batch.events.empty());
                    } else {
                        std::array<Bytes, 3> payloads{Bytes(28), Bytes(52), Bytes(36)};
                        constexpr std::array<uint16_t, 3> MESSAGES{UBX_MSG_NAV_POSLLH, UBX_MSG_NAV_SOL,
                                                                   UBX_MSG_NAV_VELNED};
                        for (auto& payload : payloads) {
                            (void) LittleEndian::write<uint32_t>(payload, 0, 1000);
                        }
                        (void) LittleEndian::write<int32_t>(payloads[0], 4, 80000000);
                        (void) LittleEndian::write<int32_t>(payloads[0], 8, 470000000);
                        payloads[1][10] = static_cast<uint8_t>(rawFix);
                        payloads[1][11] = static_cast<uint8_t>(flags);
                        (void) LittleEndian::write<uint32_t>(payloads[2], 20, 1200);
                        for (const unsigned index : order) {
                            CHECK(driver.decode(packet(MESSAGES[index], payloads[index])).batch.events.empty());
                        }
                    }
                    Bytes end(4);
                    (void) LittleEndian::write<uint32_t>(end, 0, 1000);
                    const auto decoded = driver.decode(packet(UBX::NAV_EOE, end));
                    CHECK(decoded.batch.events.size() == 1);
                    const auto& fix = std::get<GPSNativePositionReport>(decoded.batch.events.front());
                    CHECK(fix.fix_type == expected);
                    CHECK(fix.vel_ned_valid == (expected != Fix::NoFix && expected != Fix::Unknown));
                    CHECK(fix.latitude_deg == 47 && fix.longitude_deg == 8);
                    CHECK(std::abs(fix.vel_m_s - 12) < 1e-5f);
                } while (legacy && std::next_permutation(order.begin(), order.end()));
            }
        }
    }
}

static void transactionalFrames()
{
    Receiver receiver;
    GPSNativePositionReport position{};
    GPSNativeSatelliteReport satellites{};
    GPSNativeUBX driver(receiver.io(), &position, &satellites);
    unsigned baud = 115200;
    GPSProtocol::GPSConfig config{};
    config.output_mode = GPSProtocol::OutputMode::GPS;
    CHECK(driver.configure(baud, config) == 0);
    Bytes payload(20, 0);
    payload[4] = 1;
    payload[5] = 1;
    payload[8] = 0;
    payload[9] = 17;
    payload[10] = 35;
    const auto valid = packet(UBX_MSG_NAV_SAT, payload);
    payload[9] = 23;
    auto corrupt = packet(UBX_MSG_NAV_SAT, payload);
    corrupt.back() ^= 1;
    Bytes joined = valid;
    joined.insert(joined.end(), corrupt.begin(), corrupt.end());
    auto decoded = driver.decode(joined);
    CHECK(decoded.bytesConsumed == joined.size());
    CHECK(decoded.batch.events.size() == 1);
    CHECK(std::get<GPSNativeSatelliteReport>(decoded.batch.events[0]).entries[0].id == 17);
    CHECK(satellites.entries[0].id == 17);
    CHECK(driver.decode(std::span(valid).first(valid.size() - 1)).batch.events.empty());
    CHECK(satellites.entries[0].id == 17);
    CHECK(driver.decode(std::span(valid).last(1)).batch.events.size() == 1);
    payload[5] = 2;  // A valid checksum cannot make an incomplete counted payload valid.
    CHECK(driver.decode(packet(UBX_MSG_NAV_SAT, payload)).batch.events.empty());
    CHECK(satellites.entries[0].id == 17);
    CHECK(driver.decode(packet(UBX_MSG_NAV_SAT, Bytes(7, 0))).batch.events.empty());
    CHECK(satellites.entries[0].id == 17);
    CHECK(std::get<GPSNativeSatelliteReport>(decoded.batch.events[0]).entries[0].id == 17);
    CHECK(driver.decode(packet(UBX_MSG_NAV_SAT, Bytes{0, 0, 0, 0, 1, 0, 0, 0})).batch.events.size() == 1);
    CHECK(satellites.count == 0);
    const std::string originalModel = driver.modelName();
    Bytes version(70, 0);
    const std::string module = "MOD=NEO-M9N";
    std::copy(module.begin(), module.end(), version.begin() + 40);
    auto badVersion = packet(UBX_MSG_MON_VER, version);
    badVersion.back() ^= 1;
    CHECK(driver.decode(badVersion).batch.events.empty());
    CHECK(driver.modelName() == originalModel);

    Bytes baseVersion(40, 0);
    const std::string firmware = "SPG 4.04";
    std::copy(firmware.begin(), firmware.end(), baseVersion.begin());
    CHECK(driver.decode(packet(UBX_MSG_MON_VER, baseVersion)).batch.events.empty());
    CHECK(driver.firmwareVersion() == firmware);

    Bytes pvt(UBX::WIRE_SIZE<ubx_payload_rx_nav_pvt_t>, 0);
    pvt[20] = 3;
    pvt[21] = 1;
    driver.setDecodeContext({.navigation = true});
    Bytes epochs;
    for (uint8_t index = 1; index <= 20; ++index) {
        pvt[23] = index;
        const auto frame = packet(UBX_MSG_NAV_PVT, pvt);
        epochs.insert(epochs.end(), frame.begin(), frame.end());
    }
    size_t offset = 0;
    unsigned count = 0;
    while (offset < epochs.size()) {
        auto batch = driver.decode(std::span(epochs).subspan(offset));
        CHECK(batch.bytesConsumed > 0);
        CHECK(batch.batch.events.size() <= GPSDecodedBatch::MAX_EVENTS);
        offset += batch.bytesConsumed;
        for (const auto& event : batch.batch.events) {
            CHECK(std::get<GPSNativePositionReport>(event).satellites_used == ++count);
        }
    }
    CHECK(count == 20);
    driver.setDecodeContext({true, true, true});
    Bytes correction{0xd3, 0, 2, 0x3e, 0xd0};
    const auto crc = RTCMFramer::crc24q(correction);
    correction.insert(correction.end(), {uint8_t(crc >> 16), uint8_t(crc >> 8), uint8_t(crc)});
    std::copy(correction.begin(), correction.end(), pvt.begin() + 40);
    const auto embedded = driver.decode(packet(UBX_MSG_NAV_PVT, pvt));
    CHECK(embedded.batch.events.size() == 1);
    CHECK(std::holds_alternative<GPSNativePositionReport>(embedded.batch.events.front()));
    const auto standalone = driver.decode(correction);
    CHECK(standalone.batch.events.size() == 1);
    CHECK(std::holds_alternative<GPSRTCMReport>(standalone.batch.events.front()));
}

static void controlDeadline()
{
    Receiver receiver;
    GPSNativePositionReport position{};
    gps_test_time = 1000000;
    auto io = receiver.io();
    const auto read = io.read;
    const auto write = io.write;
    bool expireRead = false;
    bool verifyWrite = false;
    io.read = [&](auto bytes, GPSDeadline deadline) {
        const auto result = read(bytes, deadline);
        if (expireRead && result.status == GPSReadStatus::Data) {
            gps_test_time = deadline.untilUs;
            expireRead = false;
            verifyWrite = true;
        }
        return result;
    };
    io.write = [&](auto bytes, GPSDeadline deadline) {
        if (verifyWrite) {
            CHECK(deadline.remainingMilliseconds(gps_test_time) > 0);
        }
        return write(bytes, deadline);
    };
    GPSNativeUBX driver(io, &position, nullptr);
    unsigned baud = 115200;
    GPSProtocol::GPSConfig config{};
    config.output_mode = GPSProtocol::OutputMode::GPS;
    CHECK(driver.configure(baud, config) == 0);
    driver.receive(10);  // Drain the configuration responses before the timed warning.
    receiver.read_chunk = GPS_READ_BUFFER_SIZE;
    receiver.bufferWarning();
    expireRead = true;
    driver.receive(10);
    CHECK(verifyWrite);
    CHECK(receiver.comms_polls == 1);
    CHECK(driver.ioError() == 0);
}

static void reentrantPayload()
{
    Receiver receiver;
    GPSNativePositionReport position{};
    auto io = receiver.io();
    GPSNativeUBX* active = nullptr;
    bool reentered = false;
    QStringList warnings;
    Bytes correction{0xd3, 0, 2, 0x3e, 0xd0};
    const auto crc = RTCMFramer::crc24q(correction);
    correction.insert(correction.end(), {uint8_t(crc >> 16), uint8_t(crc >> 8), uint8_t(crc)});
    io.log = [&](GPSProtocolLogLevel, QStringView message) {
        warnings.push_back(message.toString());
        if (!reentered && message == u"ubx msg: txbuf alloc") {
            reentered = true;
            active->setDecodeContext({.navigation = true, .corrections = true});
            for (auto byte : packet(UBX_MSG_INF_WARNING, Bytes{'o', 'k'})) {
                active->decodeByte(byte);
            }
            for (auto byte : std::span(correction).first(4)) {
                active->decodeByte(byte);
            }
        }
    };
    GPSNativeUBX driver(std::move(io), &position, nullptr);
    active = &driver;
    driver.setDecodeContext({.navigation = true, .corrections = true});
    const std::string warning = "txbuf alloc";
    CHECK(driver.decode(packet(UBX_MSG_INF_WARNING, Bytes(warning.begin(), warning.end()))).batch.events.empty());
    CHECK(reentered);
    CHECK(warnings == (QStringList{"ubx msg: txbuf alloc", "ubx msg: ok"}));
    const auto decoded = driver.decode(std::span(correction).subspan(4));
    CHECK(decoded.batch.events.size() == 1);
    CHECK(std::holds_alternative<GPSRTCMReport>(decoded.batch.events.front()));
    CHECK(receiver.transport_operations == 0);
    driver.receive(1);
    CHECK(receiver.comms_polls == 1);
}

static void isolatedFrameAndControl()
{
    UBX::FrameDecoder decoder;
    const Bytes payload = {0x06, 0x24};
    const auto bytes = packet(UBX_MSG_ACK_ACK, payload);
    std::optional<UBX::Frame> frame;
    for (size_t index = 0; index < bytes.size(); ++index) {
        frame = decoder.consume(bytes[index]);
        CHECK(frame.has_value() == (index + 1 == bytes.size()));
    }
    CHECK(frame->message == UBX_MSG_ACK_ACK && frame->length == 2);
    CHECK(frame->payload[0] == 6 && frame->payload[1] == 0x24);
    const auto retained = *frame;
    auto corrupt = bytes;
    corrupt.back() ^= 1;
    for (auto byte : corrupt) {
        CHECK(!decoder.consume(byte));
    }
    CHECK(decoder.idle());
    CHECK(retained.payload[0] == 6);
    const auto longFrame = packet(UBX_MSG_INF_NOTICE, Bytes(4096, 0xa5));
    for (auto byte : longFrame) {
        frame = decoder.consume(byte);
    }
    CHECK(frame && frame->length == 4096 && frame->payload.back() == 0xa5);
    decoder.reset();
    for (auto byte : bytes) {
        decoder.consume(byte);
    }
    CHECK(frame->length == 4096 && frame->payload.front() == 0xa5);
    CHECK(retained.payload[0] == 6 && retained.payload[1] == 0x24);
    for (auto byte : std::span(bytes).first(5)) {
        CHECK(!decoder.consume(byte));
    }
    decoder.reset();
    for (auto byte : bytes) {
        frame = decoder.consume(byte);
    }
    CHECK(frame && frame->message == UBX_MSG_ACK_ACK && frame->length == 2);

    for (size_t prefix = 1; prefix <= 4; ++prefix) {
        decoder.reset();
        for (size_t i = 0; i < prefix; ++i) {
            CHECK(!decoder.consume(0xb5));
        }
        unsigned completed = 0;
        for (auto byte : bytes) {
            if (const auto overlapping = decoder.consume(byte)) {
                ++completed;
                CHECK(overlapping->message == UBX_MSG_ACK_ACK);
                CHECK(overlapping->length == payload.size());
                CHECK(std::equal(payload.begin(), payload.end(), overlapping->payload.begin()));
            }
        }
        CHECK(completed == 1);
        CHECK(decoder.idle());
    }

    UBX::ReceiverController controller;
    controller.beginAcknowledgement(UBX_MSG_CFG_NAV5);
    controller.accept(UBX::Acknowledgement{UBX_MSG_CFG_RATE, true});
    CHECK(controller.acknowledgement() == GPSCommandOutcome::Pending);
    controller.accept(UBX::Acknowledgement{UBX_MSG_CFG_NAV5, false});
    CHECK(controller.acknowledgement() == GPSCommandOutcome::Rejected);
    controller.finishAcknowledgement();
    controller.accept(UBX::Acknowledgement{UBX_MSG_CFG_NAV5, true});
    CHECK(controller.acknowledgement() == GPSCommandOutcome::Rejected);

    const std::array<uint32_t, 2> keys{UBX_CFG_KEY_NAVSPG_DYNMODEL, UBX_CFG_KEY_RATE_MEAS};
    controller.beginReadback(keys);
    UBX::ConfigurationValues values;
    values.count = 2;
    values.keys[0] = values.keys[1] = keys[0];
    controller.accept(values);
    CHECK(!controller.readbackReady());
    values.keys[0] = keys[1];
    values.keys[1] = keys[0];
    values.values[0] = 200;
    values.values[1] = 4;
    controller.accept(values);
    CHECK(controller.readbackReady());
    CHECK(controller.readback().values[0] == 4 && controller.readback().values[1] == 200);
    CHECK(!UBX::decodeConfigurationValues(Bytes{1, 0, 0, 0, 1}));
}

static void checkedWireCodecs()
{
    const auto fixed = []<typename T>() {
        using Codec = UBX::MessageCodec<T>;
        Bytes payload(UBX::WIRE_SIZE<T>, 0);
        CHECK(Codec::decode(payload));
        CHECK(!Codec::decode({}));
        payload.pop_back();
        CHECK(!Codec::decode(payload));
        payload.resize(UBX::WIRE_SIZE<T> + 1);
        CHECK(!Codec::decode(payload));
    };
    fixed.operator()<ubx_payload_rx_nav_posllh_t>();
    fixed.operator()<ubx_payload_rx_nav_dop_t>();
    fixed.operator()<ubx_payload_rx_nav_sol_t>();
    fixed.operator()<ubx_payload_rx_nav_pvt_t>();
    fixed.operator()<ubx_payload_rx_nav_timeutc_t>();
    fixed.operator()<ubx_payload_rx_nav_status_t>();
    fixed.operator()<ubx_payload_rx_nav_svin_t>();
    fixed.operator()<ubx_payload_rx_nav_velned_t>();
    fixed.operator()<ubx_payload_rx_nav_relposned_t>();
    fixed.operator()<ubx_payload_rx_nav_daheading_t>();
    fixed.operator()<ubx_payload_rx_nav_hpposllh_t>();
    fixed.operator()<ubx_payload_rx_ack_ack_t>();
    fixed.operator()<ubx_payload_rx_ack_nak_t>();
    fixed.operator()<ubx_payload_rx_rxm_rtcm_t>();
    fixed.operator()<ubx_payload_rx_mon_hw_ubx6_t>();
    fixed.operator()<ubx_payload_rx_mon_hw_ubx7_t>();
    CHECK(UBX::MessageCodec<ubx_payload_rx_nav_pvt_t>::decode(Bytes(84)));

    Bytes rf(28, 0);
    rf[1] = 1;
    CHECK(UBX::MessageCodec<ubx_payload_rx_mon_rf_t>::decode(rf));
    rf[1] = 2;
    CHECK(!UBX::MessageCodec<ubx_payload_rx_mon_rf_t>::decode(rf));
    rf[1] = 1;
    rf[0] = 99;
    CHECK(!UBX::MessageCodec<ubx_payload_rx_mon_rf_t>::decode(rf));
    Bytes comms(48, 0);
    comms[1] = 1;
    CHECK(UBX::MessageCodec<ubx_payload_rx_mon_comms_t>::decode(comms));
    comms[1] = 2;
    CHECK(!UBX::MessageCodec<ubx_payload_rx_mon_comms_t>::decode(comms));
    CHECK(UBX::MessageCodec<ubx_payload_rx_sec_sig_t>::decode(Bytes{2, 7, 0, 0}));
    CHECK(!UBX::MessageCodec<ubx_payload_rx_sec_sig_t>::decode(Bytes{99, 7, 0, 0}));
    CHECK(!UBX::MessageCodec<ubx_payload_rx_sec_sig_t>::decode(Bytes{2, 7, 0, 1}));
    CHECK(!UBX::MessageCodec<ubx_payload_rx_nav_sat_part2_t>::block(Bytes(12), 1));
    const auto ack = UBX::MessageCodec<ubx_payload_rx_ack_ack_t>::decode(Bytes{6, 0x24});
    CHECK(ack && ack->msg == UBX_MSG_CFG_NAV5);
}

const auto& testCases()
{
    static const struct
    {
        const char* name;
        void (*run)();
    } cases[] = {
        {"isolated-frame-control", isolatedFrameAndControl},
        {"checked-wire-codecs", checkedWireCodecs},
        {"position-f9p", [] { positionMode(false, true); }},
        {"receiver-settings", receiverSettings},
        {"native-configuration-validation", invalidConfiguration},
        {"integrity-original-receipts", integrityReceipts},
        {"position-m9n", [] { positionMode(false, false); }},
        {"position-m8p", [] { positionMode(true, true); }},
        {"position-m8n", [] { positionMode(true, false); }},
        {"position-stop-failures", [] { positionModeFailure(); }},
        {"control-deadline", controlDeadline},
        {"explicit-no-fix", explicitNoFix},
        {"read-only-baud-discovery", baudDiscovery},
        {"discovery-failures-and-late-acks", discoveryFailures},
        {"navigation-fix-flags-and-ordering", navigationFixFlags},
        {"transactional-frames", transactionalFrames},
        {"reentrant-payload", reentrantPayload},
        {"receive-read-diagnostics", receiveFailureLogging},
        {"comms-diagnostic-values", commsDiagnostics},
        {"comms-malformed-replies", invalidCommsDiagnostics},
        {"comms-expired-reply", expiredCommsDiagnostics},
        {"buffer-warning-rate-limit",
         [] {
             Fixture f;
             f.receiver.bufferWarning(false);
             f.driver.receive(100);
             CHECK(f.receiver.comms_polls == 0);
             f.receiver.bufferWarning();
             f.driver.receive(100);
             CHECK(f.receiver.comms_polls == 1);
             f.receiver.bufferWarning();
             f.driver.receive(100);
             CHECK(f.receiver.comms_polls == 1);
             gps_test_time += 5000000;
             f.receiver.bufferWarning();
             f.driver.receive(100);
             CHECK(f.receiver.comms_polls == 2);
         }},
        {"buffer-poll-failure-rate-limit",
         [] {
             Fixture f;
             f.receiver.fail_comms_write = true;
             f.receiver.bufferWarning();
             f.driver.receive(100);
             f.receiver.bufferWarning();
             f.driver.receive(100);
             CHECK(f.receiver.comms_polls == 1);
             gps_test_time += 5000000;
             f.receiver.fail_comms_write = false;
             f.receiver.bufferWarning();
             f.driver.receive(100);
             CHECK(f.receiver.comms_polls == 2);
         }},
        {"already-stopped",
         [] {
             Fixture f;
             f.success(1);
         }},
        {"poll-read-failure",
         [] {
             Fixture f;
             f.readFailure(-1);
         }},
        {"poll-read-errno",
         [] {
             Fixture f;
             f.readFailure(-EIO);
         }},
        {"poll-read-cancelled",
         [] {
             Fixture f;
             f.readFailure(GPSProtocol::ReadCancelled);
         }},
        {"silent-then-stopped",
         [] {
             Fixture f;
             f.receiver.replies = {SurveyReply::silent, SurveyReply::stopped};
             f.success(2);
         }},
        {"delayed-stop",
         [] {
             Fixture f;
             f.receiver.replies = {SurveyReply::active, SurveyReply::active, SurveyReply::stopped};
             f.success(3);
             CHECK(f.receiver.started_at - f.receiver.disabled_at >= 200000);
         }},
        {"completed-survey-is-not-stopped",
         [] {
             Fixture f;
             f.receiver.replies = {SurveyReply::valid, SurveyReply::stopped};
             f.success(2);
         }},
        {"bad-checksum-is-not-confirmation",
         [] {
             Fixture f;
             f.receiver.replies = {SurveyReply::bad_checksum, SurveyReply::stopped};
             f.success(2);
         }},
        {"bad-length-is-not-confirmation",
         [] {
             Fixture f;
             f.receiver.replies = {SurveyReply::bad_length, SurveyReply::stopped};
             f.success(2);
         }},
        {"active-timeout",
         [] {
             Fixture f;
             f.receiver.replies = {SurveyReply::active};
             f.timeout();
         }},
        {"valid-timeout",
         [] {
             Fixture f;
             f.receiver.replies = {SurveyReply::valid};
             f.timeout();
         }},
        {"silent-timeout",
         [] {
             Fixture f;
             f.receiver.replies = {SurveyReply::silent};
             f.timeout();
         }},
        {"reconfigure-does-not-reuse-stop-confirmation",
         [] {
             Fixture f;
             f.success(1);
             f.receiver = Receiver{};
             f.receiver.replies = {SurveyReply::silent};
             f.timeout();
         }},
        {"disable-nak",
         [] {
             Fixture f;
             f.receiver.reject_disable = true;
             CHECK(f.configure() < 0);
             CHECK(!f.driver.receiverReady());
             CHECK(f.receiver.modes == std::vector<uint32_t>({0}));
             CHECK(f.receiver.polls == 0 && f.receiver.starts == 0);
         }},
        {"start-nak",
         [] {
             Fixture f;
             f.receiver.reject_start = true;
             CHECK(f.configure() < 0);
             CHECK(!f.driver.receiverReady());
             CHECK(f.receiver.modes == std::vector<uint32_t>({0, 1}));
             CHECK(f.receiver.polls == 1 && f.receiver.starts == 1);
         }},
        {"poll-write-failure",
         [] {
             Fixture f;
             f.receiver.fail_poll_write = true;
             CHECK(f.configure() < 0);
             CHECK(!f.driver.receiverReady());
             CHECK(f.receiver.modes == std::vector<uint32_t>({0}));
             CHECK(f.receiver.starts == 0);
         }},
        {"configured-status-callback",
         [] {
             Fixture f;
             f.success(1);
             f.receiver.survey(SurveyReply::active);
             f.driver.receive(100);
             CHECK(f.receiver.status_callbacks == 1);
         }},
        {"configured-survey-activates-rtcm",
         [] {
             Fixture f;
             f.success(1);
             f.receiver.survey(SurveyReply::valid);
             f.driver.receive(100);
             CHECK(f.receiver.status_callbacks == 1);
             CHECK(f.receiver.rtcm_enables == 1);
             CHECK(f.driver.ioError() == 0);
         }},
        {"fixed-base-does-not-poll",
         [] {
             Fixture f;
             f.base = {.useFixedBase = true,
                       .fixedPosition = {.latitudeDegrees = 47.0, .longitudeDegrees = 8.0, .altitudeMeters = 500.0f},
                       .fixedBaseAccuracyMeters = 1.0f};
             CHECK(f.configure() == 0);
             CHECK(f.driver.receiverReady());
             CHECK(f.receiver.modes == std::vector<uint32_t>({2}));
             CHECK(f.receiver.polls == 0 && f.receiver.starts == 0);
             CHECK(f.receiver.rtcm_enables == 1);
         }},
    };

    return cases;
}
}  // namespace

class GPSProtocolUbxTest : public UnitTest
{
    Q_OBJECT

private slots:

    void _cases_data();
    void _cases();
};

void GPSProtocolUbxTest::_cases_data()
{
    QTest::addColumn<int>("index");
    const auto& cases = testCases();
    for (size_t index = 0; index < std::size(cases); ++index) {
        QTest::newRow(cases[index].name) << static_cast<int>(index);
    }
}

void GPSProtocolUbxTest::_cases()
{
    QFETCH(int, index);
    gps_test_time = 0;
    gps_test_warnings.clear();
    try {
        testCases()[index].run();
    } catch (const std::exception& error) {
        QFAIL(error.what());
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSProtocolUbxTest, TestLabel::Unit)

#include "gps-ubx-test.moc"
