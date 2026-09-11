#include <QtCore/QBuffer>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonParseError>
#include <QtCore/QSet>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include "GPSReceiverProfile.h"
#include "GPSRecordingFormat.h"
#include "GPSRecordingValidator.h"
#include "JsonParsing.h"

namespace {
struct WireEvent
{
    quint64 atUs = 0;
    GPSRecordingEvent::Kind kind = GPSRecordingEvent::Kind::Rx;
    QByteArray bytes;
    int value = 0;
    quint64 startedAtUs = 0;
    quint64 stream = 0;
    bool resumed = false;
    GPSRecordingMetadata metadata;
    std::optional<GPSWriteResult> writeResult;
    bool fatal = false;
    std::optional<qint64> receivedAtUs;
    std::optional<GPSOpenStatus> openStatus;
    std::optional<GPSReadStatus> readStatus;

    GPSRecordingEvent normalized() const;
};

GPSRecordingEvent WireEvent::normalized() const
{
    using E = GPSRecordingEvent;
    E event{.atUs = atUs, .startedAtUs = startedAtUs, .stream = stream};
    switch (kind) {
        case E::Kind::Session:
            event.payload = E::Session{metadata};
            break;
        case E::Kind::Open:
        case E::Kind::OpenError:
            event.payload = E::Open{kind == E::Kind::Open, resumed, openStatus};
            break;
        case E::Kind::Rx:
            event.payload = E::Read{E::Read::Outcome::Data, bytes, value, receivedAtUs, readStatus};
            break;
        case E::Kind::Timeout:
            event.payload = E::Read{E::Read::Outcome::TimedOut, {}, value, {}, readStatus};
            break;
        case E::Kind::ReadError:
            event.payload = E::Read{E::Read::Outcome::Error, bytes, value, {}, readStatus};
            break;
        case E::Kind::Disconnect:
            event.payload = E::Read{E::Read::Outcome::Closed, {}, value, {}, readStatus};
            break;
        case E::Kind::Cancel:
            event.payload = E::Read{E::Read::Outcome::Cancelled, {}, value, {}, readStatus};
            break;
        case E::Kind::Tx:
        case E::Kind::WriteError:
            event.payload = E::Write{bytes, kind == E::Kind::Tx, value, fatal};
            break;
        case E::Kind::Baud:
        case E::Kind::BaudError:
            event.payload = E::Baud{value, kind == E::Kind::Baud};
            break;
        case E::Kind::Close:
            event.payload = E::Close{value};
            break;
        case E::Kind::ConfigurationStarted:
            event.payload = E::Configuration{};
            break;
        case E::Kind::ConfigurationFinished:
            event.payload = E::Configuration{value};
            break;
        case E::Kind::BoundedWrite:
            event.payload = E::BoundedWrite{bytes, writeResult.value_or(GPSWriteResult{}), fatal};
            break;
    }
    return event;
}
}  // namespace

namespace {
using K = GPSRecordingEvent::Kind;
using T = GPSRecordingMetadata::Transport;
using W = GPSWriteStatus;

template <typename TEnum>
struct Name
{
    TEnum value;
    const char* text;
};

constexpr Name<K> kinds[] = {{K::Session, "session"},
                             {K::Open, "open"},
                             {K::OpenError, "open_error"},
                             {K::Rx, "rx"},
                             {K::Tx, "tx"},
                             {K::Baud, "baud"},
                             {K::BaudError, "baud_error"},
                             {K::Timeout, "timeout"},
                             {K::ReadError, "read_error"},
                             {K::WriteError, "write_error"},
                             {K::Disconnect, "disconnect"},
                             {K::Cancel, "cancel"},
                             {K::Close, "close"},
                             {K::ConfigurationStarted, "configuration_started"},
                             {K::ConfigurationFinished, "configuration_finished"},
                             {K::BoundedWrite, "bounded_write"}};
constexpr Name<T> transports[] = {
    {T::Unknown, "unknown"},          {T::Serial, "serial"},   {T::Tcp, "tcp"}, {T::Udp, "udp"},
    {T::UdpListener, "udp_listener"}, {T::UdpPeer, "udp_peer"}};
constexpr Name<W> writes[] = {{W::Completed, "completed"},     {W::TimedOut, "timed_out"},
                              {W::Cancelled, "cancelled"},     {W::Error, "error"},
                              {W::Unsupported, "unsupported"}, {W::InvalidData, "invalid_data"}};
constexpr Name<GPSOpenStatus> opens[] = {{GPSOpenStatus::Opened, "opened"},
                                         {GPSOpenStatus::TimedOut, "timed_out"},
                                         {GPSOpenStatus::Cancelled, "cancelled"},
                                         {GPSOpenStatus::Error, "error"},
                                         {GPSOpenStatus::Unsupported, "unsupported"}};
constexpr Name<GPSReadStatus> reads[] = {{GPSReadStatus::Data, "data"},
                                         {GPSReadStatus::TimedOut, "timed_out"},
                                         {GPSReadStatus::Cancelled, "cancelled"},
                                         {GPSReadStatus::Closed, "closed"},
                                         {GPSReadStatus::Error, "error"},
                                         {GPSReadStatus::Overflow, "overflow"},
                                         {GPSReadStatus::InvalidData, "invalid_data"}};
constexpr Name<GPSReceiverConfig::Role> roles[] = {{GPSReceiverConfig::Role::RTKBase, "rtk_base"},
                                                   {GPSReceiverConfig::Role::Position, "position"}};
constexpr Name<GPSReceiverConfig::OutputProtocol> protocols[] = {{GPSReceiverConfig::OutputProtocol::Native, "native"},
                                                                 {GPSReceiverConfig::OutputProtocol::NMEA, "nmea"}};
constexpr Name<int> drivers[] = {{-1, "none"},
                                 {static_cast<int>(GPSType::u_blox), "ublox"},
                                 {static_cast<int>(GPSType::trimble), "trimble"},
                                 {static_cast<int>(GPSType::septentrio), "septentrio"},
                                 {static_cast<int>(GPSType::femto), "femto"}};
// Frozen v1 ordinals and the legacy stream callback's integer status are translated explicitly.
constexpr Name<int> configurationStatuses[] = {{0, "not_configured"}, {1, "ready"},           {2, "unsupported"},
                                               {3, "cancelled"},      {4, "transport_error"}, {5, "failed"}};

template <typename TEnum, size_t N>
QString name(TEnum value, const Name<TEnum> (&names)[N])
{
    for (const auto& entry : names) {
        if (entry.value == value) {
            return QString::fromLatin1(entry.text);
        }
    }
    return {};
}

template <typename TEnum, size_t N>
bool parseName(const QJsonValue& json, TEnum& value, const Name<TEnum> (&names)[N])
{
    if (!json.isString()) {
        return false;
    }
    for (const auto& entry : names) {
        if (json.toString() == QLatin1String(entry.text)) {
            value = entry.value;
            return true;
        }
    }
    return false;
}

bool integer(const QJsonValue& value, qint64 minimum, qint64 maximum)
{
    const double number = value.toDouble(std::numeric_limits<double>::quiet_NaN());
    return std::isfinite(number) && number >= minimum && number <= maximum && std::floor(number) == number;
}

bool readMetadata(const QJsonObject& object, int version, GPSRecordingMetadata& result, QString& error)
{
    const auto enumeration = version == 1 ? QJsonValue::Double : QJsonValue::String;
    if (!JsonParsing::validateKeysStrict(object,
                                         {{"transport", enumeration, true},
                                          {"protocol", enumeration, true},
                                          {"role", enumeration, true},
                                          {"driver", enumeration, true},
                                          {"baud", QJsonValue::Double, true},
                                          {"fixed_baud", QJsonValue::Double, version >= 2},
                                          {"configured", QJsonValue::Bool, true},
                                          {"producer", QJsonValue::String, false},
                                          {"build", QJsonValue::String, false},
                                          {"configuration_revision", QJsonValue::Double, false},
                                          {"constellation_mask", QJsonValue::Double, true},
                                          {"dynamic_model", QJsonValue::Double, true},
                                          {"output_rate_hz", QJsonValue::Double, true},
                                          {"heading_offset_deg", QJsonValue::Double, true},
                                          {"base", QJsonValue::Object, true}},
                                         error)) {
        return false;
    }
    auto& m = result;
    m.provenance.producer = object["producer"].toString();
    m.provenance.build = object["build"].toString();
    if (object.contains("configuration_revision") && !integer(object["configuration_revision"], 0, UINT32_MAX)) {
        error = QStringLiteral("Invalid driver configuration revision");
        return false;
    }
    m.provenance.configurationRevision = static_cast<quint32>(object["configuration_revision"].toInteger());
    if (!m.provenance.valid()) {
        error = QStringLiteral("Invalid recording provenance");
        return false;
    }
    if (version == 1) {
        if (!integer(object["transport"], 0, 3) || !integer(object["protocol"], 0, 1) ||
            !integer(object["role"], 0, 1) || !integer(object["driver"], -1, 3)) {
            error = QStringLiteral("Invalid version 1 profile enumeration");
            return false;
        }
        constexpr T legacyTransport[] = {T::Unknown, T::Serial, T::Tcp, T::Udp};
        constexpr GPSReceiverConfig::Role legacyRole[] = {GPSReceiverConfig::Role::RTKBase,
                                                          GPSReceiverConfig::Role::Position};
        constexpr GPSReceiverConfig::OutputProtocol legacyProtocol[] = {GPSReceiverConfig::OutputProtocol::Native,
                                                                        GPSReceiverConfig::OutputProtocol::NMEA};
        constexpr int legacyDriver[] = {-1, static_cast<int>(GPSType::u_blox), static_cast<int>(GPSType::trimble),
                                        static_cast<int>(GPSType::septentrio), static_cast<int>(GPSType::femto)};
        m.transport = legacyTransport[object["transport"].toInt()];
        m.receiver.role = legacyRole[object["role"].toInt()];
        m.receiver.outputProtocol = legacyProtocol[object["protocol"].toInt()];
        m.driverType = legacyDriver[object["driver"].toInt() + 1];
        m.fixedBaud = m.transport == T::Tcp || m.transport == T::Udp ? 115200 : 0;
    } else if (!parseName(object["transport"], m.transport, transports) ||
               !parseName(object["protocol"], m.receiver.outputProtocol, protocols) ||
               !parseName(object["role"], m.receiver.role, roles) ||
               !parseName(object["driver"], m.driverType, drivers)) {
        error = QStringLiteral("Unknown profile enumeration");
        return false;
    }
    for (const char* key : {"baud", "fixed_baud", "constellation_mask", "dynamic_model", "output_rate_hz"}) {
        if (object.contains(key) && !integer(object[key], 0, std::numeric_limits<int>::max())) {
            error = QStringLiteral("Invalid profile integer: %1").arg(QLatin1String(key));
            return false;
        }
    }
    m.initialBaud = object["baud"].toInt();
    if (version >= 2) {
        m.fixedBaud = object["fixed_baud"].toInt();
    }
    m.configured = object["configured"].toBool();
    m.receiver.constellationMask = object["constellation_mask"].toInt();
    m.receiver.dynamicModel = object["dynamic_model"].toInt();
    m.receiver.outputRateHz = object["output_rate_hz"].toInt();
    const double heading = object["heading_offset_deg"].toDouble();
    if (!std::isfinite(heading) || std::abs(heading) > std::numeric_limits<float>::max()) {
        error = QStringLiteral("Invalid heading offset");
        return false;
    }
    m.receiver.headingOffsetDeg = static_cast<float>(heading);
    const auto base = object["base"].toObject();
    if (!JsonParsing::validateKeysStrict(base,
                                         {{"fixed", QJsonValue::Bool, true},
                                          {"survey_accuracy_m", QJsonValue::Double, true},
                                          {"survey_duration_s", QJsonValue::Double, true},
                                          {"latitude", QJsonValue::Double, true},
                                          {"longitude", QJsonValue::Double, true},
                                          {"altitude_m", QJsonValue::Double, true},
                                          {"accuracy_m", QJsonValue::Double, true}},
                                         error)) {
        return false;
    }
    if (!integer(base["survey_duration_s"], 0, std::numeric_limits<int>::max())) {
        error = QStringLiteral("Invalid survey duration");
        return false;
    }
    for (const char* key : {"survey_accuracy_m", "latitude", "longitude", "altitude_m", "accuracy_m"}) {
        const double number = base[key].toDouble();
        if (!std::isfinite(number) || std::abs(number) > std::numeric_limits<float>::max()) {
            error = QStringLiteral("Invalid base coordinate or accuracy");
            return false;
        }
    }
    m.receiver.base = {.useFixedBase = base["fixed"].toBool(),
                       .surveyInAccMeters = base["survey_accuracy_m"].toDouble(),
                       .surveyInDurationSecs = base["survey_duration_s"].toInt(),
                       .fixedBaseLatitude = base["latitude"].toDouble(),
                       .fixedBaseLongitude = base["longitude"].toDouble(),
                       .fixedBaseAltitudeMeters = static_cast<float>(base["altitude_m"].toDouble()),
                       .fixedBaseAccuracyMeters = static_cast<float>(base["accuracy_m"].toDouble())};
    return true;
}

QJsonObject metadataJson(const GPSRecordingMetadata& m)
{
    return {{"transport", name(m.transport, transports)},
            {"protocol", name(m.receiver.outputProtocol, protocols)},
            {"role", name(m.receiver.role, roles)},
            {"driver", name(m.driverType, drivers)},
            {"baud", m.initialBaud},
            {"fixed_baud", static_cast<qint64>(m.fixedBaud)},
            {"configured", m.configured},
            {"producer", m.provenance.producer},
            {"build", m.provenance.build},
            {"configuration_revision", static_cast<qint64>(m.provenance.configurationRevision)},
            {"constellation_mask", m.receiver.constellationMask},
            {"dynamic_model", m.receiver.dynamicModel},
            {"output_rate_hz", m.receiver.outputRateHz},
            {"heading_offset_deg", m.receiver.headingOffsetDeg},
            {"base", QJsonObject{{"fixed", m.receiver.base.useFixedBase},
                                 {"survey_accuracy_m", m.receiver.base.surveyInAccMeters},
                                 {"survey_duration_s", m.receiver.base.surveyInDurationSecs},
                                 {"latitude", m.receiver.base.fixedBaseLatitude},
                                 {"longitude", m.receiver.base.fixedBaseLongitude},
                                 {"altitude_m", m.receiver.base.fixedBaseAltitudeMeters},
                                 {"accuracy_m", m.receiver.base.fixedBaseAccuracyMeters}}}};
}
}  // namespace

namespace {
QJsonObject eventJson(const GPSRecordingEvent& event)
{
    QJsonObject item{{"at_us", static_cast<qint64>(event.atUs)},
                     {"stream", static_cast<qint64>(event.stream)},
                     {"kind", name(event.kind(), kinds)}};
    if (!event.bytes().isEmpty()) {
        item.insert("hex", QString::fromLatin1(event.bytes().toHex()));
    }
    if (event.startedAtUs) {
        item.insert("started_us", static_cast<qint64>(event.startedAtUs));
    }
    if (event.receivedAtUs()) {
        item.insert("received_us", *event.receivedAtUs());
    }
    if (event.openStatus()) {
        item.insert("open_status", name(*event.openStatus(), opens));
    }
    if (event.readStatus()) {
        item.insert("read_status", name(*event.readStatus(), reads));
    }
    if (event.resumed()) {
        item.insert("resumed", true);
    }
    if (event.kind() == K::Session) {
        item.insert("profile", metadataJson(event.metadata()));
    } else if (event.kind() == K::ConfigurationFinished) {
        item.insert("status", name(event.value(), configurationStatuses));
    } else if (event.value() || event.kind() == K::WriteError) {
        item.insert("value", event.value());
    }
    if (event.writeResult()) {
        const auto w = *event.writeResult();
        item.insert("write", QJsonObject{{"status", name(w.status, writes)},
                                         {"accepted", w.acceptedBytes},
                                         {"written", w.writtenBytes},
                                         {"uncertain", w.uncertainBytes},
                                         {"fatal", event.fatal()}});
    }
    return item;
}

}  // namespace

bool GPSRecordingDocument::writeTo(QIODevice& device, QString& error,
                                   const std::function<bool(qsizetype)>& progress) const
{
    error.clear();
    if (events.size() > MAX_EVENTS) {
        error = QStringLiteral("Recording has too many events");
        return false;
    }
    qsizetype written = 0;
    const auto write = [&](QByteArrayView bytes) {
        if (bytes.size() > MAX_BYTES - written) {
            error = QStringLiteral("Recording exceeds 4 MiB");
            return false;
        }
        if (device.write(bytes.data(), bytes.size()) != bytes.size()) {
            error = QStringLiteral("Cannot write recording: %1").arg(device.errorString());
            return false;
        }
        written += bytes.size();
        return true;
    };
    const auto proceed = [&](qsizetype count) {
        if (progress && !progress(count)) {
            error = QStringLiteral("Recording export cancelled");
            return false;
        }
        return true;
    };
    if (!proceed(0) || !write("{\"fileType\":\"GPSRecording\",\"version\":" + QByteArray::number(CURRENT_VERSION) +
                              ",\"description\":\"QGC receiver recording\",\"limit_reached\":"))
        return false;
    if (!write(limitReached ? "true,\"events\":[" : "false,\"events\":["))
        return false;
    GPSRecordingValidator validator;
    for (qsizetype i = 0; i < events.size(); ++i) {
        const auto& event = events[i];
        // Bound hex expansion before allocating even a single event's JSON.
        if (event.bytes().size() > (MAX_BYTES - written) / 2) {
            error = QStringLiteral("Recording exceeds 4 MiB");
            return false;
        }
        if (!validator.check(event, error) || (i && !write(",")) ||
            !write(QJsonDocument(eventJson(event)).toJson(QJsonDocument::Compact)) || !proceed(i + 1))
            return false;
    }
    return write("]}");
}

QByteArray GPSRecordingDocument::encode(QString* error) const
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QString failure;
    const bool success = writeTo(buffer, failure);
    if (error)
        *error = failure;
    return success ? buffer.data() : QByteArray{};
}

bool GPSRecordingDocument::decode(const QByteArray& bytes, GPSRecordingDocument& result, QString& error)
{
    if (bytes.size() > MAX_BYTES) {
        error = QStringLiteral("Recording exceeds 4 MiB");
        return false;
    }
    QJsonParseError parseError;
    const auto json = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !json.isObject()) {
        error = QStringLiteral("Invalid recording JSON: %1").arg(parseError.errorString());
        return false;
    }
    const auto object = json.object();
    if (!JsonParsing::validateKeysStrict(object,
                                         {{"version", QJsonValue::Double, true},
                                          {"fileType", QJsonValue::String, false},
                                          {"description", QJsonValue::String, false},
                                          {"limit_reached", QJsonValue::Bool, false},
                                          {"events", QJsonValue::Array, true}},
                                         error)) {
        return false;
    }
    if (!integer(object["version"], 1, CURRENT_VERSION)) {
        error = QStringLiteral("Unsupported recording version");
        return false;
    }
    const int version = object["version"].toInt();
    if ((version >= 2 || object.contains("fileType")) &&
        object["fileType"].toString() != QStringLiteral("GPSRecording")) {
        error = QStringLiteral("Invalid recording file type");
        return false;
    }
    const auto array = object["events"].toArray();
    if (array.size() > MAX_EVENTS) {
        error = QStringLiteral("Recording has too many events");
        return false;
    }
    GPSRecordingDocument parsed;
    parsed.sourceVersion = version;
    parsed.limitReached = object["limit_reached"].toBool();
    GPSRecordingValidator validator;
    for (const auto& value : array) {
        if (!value.isObject()) {
            error = QStringLiteral("Recording event must be an object");
            return false;
        }
        const auto item = value.toObject();
        if (!JsonParsing::validateKeysStrict(item,
                                             {{"at_us", QJsonValue::Double, true},
                                              {"kind", QJsonValue::String, true},
                                              {"stream", QJsonValue::Double, false},
                                              {"started_us", QJsonValue::Double, false},
                                              {"hex", QJsonValue::String, false},
                                              {"value", QJsonValue::Double, false},
                                              {"resumed", QJsonValue::Bool, false},
                                              {"profile", QJsonValue::Object, false},
                                              {"status", QJsonValue::String, false},
                                              {"write", QJsonValue::Object, false},
                                              {"received_us", QJsonValue::Double, false},
                                              {"open_status", QJsonValue::String, false},
                                              {"read_status", QJsonValue::String, false}},
                                             error)) {
            return false;
        }
        WireEvent event;
        if (!integer(item["at_us"], 0, 9000000000000000LL) || !parseName(item["kind"], event.kind, kinds) ||
            (item.contains("stream") && !integer(item["stream"], 0, 9000000000000000LL)) ||
            (item.contains("value") &&
             !integer(item["value"], std::numeric_limits<int>::min(), std::numeric_limits<int>::max()))) {
            error = QStringLiteral("Invalid recording event integer or kind");
            return false;
        }
        event.atUs = item["at_us"].toInteger();
        event.stream = item["stream"].toInteger();
        event.value = item["value"].toInt();
        event.resumed = item["resumed"].toBool();
        if (item.contains("started_us") && !integer(item["started_us"], 0, event.atUs)) {
            error = QStringLiteral("Out-of-order event or invalid operation timing");
            return false;
        }
        event.startedAtUs = item["started_us"].toInteger();
        if (item.contains("received_us")) {
            if (version < 3 || event.kind != K::Rx || !integer(item["received_us"], -9000000000000000LL, event.atUs)) {
                error = QStringLiteral("Invalid producer receipt time");
                return false;
            }
            event.receivedAtUs = item["received_us"].toInteger();
        }
        if (item.contains("open_status")) {
            GPSOpenStatus status;
            if (version < 3 || (event.kind != K::Open && event.kind != K::OpenError) ||
                !parseName(item["open_status"], status, opens)) {
                error = QStringLiteral("Invalid open outcome");
                return false;
            }
            event.openStatus = status;
        }
        if (item.contains("read_status")) {
            GPSReadStatus status;
            if (version < 3 || !parseName(item["read_status"], status, reads) ||
                (event.kind != K::Rx && event.kind != K::Timeout && event.kind != K::Cancel &&
                 event.kind != K::Disconnect && event.kind != K::ReadError)) {
                error = QStringLiteral("Invalid read outcome");
                return false;
            }
            event.readStatus = status;
        }
        const auto hex = item["hex"].toString().toLatin1();
        event.bytes = QByteArray::fromHex(hex);
        if (event.bytes.toHex() != hex.toLower()) {
            error = QStringLiteral("Invalid recording payload hex");
            return false;
        }
        if (event.kind == K::Session) {
            if (!item.contains("profile") ||
                !readMetadata(item["profile"].toObject(), version, event.metadata, error)) {
                if (error.isEmpty()) {
                    error = QStringLiteral("Missing or duplicate stream profile");
                }
                return false;
            }
        } else if (item.contains("profile")) {
            error = QStringLiteral("Profile outside session event");
            return false;
        }
        if (event.kind == K::ConfigurationFinished) {
            if (version >= 2 ? !parseName(item["status"], event.value, configurationStatuses)
                             : !integer(item["value"], 0, 5)) {
                error = QStringLiteral("Unknown configuration status");
                return false;
            }
        } else if (item.contains("status")) {
            error = QStringLiteral("Unexpected configuration status");
            return false;
        }
        if (event.kind == K::BoundedWrite) {
            const auto write = item["write"].toObject();
            GPSWriteResult w;
            if (version < 2 || !item.contains("write") ||
                !JsonParsing::validateKeysStrict(write,
                                                 {{"status", QJsonValue::String, true},
                                                  {"accepted", QJsonValue::Double, true},
                                                  {"written", QJsonValue::Double, true},
                                                  {"uncertain", QJsonValue::Double, true},
                                                  {"fatal", QJsonValue::Bool, true}},
                                                 error) ||
                !parseName(write["status"], w.status, writes) || !integer(write["accepted"], 0, event.bytes.size()) ||
                !integer(write["written"], 0, event.bytes.size()) ||
                !integer(write["uncertain"], 0, event.bytes.size())) {
                if (error.isEmpty()) {
                    error = QStringLiteral("Invalid bounded write evidence");
                }
                return false;
            }
            w.acceptedBytes = write["accepted"].toInt();
            w.writtenBytes = write["written"].toInt();
            w.uncertainBytes = write["uncertain"].toInt();
            event.writeResult = w;
            event.fatal = write["fatal"].toBool();
        } else if (item.contains("write")) {
            error = QStringLiteral("Unexpected write evidence");
            return false;
        }
        if (event.resumed && event.kind != K::Open) {
            error = QStringLiteral("Only open can be resumed");
            return false;
        }
        auto canonical = event.normalized();
        if (!validator.check(canonical, error))
            return false;
        parsed.events.append(std::move(canonical));
    }
    result = std::move(parsed);
    error.clear();
    return true;
}
