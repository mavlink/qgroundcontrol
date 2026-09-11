#include <algorithm>
#include <cstdio>
#include <deque>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "GPSProtocolTestIO.h"
#include "UBX/GPSDriverUBX.h"

// Keep checks active in Release, too.
#define CHECK(condition)                                                                                 \
    do {                                                                                                 \
        if (!(condition)) {                                                                              \
            throw std::runtime_error(std::string("line ") + std::to_string(__LINE__) + ": " #condition); \
        }                                                                                                \
    } while (0)

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
    int readback_mode = 0;
    unsigned readback_requests = 0;
    std::vector<SurveyReply> replies{SurveyReply::stopped};
    bool reject_disable = false;
    bool reject_nmea = false;
    bool reject_constellations = false;
    bool timeout_constellations = false;
    bool cancel_nmea = false;
    std::map<unsigned, unsigned> output_protocols;
    bool legacy = false;
    std::string module = "ZED-F9P";
    bool reject_start = false;
    bool fail_poll_write = false;
    int poll_read_error = 0;
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
    gps_abstime disabled_at = 0;
    gps_abstime started_at = 0;

    static int callback(GPSCallbackType type, void* data, int size, void* user)
    {
        return static_cast<Receiver*>(user)->handle(type, data, size);
    }

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

    void process(const Bytes& bytes)
    {
        const auto message = uint16_t(littleEndian(bytes, 2, 2));
        const Bytes payload(bytes.begin() + 6, bytes.end() - 2);
        CHECK(bytes == packet(message, payload));  // Validate outgoing framing/checksum.

        if (message == UBX_MSG_MON_COMMS) {
            CHECK(payload.empty());
            ++comms_polls;
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
            memcpy(version.data() + 30, legacy ? "00080000" : module == "ZED-X20P" ? "000B0000" : "00190000", 8);
            const std::string identity = "MOD=" + module;
            CHECK(identity.size() < 30);
            memcpy(version.data() + 40, identity.c_str(), identity.size());
            queue(packet(message, version));
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
            if (message == UBX_MSG_CFG_PRT) {
                for (size_t offset = 0; offset < payload.size(); offset += 20) {
                    output_protocols[payload.at(offset)] = littleEndian(payload, offset + 14, 2);
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
                disabled_at = gps_test_time;
                reject = reject_disable && mode == 0;
            }

            queue(packet(reject ? UBX_MSG_ACK_NAK : UBX_MSG_ACK_ACK, {uint8_t(message), uint8_t(message >> 8)}));
            return;
        }

        CHECK(message == UBX_MSG_CFG_VALSET);
        CHECK(payload.size() >= 4);
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

        if (cancel_nmea && settings.count(UBX_CFG_KEY_CFG_USBOUTPROT_NMEA) &&
            settings.at(UBX_CFG_KEY_CFG_USBOUTPROT_NMEA) == 1) {
            polls = 1;
            poll_read_error = GPSProtocol::ReadCancelled;
        }
        if (timeout_constellations && settings.count(UBX_CFG_KEY_SIGNAL_GPS_ENA)) {
            return;
        }
        bool reject = reject_nmea && settings.count(UBX_CFG_KEY_CFG_USBOUTPROT_NMEA) &&
                      settings.at(UBX_CFG_KEY_CFG_USBOUTPROT_NMEA) == 1;
        reject |= reject_constellations && settings.count(UBX_CFG_KEY_SIGNAL_GPS_ENA);
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

        queue(packet(reject ? UBX_MSG_ACK_NAK : UBX_MSG_ACK_ACK, {uint8_t(message), uint8_t(message >> 8)}));
    }

    int handle(GPSCallbackType type, void* data, int size)
    {
        if (type == GPSCallbackType::readDeviceData) {
            const auto request = *static_cast<const GPSReadRequest*>(data);
            const int timeout = request.timeoutMs;
            data = request.buffer;
            CHECK(timeout >= 0);

            if (polls > 0 && poll_read_error < 0) {
                ++failed_reads;
                gps_test_time += 1000;
                return poll_read_error;
            }

            if (incoming.empty()) {
                // The driver's deadlines use strict comparisons; move past the timeout.
                gps_test_time += uint64_t(timeout) * 1000 + 1;
                return 0;
            }

            gps_test_time += 1000;
            const size_t count = std::min({incoming.size(), size_t(size), read_chunk});

            for (size_t i = 0; i < count; ++i) {
                static_cast<uint8_t*>(data)[i] = incoming.front();
                incoming.pop_front();
            }

            return static_cast<int>(count);
        }

        if (type == GPSCallbackType::writeDeviceData) {
            const auto* bytes = static_cast<const uint8_t*>(data);

            if (fail_comms_write && outgoing.empty() && size >= 4 && bytes[2] == 0x0a && bytes[3] == 0x36) {
                ++comms_polls;
                return -1;
            }

            if (fail_poll_write && outgoing.empty() && size >= 4 && bytes[2] == 0x01 && bytes[3] == 0x3b) {
                return -1;
            }

            outgoing.insert(outgoing.end(), bytes, bytes + size);

            if (outgoing.size() >= 6 && outgoing.size() == littleEndian(outgoing, 4, 2) + 8) {
                const Bytes complete = std::move(outgoing);
                outgoing.clear();
                process(complete);
            }

            return size;
        }

        if (type == GPSCallbackType::surveyInStatus) {
            ++status_callbacks;
        }

        return 0;
    }
};

struct Fixture
{
    Receiver receiver;
    GPSPositionReport position{};
    GPSDriverUBX driver;
    GPSBaseStationConfig base;

    Fixture()
        : driver(makeGPSProtocolTestIO(Receiver::callback, &receiver), &position, nullptr)
    {
        gps_test_time = 0;
        gps_test_warnings.clear();
        base.surveyInAccMeters = 1.25;
        base.surveyInDurationSecs = 60;
    }

    int configure(GPSProtocol::OutputMode output = GPSProtocol::OutputMode::RTCM,
                  GPSDriverUBX::OutputProtocol protocol = GPSDriverUBX::OutputProtocol::Native)
    {
        unsigned baudrate = 115200;
        GPSProtocol::GPSConfig config{};
        config.output_mode = output;
        config.base = base;
        return protocol == GPSDriverUBX::OutputProtocol::Native ? driver.configure(baudrate, config)
                                                                : driver.configure(baudrate, config, protocol);
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

        if (error == GPSProtocol::ReadCancelled) {
            CHECK(gps_test_warnings.empty());

        } else {
            CHECK(gps_test_warnings == std::vector<std::string>{"ubx poll_or_read err"});
        }
    }
};

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
              .fixedBaseLatitude = 47.0,
              .fixedBaseLongitude = 8.0,
              .fixedBaseAltitudeMeters = 500.0f,
              .fixedBaseAccuracyMeters = 1.0f};
    CHECK(f.configure(GPSProtocol::OutputMode::GPS) == 0);
    CHECK(f.driver.receiverReady());
    CHECK(f.receiver.modes == (base_capable ? std::vector<uint32_t>{0} : std::vector<uint32_t>{}));
    CHECK(f.receiver.polls == (base_capable ? 3u : 0u));
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

    Bytes pvt(sizeof(ubx_payload_rx_nav_pvt_t), 0);
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
    CHECK(f.position.fix_type == 3);
    CHECK(f.position.latitude_deg == 20.0);
    CHECK(f.position.longitude_deg == 10.0);
    CHECK(f.position.satellites_used == 12);
}

static void positionModeFailure()
{
    for (bool legacy : {false, true}) {
        for (int failure = 0; failure < 5; ++failure) {
            Fixture f;
            f.receiver.legacy = legacy;
            f.receiver.module = legacy ? "NEO-M8P" : "ZED-F9P";
            f.receiver.reject_disable = failure == 0;
            f.receiver.fail_poll_write = failure == 1;
            if (failure == 2) {
                f.receiver.replies = {SurveyReply::active};
            }
            if (failure >= 3) {
                f.receiver.poll_read_error = failure == 3 ? -EIO : GPSProtocol::ReadCancelled;
            }
            CHECK(f.configure(GPSProtocol::OutputMode::GPS) < 0);
            CHECK(!f.driver.receiverReady());
            CHECK(f.receiver.modes == std::vector<uint32_t>{0});
            CHECK(f.receiver.starts == 0 && f.receiver.rtcm_enables == 0);
            CHECK(f.receiver.status_callbacks == 0);
            if (failure == 2) {
                CHECK(gps_test_warnings == std::vector<std::string>{"Time mode did not stop"});
            }
            if (failure == 4) {
                CHECK(gps_test_warnings.empty());
            }
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
    Bytes mon_rf(sizeof(ubx_payload_rx_mon_rf_t), 0);
    mon_rf[1] = 1;
    mon_rf[5] = 3;
    f.receiver.queue(packet(UBX_MSG_MON_RF, mon_rf));
    f.driver.receive(100);
    CHECK(f.position.jamming_state == 3);
    const auto rf_stamp = f.position.jamming_state_timestamp;
    CHECK(rf_stamp != 0);

    Bytes nav_status(sizeof(ubx_payload_rx_nav_status_t), 0);
    nav_status[7] = 1 << UBX_RX_NAV_STATUS_SPOOFDETSTATE_SHIFT;
    f.receiver.queue(packet(UBX_MSG_NAV_STATUS, nav_status));
    f.driver.receive(100);
    const auto spoof_stamp = f.position.spoofing_state_timestamp;
    CHECK(spoof_stamp != 0);
    CHECK(f.position.jamming_state_timestamp == rf_stamp);

    Bytes pvt(sizeof(ubx_payload_rx_nav_pvt_t), 0);
    pvt[20] = 3;
    pvt[21] = 1;
    for (int i = 0; i < 10; ++i) {
        gps_test_time += 1000000;
        f.receiver.queue(packet(UBX_MSG_NAV_PVT, pvt));
        CHECK(f.driver.receive(100) & 1);
        CHECK(f.position.timestamp > rf_stamp);
        CHECK(f.position.jamming_state_timestamp == rf_stamp);
        CHECK(f.position.spoofing_state_timestamp == spoof_stamp);
    }
    Bytes corrupt = packet(UBX_MSG_MON_RF, mon_rf);
    corrupt.back() ^= 0xff;
    f.receiver.queue(corrupt);
    f.driver.receive(100);
    CHECK(f.position.jamming_state_timestamp == rf_stamp);
    f.receiver.queue(packet(UBX_MSG_MON_RF, mon_rf));
    f.driver.receive(100);
    CHECK(f.position.jamming_state == 3);
    CHECK(f.position.jamming_state_timestamp > rf_stamp);

    Bytes sec_sig(4, 0);
    sec_sig[0] = 2;
    sec_sig[1] = 1 | (3 << 1);
    f.receiver.queue(packet(UBX_MSG_SEC_SIG, sec_sig));
    f.driver.receive(100);
    CHECK(f.position.jamming_state == 3);
    const auto sec_stamp = f.position.jamming_state_timestamp;
    gps_test_time += 6000000;
    f.receiver.queue(packet(UBX_MSG_NAV_PVT, pvt));
    CHECK(f.driver.receive(100) & 1);
    CHECK(f.position.jamming_state_timestamp == sec_stamp);
    f.receiver.queue(packet(UBX_MSG_SEC_SIG, sec_sig));
    f.driver.receive(100);
    CHECK(f.position.jamming_state == 3);
    CHECK(f.position.jamming_state_timestamp > sec_stamp);

    Bytes rtcm(sizeof(ubx_payload_rx_rxm_rtcm_t), 0);
    rtcm[1] = 2 << UBX_RX_RXM_RTCM_MSGUSED_SHIFT;
    f.receiver.queue(packet(UBX_MSG_RXM_RTCM, rtcm));
    f.driver.receive(100);
    CHECK(f.position.corrections_msg_used == 2);
    const auto correction_stamp = f.position.corrections_timestamp;
    CHECK(correction_stamp != 0);
    gps_test_time += 6000000;
    f.receiver.queue(packet(UBX_MSG_NAV_PVT, pvt));
    CHECK(f.driver.receive(100) & 1);
    CHECK(f.position.corrections_timestamp == correction_stamp);
    Bytes cor(sizeof(ubx_payload_rx_rxm_cor_t), 0);
    cor[4] = 29;
    cor[5] = 1;  // msgUsed=2 in statusInfo bits 8..7.
    f.receiver.queue(packet(UBX_MSG_RXM_COR, cor));
    f.driver.receive(100);
    CHECK(f.position.corrections_protocol == GPSPositionReport::CORRECTIONS_PROTOCOL_PMP);
    CHECK(f.position.corrections_msg_used == 2);
    CHECK(f.position.corrections_timestamp > correction_stamp);
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
    CHECK(gps_test_warnings == std::vector<std::string>{"ubx msg: txbuf alloc"});
    gps_test_warnings.clear();
    f.receiver.queue(reply);
    CHECK(f.driver.receive(100) < 0);  // Diagnostic traffic alone is not a position update.
    const std::vector<std::string> expected{"MON-COMMS after txbuf: txErrors=0x02 ports=2 (snapshot after warning)",
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
              std::vector<std::string>{"MON-COMMS after txbuf: txErrors=0x00 ports=0 (snapshot after warning)"});
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

static void nmeaOutput(bool legacy, bool base_capable)
{
    Fixture f;
    f.receiver.legacy = legacy;
    f.receiver.module = legacy ? "NEO-M8P" : (base_capable ? "ZED-F9P" : "NEO-M9N");
    CHECK(f.configure(GPSProtocol::OutputMode::GPS, GPSDriverUBX::OutputProtocol::NMEA) == 0);
    CHECK(f.driver.receiverReady());
    if (legacy) {
        CHECK((f.receiver.output_protocols.at(1) & 2) != 0);
        CHECK((f.receiver.output_protocols.at(3) & 2) != 0);
        for (const uint16_t message : {uint16_t(0x04f0), uint16_t(0x00f0), uint16_t(0x02f0), uint16_t(0x03f0)}) {
            CHECK(f.receiver.message_rates.at(message) == 1);
        }
    } else {
        CHECK(f.receiver.current_settings.at(UBX_CFG_KEY_CFG_UART1OUTPROT_NMEA) == 1);
        CHECK(f.receiver.current_settings.at(UBX_CFG_KEY_CFG_USBOUTPROT_NMEA) == 1);
        for (const uint32_t key : {0x209100abu, 0x209100bau, 0x209100bfu, 0x209100c4u}) {
            CHECK(f.receiver.current_settings.at(key + 1) == 1);
            CHECK(f.receiver.current_settings.at(key + 3) == 1);
            CHECK(f.receiver.current_settings.count(key + 2) == 0);
        }
        f.receiver.reject_nmea = true;
        CHECK(f.configure(GPSProtocol::OutputMode::GPS, GPSDriverUBX::OutputProtocol::NMEA) < 0);
    }
}

static void nmeaOutputFailures()
{
    {
        Fixture f;
        f.receiver.cancel_nmea = true;
        CHECK(f.configure(GPSProtocol::OutputMode::GPS, GPSDriverUBX::OutputProtocol::NMEA) < 0);
        CHECK(f.receiver.failed_reads == 1);
        CHECK(!f.driver.receiverReady());
        CHECK(gps_test_warnings.empty());
    }

    {
        Fixture f;
        CHECK(f.configure(GPSProtocol::OutputMode::RTCM, GPSDriverUBX::OutputProtocol::NMEA) < 0);
        CHECK(f.receiver.current_settings.empty());
        CHECK(!f.driver.receiverReady());
    }
    {
        Fixture f;
        CHECK(f.configure(GPSProtocol::OutputMode::GPS, static_cast<GPSDriverUBX::OutputProtocol>(99)) < 0);
        CHECK(f.receiver.current_settings.empty());
    }
}

static void receiverSettings()
{
    for (int scenario = 0; scenario < 5; ++scenario) {
        gps_test_time = 0;
        gps_test_warnings.clear();
        Receiver receiver;
        receiver.legacy = scenario == 2;
        receiver.module = receiver.legacy ? "NEO-M8P" : scenario == 1 ? "NEO-M9N" : "ZED-F9P";
        receiver.reject_constellations = scenario == 3;
        receiver.timeout_constellations = scenario == 4;
        GPSPositionReport position{};
        GPSDriverUBX driver(makeGPSProtocolTestIO(Receiver::callback, &receiver), &position, nullptr);
        GPSProtocol::GPSConfig config{};
        config.dynamicModel = 4;
        config.outputRateHz = 5;
        config.output_mode = GPSProtocol::OutputMode::GPS;
        config.gnss_systems = receiver.legacy ? GPSProtocol::GNSSSystemsMask::RECEIVER_DEFAULTS
                                              : static_cast<GPSProtocol::GNSSSystemsMask>(5);
        config.require_gnss_config = !receiver.legacy;
        unsigned baudrate = 115200;
        CHECK((driver.configure(baudrate, config) == 0) == (scenario < 2));
        CHECK(driver.supportsOutputRateSelection() == !receiver.legacy);
        if (scenario >= 3) {
            CHECK(driver.settingOutcome(0) == GPSCommandOutcome::Acknowledged);
            CHECK(driver.settingOutcome(1) == GPSCommandOutcome::Acknowledged);
            CHECK(driver.settingOutcome(2) ==
                  (scenario == 3 ? GPSCommandOutcome::Rejected : GPSCommandOutcome::TimedOut));
        }
        CHECK(driver.constellationRequestRejected() == (scenario == 3));
        CHECK(driver.constellationConfigurationRejected() == (scenario >= 3));
        if (scenario < 2) {
            CHECK(receiver.current_settings.at(UBX_CFG_KEY_NAVSPG_DYNMODEL) == 4);
            CHECK(receiver.current_settings.at(UBX_CFG_KEY_RATE_MEAS) == 200);
            CHECK(receiver.current_settings.at(UBX_CFG_KEY_SIGNAL_GPS_ENA) == 1);
            CHECK(receiver.current_settings.at(UBX_CFG_KEY_SIGNAL_GAL_ENA) == 1);
            CHECK(receiver.current_settings.at(UBX_CFG_KEY_SIGNAL_BDS_ENA) == 0);
            CHECK(gps_test_warnings.empty());
        } else {
            CHECK(!driver.receiverReady());
        }
    }
}

static void configurationReadback()
{
    for (int mode = 0; mode < 7; ++mode) {
        Fixture f;
        CHECK(f.configure(GPSProtocol::OutputMode::GPS) == 0);
        f.receiver.readback_mode = mode;
        f.receiver.current_settings[UBX_CFG_KEY_NAVSPG_DYNMODEL] = 7;
        f.receiver.current_settings[UBX_CFG_KEY_SIGNAL_GPS_ENA] = 1;
        f.receiver.current_settings[UBX_CFG_KEY_SIGNAL_QZSS_ENA] = 1;
        f.receiver.current_settings[UBX_CFG_KEY_SIGNAL_GAL_ENA] = 1;
        const gps_abstime started = gps_test_time;
        GPSDriverUBX::ConfigurationReadback report;
        CHECK(f.driver.readConfiguration(report, 500) == (mode == 0));
        CHECK(f.receiver.readback_requests == 1);
        CHECK(gps_test_time - started <= 600000);
        CHECK(f.driver.receiverReady());
        CHECK(f.driver.ioError() == 0);
        if (mode == 0) {
            CHECK(report.dynamic_model == 7);
            CHECK(report.measurement_interval_ms == 200 && report.navigation_rate == 1);
            CHECK(report.constellations_reported && report.constellation_mask == 5);
        }
    }
    Fixture cancelled;
    CHECK(cancelled.configure(GPSProtocol::OutputMode::GPS) == 0);
    cancelled.receiver.poll_read_error = GPSProtocol::ReadCancelled;
    GPSDriverUBX::ConfigurationReadback report;
    CHECK(!cancelled.driver.readConfiguration(report, 500));
    CHECK(cancelled.driver.ioError() == GPSProtocol::ReadCancelled);
}

static void transactionalFrames()
{
    Receiver receiver;
    GPSPositionReport position{};
    GPSSatelliteReport satellites{};
    GPSDriverUBX driver(makeGPSProtocolTestIO(Receiver::callback, &receiver), &position, &satellites);
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
    CHECK(std::get<GPSSatelliteReport>(decoded.batch.events[0]).entries[0].id == 17);
    CHECK(satellites.entries[0].id == 17);
    CHECK(driver.decode(std::span(valid).first(valid.size() - 1)).batch.events.empty());
    CHECK(satellites.entries[0].id == 17);
    CHECK(driver.decode(std::span(valid).last(1)).batch.events.size() == 1);
    payload[5] = 2;  // A valid checksum cannot make an incomplete counted payload valid.
    CHECK(driver.decode(packet(UBX_MSG_NAV_SAT, payload)).batch.events.empty());
    CHECK(satellites.entries[0].id == 17);
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

    Bytes pvt(sizeof(ubx_payload_rx_nav_pvt_t), 0);
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
            CHECK(std::get<GPSPositionReport>(event).satellites_used == ++count);
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
    CHECK(std::holds_alternative<GPSPositionReport>(embedded.batch.events.front()));
    const auto standalone = driver.decode(correction);
    CHECK(standalone.batch.events.size() == 1);
    CHECK(std::holds_alternative<GPSRTCMReport>(standalone.batch.events.front()));
}

static void controlDeadline()
{
    Receiver receiver;
    GPSPositionReport position{};
    gps_test_time = 1000000;
    auto io = makeGPSProtocolTestIO(Receiver::callback, &receiver);
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
        if (verifyWrite)
            CHECK(deadline.remainingMilliseconds(gps_test_time) > 0);
        return write(bytes, deadline);
    };
    GPSDriverUBX driver(io, &position, nullptr);
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

int main()
{
    const struct
    {
        const char* name;
        void (*run)();
    } cases[] = {
        {"position-f9p", [] { positionMode(false, true); }},
        {"receiver-settings", receiverSettings},
        {"configuration-readback", configurationReadback},
        {"integrity-original-receipts", integrityReceipts},
        {"nmea-failures", nmeaOutputFailures},
        {"nmea-f9p", [] { nmeaOutput(false, true); }},
        {"nmea-m9n", [] { nmeaOutput(false, false); }},
        {"nmea-legacy", [] { nmeaOutput(true, true); }},
        {"position-m9n", [] { positionMode(false, false); }},
        {"position-m8p", [] { positionMode(true, true); }},
        {"position-m8n", [] { positionMode(true, false); }},
        {"position-stop-failures", [] { positionModeFailure(); }},
        {"control-deadline", controlDeadline},
        {"transactional-frames", transactionalFrames},
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
                       .fixedBaseLatitude = 47.0,
                       .fixedBaseLongitude = 8.0,
                       .fixedBaseAltitudeMeters = 500.0f,
                       .fixedBaseAccuracyMeters = 1.0f};
             CHECK(f.configure() == 0);
             CHECK(f.driver.receiverReady());
             CHECK(f.receiver.modes == std::vector<uint32_t>({2}));
             CHECK(f.receiver.polls == 0 && f.receiver.starts == 0);
             CHECK(f.receiver.rtcm_enables == 1);
         }},
    };

    bool success = true;

    for (const auto& test : cases) {
        try {
            test.run();
            std::printf("PASS %s\n", test.name);

        } catch (const std::exception& error) {
            std::fprintf(stderr, "FAIL %s: %s\n", test.name, error.what());
            success = false;
        }
    }

    return success ? 0 : 1;
}
