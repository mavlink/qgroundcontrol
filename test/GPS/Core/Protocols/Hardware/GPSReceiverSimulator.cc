#include "GPSReceiverSimulator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <utility>

#include <QtCore/QFile>
#include <QtCore/QTimeZone>
#include <QtNetwork/QTcpSocket>

#include "GPSProtocolMath.h"
#include "LittleEndian.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "Transport/Support/ReplayGPSTransport.h"

using namespace std::chrono_literals;

namespace GPSTest {

namespace {

constexpr uint32_t TIME_MODE_SURVEY_IN = 1;
constexpr uint32_t TIME_MODE_FIXED = 2;
constexpr uint8_t FIX_3D = 3;
/// What a receiver in time mode reports once it holds its position.
constexpr uint8_t FIX_TIME_ONLY = 5;

/// How often a capture is asked for the bytes that are due, and the most it sends at once.
constexpr std::chrono::milliseconds CAPTURE_POLL{10};
constexpr qsizetype CAPTURE_BURST_BYTES = 64 * 1024;

/// GPS time has been this far ahead of UTC since 2017, and BeiDou time is this far behind GPS time.
constexpr qint64 GPS_LEAP_MS = 18'000;
constexpr qint64 BEIDOU_BEHIND_GPS_MS = 14'000;
constexpr qint64 WEEK_MS = qint64{7} * 24 * 60 * 60 * 1000;
constexpr int GLONASS_AHEAD_OF_UTC_SECONDS = 3 * 60 * 60;

constexpr double GEOID_SEPARATION_M = 47.3;
constexpr double HORIZONTAL_ACCURACY_M = 1.5;
constexpr double VERTICAL_ACCURACY_M = 2.5;
/// The accuracy a survey ends with when the host asked for none.
constexpr double SURVEY_ACCURACY_M = 2.0;
/// Metres in the 0.1 mm of NAV-SVIN, the survey accuracy limit and RTCM 1005.
constexpr double TENTH_MILLIMETERS = 1e4;
/// The units of a fixed position: degrees in 1e-7 and the height in cm, each with a signed high-precision part.
constexpr double DEGREES = 1e-7;
constexpr double DEGREES_HP = 1e-9;
constexpr double HEIGHT_M = 1e-2;
constexpr double HEIGHT_HP_M = 1e-4;
/// The payloads of the settings noted here: the model answers any other length of them with ACK-NAK.
constexpr size_t CFG_RATE_BYTES = 6;
constexpr size_t CFG_TMODE3_BYTES = 40;
/// The CFG-TMODE3 flag of a position in latitude, longitude and height rather than ECEF.
constexpr uint16_t CFG_TMODE3_LLA = 0x0100;

struct Satellite
{
    uint8_t gnssId;  ///< 0 GPS, 2 Galileo, 3 BeiDou, 6 GLONASS
    uint8_t svId;
    uint8_t signalDbHz;
    int8_t elevationDegrees;
    int16_t azimuthDegrees;
    bool used;
};

constexpr Satellite SKY[] = {
    {0, 2, 44, 62, 40, true},   {0, 5, 41, 35, 110, true},  {0, 12, 46, 78, 200, true}, {0, 13, 38, 22, 310, true},
    {0, 15, 42, 48, 255, true}, {0, 25, 30, 9, 75, false},  {2, 4, 43, 55, 150, true},  {2, 9, 40, 30, 290, true},
    {2, 11, 45, 70, 20, true},  {2, 24, 28, 7, 230, false}, {3, 19, 41, 44, 95, true},  {3, 22, 39, 27, 180, true},
    {3, 36, 43, 66, 330, true}, {6, 7, 40, 50, 60, true},   {6, 8, 36, 18, 130, true},  {6, 23, 42, 71, 280, true},
    {6, 24, 27, 6, 345, false},
};

/// The svid of NAV-SVINFO, which numbers the satellites of every constellation in one range: GPS 1 to 32, BeiDou 159
/// to 163 and then 33 to 64, GLONASS 65 to 96 and Galileo 211 to 246.
constexpr uint8_t legacySvid(const Satellite& satellite)
{
    switch (satellite.gnssId) {
        case 2:
            return static_cast<uint8_t>(210 + satellite.svId);
        case 3:
            return static_cast<uint8_t>(satellite.svId <= 5 ? 158 + satellite.svId : 27 + satellite.svId);
        case 6:
            return static_cast<uint8_t>(64 + satellite.svId);
        default:
            return satellite.svId;
    }
}

/// A UBX payload written at the offsets of the u-blox interface description, not through the driver's layouts.
class Payload
{
public:
    explicit Payload(qsizetype size)
        : _bytes(size, '\0')
    {}

    template <typename T>
    Payload& set(qsizetype offset, T value)
    {
        (void) LittleEndian::write<T>(mutableBytesOf(_bytes), static_cast<size_t>(offset), value);
        return *this;
    }

    [[nodiscard]] QByteArray frame(UBX::MessageId message) const { return ubxBytes(message.value(), _bytes); }

private:
    QByteArray _bytes;
};

/// The fields of an RTCM3 message, most significant bit first.
class Fields
{
public:
    Fields& put(uint64_t value, int width)
    {
        for (int bit = width - 1; bit >= 0; --bit, ++_size) {
            if (_size % 8 == 0) {
                _bytes.append('\0');
            }
            _bytes.back() = static_cast<char>(_bytes.back() | (((value >> bit) & 1) << (7 - _size % 8)));
        }
        return *this;
    }

    [[nodiscard]] QByteArray frame() const { return rtcmBytes(_bytes); }

private:
    QByteArray _bytes;
    int _size = 0;
};

template <typename T>
T field(const GPSFrame& frame, size_t offset)
{
    return LittleEndian::read<T>(frame.payload, offset).value_or(0);
}

const QDateTime& gpsEpoch()
{
    static const QDateTime epoch(QDate(1980, 1, 6), QTime(0, 0), QTimeZone::UTC);
    return epoch;
}

}  // namespace

GPSReceiverSimulator::GPSReceiverSimulator(Options options)
    : _options(std::move(options))
    , _model(_options.receiver, _clock)
    , _link(GPSCancelToken{}, &_model)
    , _commands(GPSStreamConfig{.framers = GPSFrameKind::UBX, .enabled = GPSFrameKind::UBX, .shareSBFSync = false})
    , _receiverTime(QDateTime::fromSecsSinceEpoch(QDateTime::currentSecsSinceEpoch(), QTimeZone::UTC))
{
    // The link is read whole, not in the pieces the model gives a driver under test.
    _model.coalesceReplies = true;
    _captureTimer.setInterval(CAPTURE_POLL);
    _epochTimer.setTimerType(Qt::PreciseTimer);
    (void) QObject::connect(&_server, &QTcpServer::newConnection, &_server, [this] { _accept(); });
    (void) QObject::connect(&_captureTimer, &QTimer::timeout, &_captureTimer, [this] { _playCapture(); });
    (void) QObject::connect(&_epochTimer, &QTimer::timeout, &_epochTimer, [this] { _runEpoch(); });
}

GPSReceiverSimulator::~GPSReceiverSimulator()
{
    _server.close();
    _dropClient();
}

bool GPSReceiverSimulator::listen(quint16 port, const QHostAddress& address)
{
    if (!_options.capture.isEmpty()) {
        QFile capture(_options.capture);
        if (!capture.open(QIODevice::ReadOnly)) {
            _error = QStringLiteral("Cannot read %1: %2").arg(_options.capture, capture.errorString());
            return false;
        }
    }
    if (!_server.listen(address, port)) {
        _error = _server.errorString();
        return false;
    }
    if (_options.capture.isEmpty()) {
        _followConfiguration();
    }
    return true;
}

void GPSReceiverSimulator::frame(const GPSFrame& frame)
{
    if (_model.legacy) {
        _noteLegacySetting(frame);
    }
    (void) _link.write(QByteArrayView(frame.bytes), QDeadlineTimer(QDeadlineTimer::Forever));
}

void GPSReceiverSimulator::_noteLegacySetting(const GPSFrame& frame)
{
    if (frame.messageId == UBX::Msg::CFG_RATE.value() && frame.payload.size() == CFG_RATE_BYTES) {
        if (const auto period = field<uint16_t>(frame, 0); period > 0) {
            _legacy.measurementPeriod = std::chrono::milliseconds(period);
        }
    } else if (frame.messageId == UBX::Msg::CFG_TMODE3.value() && frame.payload.size() == CFG_TMODE3_BYTES) {
        _legacy.surveyAccuracyLimit = field<uint32_t>(frame, 28);
        // A position in latitude, longitude and height, with the units of the time mode keys of protocol 27.
        if ((field<uint16_t>(frame, 2) & CFG_TMODE3_LLA) != 0) {
            _legacy.fixedPosition = {
                .latitudeDegrees = field<int32_t>(frame, 4) * DEGREES + field<int8_t>(frame, 16) * DEGREES_HP,
                .longitudeDegrees = field<int32_t>(frame, 8) * DEGREES + field<int8_t>(frame, 17) * DEGREES_HP,
                .altitudeMeters = field<int32_t>(frame, 12) * HEIGHT_M + field<int8_t>(frame, 18) * HEIGHT_HP_M};
        }
    }
}

void GPSReceiverSimulator::_accept()
{
    while (QTcpSocket* const client = _server.nextPendingConnection()) {
        _dropClient();
        _client = client;
        ++_connections;
        (void) QObject::connect(client, &QTcpSocket::readyRead, client, [this] { _receive(); });
        (void) QObject::connect(client, &QTcpSocket::disconnected, client, [this, client] {
            if (_client == client) {
                _client = nullptr;
                _captureTimer.stop();
                _replay.reset();
            }
            client->deleteLater();
        });
        if (_options.capture.isEmpty()) {
            // A new session: what the receiver had queued for the one before it is gone.
            (void) _link.open();
            _commands.reset();
        } else {
            _replay =
                std::make_unique<ReplayGPSTransport>(_options.capture, GPSCancelToken{}, _options.captureBaudRate);
            _replay->setRecordedTiming(true);
            if (_replay->open().status != GPSOpenStatus::Opened) {
                _dropClient();
                continue;
            }
            _captureTimer.start();
            _playCapture();
        }
        // What the client sent before its signals were connected.
        _receive();
    }
}

void GPSReceiverSimulator::_dropClient()
{
    _captureTimer.stop();
    _replay.reset();
    if (QTcpSocket* const client = std::exchange(_client, nullptr)) {
        // Its last signals must not reach this object, which may be going away.
        client->disconnect();
        delete client;
    }
}

void GPSReceiverSimulator::_receive()
{
    if (!_client) {
        return;
    }
    const QByteArray bytes = _client->readAll();
    _bytesReceived += bytes.size();
    if (!_options.capture.isEmpty()) {
        return;
    }
    for (const char byte : bytes) {
        _commands.push(static_cast<uint8_t>(byte), *this);
    }
    _followConfiguration();
    _sendReplies();
}

void GPSReceiverSimulator::_send(const QByteArray& bytes)
{
    if (_client && !bytes.isEmpty()) {
        _bytesSent += (std::max) (_client->write(bytes), qint64{0});
    }
}

void GPSReceiverSimulator::_sendReplies()
{
    std::array<uint8_t, 1024> buffer{};
    QByteArray replies;
    for (;;) {
        const GPSReadResult read = _link.read(buffer, 0ms);
        if (read.status != GPSReadStatus::Data) {
            break;
        }
        replies.append(reinterpret_cast<const char*>(buffer.data()), read.bytesRead);
    }
    _send(replies);
}

void GPSReceiverSimulator::_playCapture()
{
    if (!_replay || !_client) {
        return;
    }
    std::array<uint8_t, ReplayGPSTransport::DEFAULT_CHUNK_BYTES> buffer{};
    QByteArray due;
    while (due.size() < CAPTURE_BURST_BYTES) {
        const GPSReadResult read = _replay->read(buffer, 0ms);
        if (read.status != GPSReadStatus::Data) {
            break;
        }
        due.append(reinterpret_cast<const char*>(buffer.data()), read.bytesRead);
    }
    _send(due);
    // The capture ended, or failed with a warning: the link closes once the client has what was sent.
    if (_replay->fatalError()) {
        _captureTimer.stop();
        _replay.reset();
        _client->disconnectFromHost();
    }
}

void GPSReceiverSimulator::_followConfiguration()
{
    if (_model.modes.size() != _timeModeCommands) {
        _timeModeCommands = _model.modes.size();
        _surveyed = 0ms;
    }
    const std::chrono::milliseconds interval(
        (std::max) (_measurementPeriod().count() * _options.secondLength.count() / 1000,
                    std::chrono::milliseconds::rep{1}));
    if (!_epochTimer.isActive() || _epochTimer.intervalAsDuration() != interval) {
        _epochTimer.start(interval);
    }
}

void GPSReceiverSimulator::_runEpoch()
{
    const std::chrono::milliseconds period = _measurementPeriod();
    _receiverTime = _receiverTime.addMSecs(period.count());
    _clock.advanceBy(period);
    ++_navigationEpoch;
    if (_model.timeMode == TIME_MODE_SURVEY_IN && !_surveyComplete()) {
        _surveyed += period;
    }
    if (!_client) {
        return;
    }
    // In the order of a u-blox base: the observations, the navigation epoch, then the station position.
    QByteArray output = _corrections();
    if (_due(UBX::Cfg::MSGOUT_UBX_NAV_PVT, UBX::Msg::NAV_PVT)) {
        output += _navPvt();
    }
    if (_due(UBX::Cfg::MSGOUT_UBX_NAV_DOP, UBX::Msg::NAV_DOP)) {
        output += _navDop();
    }
    output += _satellites();
    if (_due(UBX::Cfg::MSGOUT_UBX_NAV_SVIN, UBX::Msg::NAV_SVIN)) {
        output += _navSvin();
    }
    if (_due(UBX::Cfg::MSGOUT_UBX_NAV_EOE, UBX::Msg::NAV_EOE)) {
        output += Payload(4).set<uint32_t>(0, _timeOfWeekMs()).frame(UBX::Msg::NAV_EOE);
    }
    if (_due(UBX::Cfg::MSGOUT_UBX_MON_RF, UBX::Msg::MON_RF)) {
        output += _monRf();
    }
    if (_due(UBX::Cfg::MSGOUT_RTCM_3X_TYPE1005, UBX::Msg::RTCM3_1005)) {
        output += _stationPosition();
    }
    _send(output);
}

unsigned GPSReceiverSimulator::_outputRate(UBX::MsgOutKey key, UBX::MessageId message) const
{
    // A CFG-MSG rate before protocol 27, a CFG-MSGOUT key from it. The driver sets UART1 and USB alike.
    if (_model.legacy) {
        const auto rate = _model.messageRates.find(message.value());
        return rate == _model.messageRates.end() ? 0 : rate->second;
    }
    return _setting(key.port(UBX::MsgOutPort::UART1).id);
}

bool GPSReceiverSimulator::_due(UBX::MsgOutKey key, UBX::MessageId message) const
{
    const unsigned rate = _outputRate(key, message);
    return rate != 0 && _navigationEpoch % rate == 0;
}

std::chrono::milliseconds GPSReceiverSimulator::_measurementPeriod() const
{
    if (_model.legacy) {
        return _legacy.measurementPeriod;
    }
    const uint32_t period = _setting(UBX::Cfg::RATE_MEAS.id);
    return period == 0 ? 1000ms : std::chrono::milliseconds(period);
}

bool GPSReceiverSimulator::_surveyComplete() const
{
    return _surveyed >= std::chrono::seconds((std::max) (_model.surveyDuration, 1U));
}

GPSEllipsoidPosition GPSReceiverSimulator::_position() const
{
    if (_model.timeMode != TIME_MODE_FIXED) {
        return ANTENNA;
    }
    if (_model.legacy) {
        return _legacy.fixedPosition;
    }
    const auto coordinate = [this](uint32_t key, uint32_t highPrecisionKey, double unit, double highPrecisionUnit) {
        return static_cast<int32_t>(_setting(key)) * unit +
               static_cast<int8_t>(_setting(highPrecisionKey)) * highPrecisionUnit;
    };
    return {
        .latitudeDegrees = coordinate(UBX::Cfg::TMODE_LAT.id, UBX::Cfg::TMODE_LAT_HP.id, DEGREES, DEGREES_HP),
        .longitudeDegrees = coordinate(UBX::Cfg::TMODE_LON.id, UBX::Cfg::TMODE_LON_HP.id, DEGREES, DEGREES_HP),
        .altitudeMeters = coordinate(UBX::Cfg::TMODE_HEIGHT.id, UBX::Cfg::TMODE_HEIGHT_HP.id, HEIGHT_M, HEIGHT_HP_M)};
}

uint32_t GPSReceiverSimulator::_setting(uint32_t key) const
{
    const auto value = _model.currentSettings.find(key);
    return value == _model.currentSettings.end() ? 0 : value->second;
}

uint32_t GPSReceiverSimulator::_timeOfWeekMs() const
{
    return static_cast<uint32_t>((gpsEpoch().msecsTo(_receiverTime) + GPS_LEAP_MS) % WEEK_MS);
}

QByteArray GPSReceiverSimulator::_navPvt() const
{
    const GPSEllipsoidPosition position = _position();
    const QDate date = _receiverTime.date();
    const QTime time = _receiverTime.time();
    const bool positionHeld =
        _model.timeMode == TIME_MODE_FIXED || (_model.timeMode == TIME_MODE_SURVEY_IN && _surveyComplete());
    const auto used = std::ranges::count_if(SKY, &Satellite::used);
    return Payload(92)
        .set<uint32_t>(0, _timeOfWeekMs())
        .set<uint16_t>(4, static_cast<uint16_t>(date.year()))
        .set<uint8_t>(6, static_cast<uint8_t>(date.month()))
        .set<uint8_t>(7, static_cast<uint8_t>(date.day()))
        .set<uint8_t>(8, static_cast<uint8_t>(time.hour()))
        .set<uint8_t>(9, static_cast<uint8_t>(time.minute()))
        .set<uint8_t>(10, static_cast<uint8_t>(time.second()))
        .set<uint8_t>(11, 0x07)  // validDate, validTime and fullyResolved
        .set<int32_t>(16, time.msec() * 1'000'000)
        .set<uint8_t>(20, positionHeld ? FIX_TIME_ONLY : FIX_3D)
        .set<uint8_t>(21, 0x01)  // gnssFixOK
        .set<uint8_t>(23, static_cast<uint8_t>(used))
        .set<int32_t>(24, static_cast<int32_t>(std::lround(position.longitudeDegrees * 1e7)))
        .set<int32_t>(28, static_cast<int32_t>(std::lround(position.latitudeDegrees * 1e7)))
        .set<int32_t>(32, static_cast<int32_t>(std::lround(position.altitudeMeters * 1e3)))
        .set<int32_t>(36, static_cast<int32_t>(std::lround((position.altitudeMeters - GEOID_SEPARATION_M) * 1e3)))
        .set<uint32_t>(40, static_cast<uint32_t>(HORIZONTAL_ACCURACY_M * 1e3))
        .set<uint32_t>(44, static_cast<uint32_t>(VERTICAL_ACCURACY_M * 1e3))
        .set<uint16_t>(76, 150)  // pDOP
        .frame(UBX::Msg::NAV_PVT);
}

QByteArray GPSReceiverSimulator::_navDop() const
{
    // gDOP, pDOP, tDOP, vDOP, hDOP, nDOP and eDOP in 0.01.
    Payload payload(18);
    payload.set<uint32_t>(0, _timeOfWeekMs());
    qsizetype offset = 4;
    for (const uint16_t dop : {170, 150, 80, 120, 90, 60, 67}) {
        payload.set<uint16_t>(offset, dop);
        offset += 2;
    }
    return payload.frame(UBX::Msg::NAV_DOP);
}

QByteArray GPSReceiverSimulator::_satellites() const
{
    constexpr qsizetype HEADER_BYTES = 8;
    constexpr qsizetype SATELLITE_BYTES = 12;
    // NAV-SVINFO before protocol 27, which has no CFG-MSGOUT key.
    const UBX::MessageId message = _model.legacy ? UBX::Msg::NAV_SVINFO : UBX::Msg::NAV_SAT;
    if (!_due(UBX::Cfg::MSGOUT_UBX_NAV_SAT, message)) {
        return {};
    }
    const auto count = static_cast<uint8_t>(std::size(SKY));
    Payload payload(HEADER_BYTES + SATELLITE_BYTES * count);
    payload.set<uint32_t>(0, _timeOfWeekMs());
    if (_model.legacy) {
        payload.set<uint8_t>(4, count);
    } else {
        payload.set<uint8_t>(4, 1).set<uint8_t>(5, count);
    }
    qsizetype offset = HEADER_BYTES;
    uint8_t channel = 0;
    for (const Satellite& satellite : SKY) {
        if (_model.legacy) {
            payload.set<uint8_t>(offset, channel++)
                .set<uint8_t>(offset + 1, legacySvid(satellite))
                .set<uint8_t>(offset + 2, satellite.used ? 0x01 : 0)  // svUsed
                .set<uint8_t>(offset + 4, satellite.signalDbHz)
                .set<int8_t>(offset + 5, satellite.elevationDegrees)
                .set<int16_t>(offset + 6, satellite.azimuthDegrees);
        } else {
            payload.set<uint8_t>(offset, satellite.gnssId)
                .set<uint8_t>(offset + 1, satellite.svId)
                .set<uint8_t>(offset + 2, satellite.signalDbHz)
                .set<int8_t>(offset + 3, satellite.elevationDegrees)
                .set<int16_t>(offset + 4, satellite.azimuthDegrees)
                .set<uint32_t>(offset + 8, satellite.used ? 0x08 : 0);  // svUsed
        }
        offset += SATELLITE_BYTES;
    }
    return payload.frame(message);
}

QByteArray GPSReceiverSimulator::_navSvin() const
{
    const bool surveying = _model.timeMode == TIME_MODE_SURVEY_IN;
    const bool complete = surveying && _surveyComplete();
    Payload payload(40);
    payload.set<uint32_t>(4, _timeOfWeekMs()).set<uint8_t>(36, complete).set<uint8_t>(37, surveying && !complete);
    if (!surveying) {
        return payload.frame(UBX::Msg::NAV_SVIN);
    }
    const double requestedSeconds = (std::max) (_model.surveyDuration, 1U);
    const double seconds = std::chrono::duration<double>(_surveyed).count();
    const uint32_t limit = _model.legacy ? _legacy.surveyAccuracyLimit : _setting(UBX::Cfg::TMODE_SVIN_ACC_LIMIT.id);
    const double requestedAccuracy = limit == 0 ? SURVEY_ACCURACY_M : limit / TENTH_MILLIMETERS;
    // The mean improves with the square root of the observation time, to the requested accuracy as the survey ends.
    const double accuracy = requestedAccuracy * std::sqrt(requestedSeconds / (std::max) (seconds, 1.0));
    const GPSProtocolMath::Ecef mean = GPSProtocolMath::toEcef(ANTENNA);
    qsizetype axis = 0;
    for (const double meters : {mean.x, mean.y, mean.z}) {
        // Centimetres, and what is left of them in 0.1 mm.
        const long long tenthMillimeters = std::llround(meters * TENTH_MILLIMETERS);
        payload.set<int32_t>(12 + 4 * axis, static_cast<int32_t>(tenthMillimeters / 100))
            .set<int8_t>(24 + axis, static_cast<int8_t>(tenthMillimeters % 100));
        ++axis;
    }
    return payload.set<uint32_t>(8, static_cast<uint32_t>(seconds))
        .set<uint32_t>(28, static_cast<uint32_t>(std::llround(accuracy * TENTH_MILLIMETERS)))
        .set<uint32_t>(32, static_cast<uint32_t>(seconds))
        .frame(UBX::Msg::NAV_SVIN);
}

QByteArray GPSReceiverSimulator::_monRf() const
{
    // One RF block whose jamming monitor and antenna supervisor report OK, with the antenna powered.
    return Payload(28).set<uint8_t>(1, 1).set<uint8_t>(5, 1).set<uint8_t>(6, 2).set<uint8_t>(7, 1).frame(
        UBX::Msg::MON_RF);
}

QByteArray GPSReceiverSimulator::_corrections() const
{
    struct Observations
    {
        UBX::MsgOutKey key;
        UBX::MessageId message;
        uint16_t number;
    };

    static constexpr Observations MESSAGES[] = {
        {UBX::Cfg::MSGOUT_RTCM_3X_TYPE1074, UBX::Msg::RTCM3_1074, 1074},
        {UBX::Cfg::MSGOUT_RTCM_3X_TYPE1077, UBX::Msg::RTCM3_1077, 1077},
        {UBX::Cfg::MSGOUT_RTCM_3X_TYPE1084, UBX::Msg::RTCM3_1084, 1084},
        {UBX::Cfg::MSGOUT_RTCM_3X_TYPE1087, UBX::Msg::RTCM3_1087, 1087},
        {UBX::Cfg::MSGOUT_RTCM_3X_TYPE1094, UBX::Msg::RTCM3_1094, 1094},
        {UBX::Cfg::MSGOUT_RTCM_3X_TYPE1097, UBX::Msg::RTCM3_1097, 1097},
        {UBX::Cfg::MSGOUT_RTCM_3X_TYPE1124, UBX::Msg::RTCM3_1124, 1124},
        {UBX::Cfg::MSGOUT_RTCM_3X_TYPE1127, UBX::Msg::RTCM3_1127, 1127},
    };
    const qint64 gpsTimeOfWeekMs = _timeOfWeekMs();
    const QDateTime glonassTime = _receiverTime.addSecs(GLONASS_AHEAD_OF_UTC_SECONDS);
    std::array<const Observations*, std::size(MESSAGES)> due{};
    size_t count = 0;
    for (const Observations& observations : MESSAGES) {
        if (_due(observations.key, observations.message)) {
            due[count++] = &observations;
        }
    }
    QByteArray output;
    for (size_t index = 0; index < count; ++index) {
        // The header of a multiple signal message whose satellite and signal masks are empty: no cell follows.
        Fields fields;
        fields.put(due[index]->number, 12).put(0, 12);
        switch (due[index]->number / 10) {
            case 108:
                // The day of the week, from Sunday, and the time of day in Moscow.
                fields.put(static_cast<uint64_t>(glonassTime.date().dayOfWeek() % 7), 3)
                    .put(static_cast<uint64_t>(glonassTime.time().msecsSinceStartOfDay()), 27);
                break;
            case 112:
                fields.put(static_cast<uint64_t>((gpsTimeOfWeekMs - BEIDOU_BEHIND_GPS_MS + WEEK_MS) % WEEK_MS), 30);
                break;
            default:
                fields.put(static_cast<uint64_t>(gpsTimeOfWeekMs), 30);
                break;
        }
        // More messages of the epoch follow; IODS, reserved, clock steering, external clock and smoothing; the masks.
        fields.put(index + 1 < count, 1).put(0, 3 + 7 + 2 + 2 + 1 + 3).put(0, 64).put(0, 32);
        output += fields.frame();
    }
    if (_due(UBX::Cfg::MSGOUT_RTCM_3X_TYPE1230, UBX::Msg::RTCM3_1230)) {
        // GLONASS code-phase biases: none, with an empty signal mask.
        output += Fields().put(1230, 12).put(0, 12).put(0, 1 + 3 + 4).frame();
    }
    return output;
}

QByteArray GPSReceiverSimulator::_stationPosition() const
{
    const GPSProtocolMath::Ecef position = GPSProtocolMath::toEcef(_position());
    const auto coordinate = [](double meters) {
        return static_cast<uint64_t>(std::llround(meters * TENTH_MILLIMETERS));
    };
    // Message 1005: station 0, no ITRF year; GPS, GLONASS and Galileo served by a physical station; then the antenna
    // reference point, with the single receiver oscillator indicator after X and the quarter cycle indicator after Y.
    return Fields()
        .put(1005, 12)
        .put(0, 12 + 6)
        .put(0b1110, 4)
        .put(coordinate(position.x), 38)
        .put(0b10, 2)
        .put(coordinate(position.y), 38)
        .put(0, 2)
        .put(coordinate(position.z), 38)
        .frame();
}

}  // namespace GPSTest
