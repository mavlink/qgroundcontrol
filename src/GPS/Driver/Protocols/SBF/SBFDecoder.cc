#include <cmath>
#include <cstddef>
#include <numbers>
#include <string.h>

#include "LittleEndian.h"
#include "RTCMFramer.h"
#include "SBF/GPSDriverSBF.h"

namespace {
constexpr double DNU = 100000.0;

GPSPositionReport::FixType fixType(uint8_t mode)
{
    switch (mode) {
        case 0:
            return GPSPositionReport::FixType::NoFix;
        case 2:
        case 6:
            return GPSPositionReport::FixType::Differential;
        case 5:
        case 8:
            return GPSPositionReport::FixType::RTKFloat;
        case 4:
        case 7:
            return GPSPositionReport::FixType::RTKFixed;
        default:
            return GPSPositionReport::FixType::Fix3D;
    }
}

sbf_buf_t decodeBlock(std::span<const uint8_t> bytes)
{
    sbf_buf_t value{};
    value.sync = LittleEndian::read<uint16_t>(bytes, 0).value_or(0);
    value.crc16 = LittleEndian::read<uint16_t>(bytes, 2).value_or(0);
    const auto id = LittleEndian::read<uint16_t>(bytes, 4).value_or(0);
    value.msg_id = id & 0x1fff;
    value.msg_revision = id >> 13;
    value.length = LittleEndian::read<uint16_t>(bytes, 6).value_or(0);
    value.TOW = LittleEndian::read<uint32_t>(bytes, 8).value_or(0);
    value.WNc = LittleEndian::read<uint16_t>(bytes, 12).value_or(0);
    switch (value.msg_id) {
        case SBF_ID_PVTGeodetic:
            value.payload_pvt_geodetic.mode_type = (LittleEndian::read<uint8_t>(bytes, 14).value_or(0) >> 0) & 15;
            value.payload_pvt_geodetic.mode_reserved = (LittleEndian::read<uint8_t>(bytes, 14).value_or(0) >> 4) & 3;
            value.payload_pvt_geodetic.mode_base_fixed = (LittleEndian::read<uint8_t>(bytes, 14).value_or(0) >> 6) & 1;
            value.payload_pvt_geodetic.mode_2d = (LittleEndian::read<uint8_t>(bytes, 14).value_or(0) >> 7) & 1;
            value.payload_pvt_geodetic.error = LittleEndian::read<uint8_t>(bytes, 15).value_or(0);
            value.payload_pvt_geodetic.latitude = LittleEndian::read<double>(bytes, 16).value_or(0);
            value.payload_pvt_geodetic.longitude = LittleEndian::read<double>(bytes, 24).value_or(0);
            value.payload_pvt_geodetic.height = LittleEndian::read<double>(bytes, 32).value_or(0);
            value.payload_pvt_geodetic.undulation = LittleEndian::read<float>(bytes, 40).value_or(0);
            value.payload_pvt_geodetic.vn = LittleEndian::read<float>(bytes, 44).value_or(0);
            value.payload_pvt_geodetic.ve = LittleEndian::read<float>(bytes, 48).value_or(0);
            value.payload_pvt_geodetic.vu = LittleEndian::read<float>(bytes, 52).value_or(0);
            value.payload_pvt_geodetic.cog = LittleEndian::read<float>(bytes, 56).value_or(0);
            value.payload_pvt_geodetic.rx_clk_bias = LittleEndian::read<double>(bytes, 60).value_or(0);
            value.payload_pvt_geodetic.RxClkDrift = LittleEndian::read<float>(bytes, 68).value_or(0);
            value.payload_pvt_geodetic.time_system = LittleEndian::read<uint8_t>(bytes, 72).value_or(0);
            value.payload_pvt_geodetic.datum = LittleEndian::read<uint8_t>(bytes, 73).value_or(0);
            value.payload_pvt_geodetic.nr_sv = LittleEndian::read<uint8_t>(bytes, 74).value_or(0);
            value.payload_pvt_geodetic.wa_corr_info = LittleEndian::read<uint8_t>(bytes, 75).value_or(0);
            value.payload_pvt_geodetic.reference_id = LittleEndian::read<uint16_t>(bytes, 76).value_or(0);
            value.payload_pvt_geodetic.mean_corr_age = LittleEndian::read<uint16_t>(bytes, 78).value_or(0);
            value.payload_pvt_geodetic.signal_info = LittleEndian::read<uint32_t>(bytes, 80).value_or(0);
            value.payload_pvt_geodetic.alert_flag = LittleEndian::read<uint8_t>(bytes, 84).value_or(0);
            value.payload_pvt_geodetic.nr_bases = LittleEndian::read<uint8_t>(bytes, 85).value_or(0);
            value.payload_pvt_geodetic.ppp_info = LittleEndian::read<uint16_t>(bytes, 86).value_or(0);
            value.payload_pvt_geodetic.latency = LittleEndian::read<uint16_t>(bytes, 88).value_or(0);
            value.payload_pvt_geodetic.h_accuracy = LittleEndian::read<uint16_t>(bytes, 90).value_or(0);
            value.payload_pvt_geodetic.v_accuracy = LittleEndian::read<uint16_t>(bytes, 92).value_or(0);
            break;
        default:
            break;
    }
    return value;
}
}  // namespace

int GPSNativeSBF::parseChar(const uint8_t b)
{
    int ret = 0;

    // A native frame owns its payload, including any embedded RTCM preambles.
    if (_rtcm_parsing && _decode_state == SBF_DECODE_SYNC1 && _rtcm_parsing->ownsByte(b)) {
        _rtcm_parsing->addByte(b);
        drainRTCM(*_rtcm_parsing);
        return 0;
    }

    switch (_decode_state) {
        // Expecting Sync1
        case SBF_DECODE_SYNC1:
            if (b == SBF_SYNC1) {  // Sync1 found --> expecting Sync2
                payloadRxAdd(b);   // add a payload byte
                _decode_state = SBF_DECODE_SYNC2;
            }

            break;

        // Expecting Sync2
        case SBF_DECODE_SYNC2:
            if (b == SBF_SYNC2) {  // Sync2 found --> expecting CRC
                payloadRxAdd(b);   // add a payload byte
                _decode_state = SBF_DECODE_PAYLOAD;

            } else {  // Sync1 not followed by Sync2: reset parser
                decodeInit();
            }

            break;

        // Expecting payload
        case SBF_DECODE_PAYLOAD:
            ret = payloadRxAdd(b);  // add a payload byte

            if (ret < 0) {
                // payload not handled, discard message
                ret = 0;
                decodeInit();

            } else if (ret > 0) {
                ret = payloadRxDone();  // finish payload processing

                if (_rtcm_parsing) {
                    _rtcm_parsing->reset();
                }

                decodeInit();

            } else {
                // expecting more payload, stay in state SBF_DECODE_PAYLOAD
                ret = 0;
            }

            break;

        default:
            break;
    }

    return ret;
}

int GPSNativeSBF::payloadRxAdd(const uint8_t b)
{
    int ret = 0;
    _wire[_rx_payload_index++] = b;
    const auto length = LittleEndian::read<uint16_t>(_wire, 6).value_or(0);

    if ((_rx_payload_index > 7 && _rx_payload_index >= length) || _rx_payload_index >= _wire.size()) {
        ret = 1;  // payload received completely
    }

    return ret;
}

uint16_t crc16(const uint8_t* data_p, uint32_t length)
{
    uint8_t x;
    uint16_t crc = 0;

    while (length--) {
        x = crc >> 8 ^ *data_p++;
        x ^= x >> 4;
        crc = static_cast<uint16_t>((crc << 8) ^ (x << 12) ^ (x << 5) ^ x);
    }

    return crc;
}

int GPSNativeSBF::payloadRxDone()
{
    _buf = decodeBlock(std::span<const uint8_t>(_wire).first(_rx_payload_index));
    if (_buf.length < 14 || _buf.length > _rx_payload_index || _buf.crc16 != crc16(_wire.data() + 4, _buf.length - 4)) {
        return 0;
    }

    // Base stations output only PVTGeodetic, which carries the survey-in status.
    if (_buf.msg_id != SBF_ID_PVTGeodetic || _buf.length < PVT_GEODETIC_LENGTH || _buf.TOW >= WEEK_MS ||
        _buf.WNc == UINT16_MAX) {
        return 0;
    }

    auto* epoch = navigationEpoch(uint64_t(_buf.WNc) * WEEK_MS + _buf.TOW);
    if (!epoch) {
        return GPSDecodedBatch::PROTOCOL_ACTIVITY;
    }
    epoch->hasPosition = true;
    const auto& pvt = _buf.payload_pvt_geodetic;

    // PVTGeodetic Datum 0 is WGS84/ITRS. Datum 19 is the correction provider's unspecified datum.
    if (pvt.datum != 0) {
        epoch->position = {};
        epoch->position.navigation.fixType = GPSPositionReport::FixType::NoFix;
        if (_configured) {
            log(GPSProtocolLogLevel::Warning, "Unsupported Septentrio position datum: %u", unsigned(pvt.datum));
            controlFailed();
            _ioErrorDetail = QStringLiteral("Septentrio position datum is not WGS84/ITRS");
            _configured = false;
            _rtcm_parsing.reset();
            publishSurvey(false, false, {});
        }
        return GPSDecodedBatch::PROTOCOL_ACTIVITY;
    }

    const bool coordinatesValid = applyPvtGeodetic(pvt, epoch->position);
    // In RTCM mode, PVTGeodetic is used to get base station survey-in
    if (_configured) {
        publishSurveyStatus(pvt, epoch->position, coordinatesValid);
    }
    return GPSDecodedBatch::PROTOCOL_ACTIVITY;
}

bool GPSNativeSBF::applyPvtGeodetic(const sbf_payload_pvt_geodetic_t& pvt, GPSNativePositionReport& position)
{
    position.navigation.fixType = fixType(pvt.mode_type);
    if (pvt.error != 0) {
        position.navigation.fixType = GPSPositionReport::FixType::NoFix;
    } else if (pvt.mode_2d && position.navigation.fixType >= GPSPositionReport::FixType::Fix3D) {
        position.navigation.fixType = GPSPositionReport::FixType::Fix2D;
    }

    // Any value beyond the specified maximum is invalid, not only the do-not-use value (-2*10^10).
    position.velocityValid = position.navigation.fixType > GPSPositionReport::FixType::NoFix && pvt.error == 0 &&
                             !(fabsf(pvt.vn) > 600.0f || fabsf(pvt.ve) > 600.0f || fabsf(pvt.vu) > 600.0f);

    const bool coordinatesValid = std::isfinite(pvt.latitude) && std::abs(pvt.latitude) <= std::numbers::pi / 2 &&
                                  std::isfinite(pvt.longitude) && std::abs(pvt.longitude) <= std::numbers::pi &&
                                  std::isfinite(pvt.height) && std::abs(pvt.height) <= DNU;
    if (!coordinatesValid || !std::isfinite(pvt.undulation) || std::abs(pvt.undulation) > DNU) {
        position.navigation.fixType = GPSPositionReport::FixType::NoFix;
    }

    const bool satellitesKnown = pvt.nr_sv < 255;  // 255 = do not use value
    position.navigation.satellitesUsed = satellitesKnown ? pvt.nr_sv : UINT8_MAX;
    if (_satellites) {
        publishSatelliteUsage(satellitesKnown ? std::optional<int>(pvt.nr_sv) : std::nullopt);
    }

    position.navigation.latitudeDegrees = pvt.latitude * GPS_RAD_TO_DEG;
    position.navigation.longitudeDegrees = pvt.longitude * GPS_RAD_TO_DEG;
    position.navigation.altitudeEllipsoidMeters = pvt.height;
    position.navigation.altitudeMslMeters = pvt.height - static_cast<double>(pvt.undulation);

    // Accuracy is reported as 2DRMS in cm; halve it for the RMS convention used by the other drivers.
    position.navigation.horizontalAccuracyMeters =
        pvt.h_accuracy != UINT16_MAX ? static_cast<float>(pvt.h_accuracy) / 200.0f : NAN;
    position.navigation.verticalAccuracyMeters =
        pvt.v_accuracy != UINT16_MAX ? static_cast<float>(pvt.v_accuracy) / 200.0f : NAN;

    position.navigation.speedMetersPerSecond = sqrtf(pvt.vn * pvt.vn + pvt.ve * pvt.ve);
    position.navigation.courseRadians =
        std::isfinite(pvt.cog) && pvt.cog >= 0.0f && pvt.cog <= 360.0f ? pvt.cog * GPS_DEG_TO_RAD : NAN;

    // WNc/TOW is GNSS system time, not UTC. Without receiver UTC/leap information,
    // retain the epoch key internally and let the facade use reception UTC.
    position.navigation.utcTimeUs = 0;
    position.navigation.timestampUs = nowUs();
    return coordinatesValid;
}

void GPSNativeSBF::publishSurveyStatus(const sbf_payload_pvt_geodetic_t& pvt, const GPSNativePositionReport& position,
                                       bool coordinatesValid)
{
    // Mode bit 6 means automatic base determination is still in progress, not completed.
    // Septentrio PolaRx5TR 5.5.0 Reference Guide, SBF Mode definition (p. 382).
    const bool active = !std::holds_alternative<GPSBaseStationConfig::Fixed>(_baseConfig.mode) && pvt.mode_base_fixed;
    const bool valid = !pvt.mode_base_fixed && pvt.mode_type == 3 && !pvt.mode_2d && !pvt.error && coordinatesValid;
    // The final update on the active-to-inactive transition freezes the survey duration.
    if (active || _survey_active) {
        (void) _surveyClock.update(nowUs());
    }
    _survey_active = active;
    // PVT accuracy describes the navigation solution, not the averaged base survey.
    GPSEllipsoidPosition surveyPosition;
    if (coordinatesValid && !pvt.error) {
        surveyPosition = {.latitudeDegrees = position.navigation.latitudeDegrees,
                          .longitudeDegrees = position.navigation.longitudeDegrees,
                          .altitudeMeters = static_cast<float>(position.navigation.altitudeEllipsoidMeters)};
    }
    publishSurvey(active, valid, _surveyClock.duration(), surveyPosition);
}

GPSNativeSBF::NavigationEpoch* GPSNativeSBF::navigationEpoch(uint64_t receiverTimeMs)
{
    flushDecoded();
    if (_lastPublishedEpoch && receiverTimeMs <= *_lastPublishedEpoch) {
        return nullptr;
    }
    for (auto& epoch : _epochs) {
        if (epoch && epoch->receiverTimeMs == receiverTimeMs) {
            return &*epoch;
        }
    }
    auto slot = _epochs.begin();
    while (slot != _epochs.end() && *slot) {
        ++slot;
    }
    if (slot == _epochs.end()) {
        slot = _epochs[0]->receiverTimeMs < _epochs[1]->receiverTimeMs ? _epochs.begin() : _epochs.begin() + 1;
        if (receiverTimeMs <= (*slot)->receiverTimeMs) {
            return nullptr;
        }
        finishEpoch(*slot);
    }
    *slot = NavigationEpoch{.receiverTimeMs = receiverTimeMs, .receiptUs = nowUs(), .position = {}};
    return &**slot;
}

void GPSNativeSBF::finishEpoch(std::optional<NavigationEpoch>& epoch)
{
    if (epoch->hasPosition && (!_lastPublishedEpoch || epoch->receiverTimeMs > *_lastPublishedEpoch)) {
        epoch->position.navigation.timestampUs = epoch->receiptUs;
        publishPosition(epoch->position);
    }
    if (!_lastPublishedEpoch || epoch->receiverTimeMs > *_lastPublishedEpoch) {
        _lastPublishedEpoch = epoch->receiverTimeMs;
    }
    epoch.reset();
}

void GPSNativeSBF::flushDecoded()
{
    if (_rtcm_parsing) {
        drainRTCM(*_rtcm_parsing);
    }
    if (_epochs[0] && _epochs[1] && _epochs[0]->receiverTimeMs > _epochs[1]->receiverTimeMs) {
        std::swap(_epochs[0], _epochs[1]);
    }
    const auto now = nowUs();
    for (auto& epoch : _epochs) {
        if (epoch && now >= epoch->receiptUs && now - epoch->receiptUs >= EPOCH_MAX_AGE_US) {
            finishEpoch(epoch);
        }
    }
}

void GPSNativeSBF::decodeInit()
{
    _decode_state = SBF_DECODE_SYNC1;
    _rx_payload_index = 0;
}

int GPSNativeSBF::decodeByte(uint8_t byte)
{
    return parseChar(byte);
}

GPSNativeSBF::GPSNativeSBF(GPSProtocolIO io, bool satelliteInfoEnabled)
    : GPSProtocol(std::move(io), satelliteInfoEnabled)
{
    decodeInit();
}

int GPSNativeSBF::receive(unsigned timeout)
{
    return _configured && !hasIOError() ? receiveDecoded(timeout) : 0;
}
