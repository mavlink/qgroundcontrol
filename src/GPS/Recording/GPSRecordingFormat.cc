#include "GPSRecordingFormat.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonParseError>
#include <QtCore/QSet>

#include <cmath>
#include <limits>

#include "GPSReceiverProfile.h"
#include "JsonParsing.h"

namespace {
using K = GPSRecordingEvent::Kind;
using T = GPSRecordingMetadata::Transport;
using W = GPSTransport::WriteStatus;

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
                                          {"fixed_baud", QJsonValue::Double, version == 2},
                                          {"configured", QJsonValue::Bool, true},
                                          {"constellation_mask", QJsonValue::Double, true},
                                          {"dynamic_model", QJsonValue::Double, true},
                                          {"output_rate_hz", QJsonValue::Double, true},
                                          {"heading_offset_deg", QJsonValue::Double, true},
                                          {"base", QJsonValue::Object, true}},
                                         error)) {
        return false;
    }
    auto& m = result;
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
    if (version == 2) {
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

GPSRecordingMetadata GPSRecordingMetadata::forReceiver(const GPSReceiverConfig& config, GPSType type)
{
    GPSRecordingMetadata metadata;
    metadata.receiver = config;
    metadata.driverType = static_cast<int>(type);
    metadata.configured = true;
    return metadata;
}

GPSRecordingMetadata GPSRecordingMetadata::fromProfile(const GPSReceiverProfile& profile)
{
    GPSRecordingMetadata result;
    result.receiver = profile.receiver;
    result.configured = profile.configurationPolicy == GPSReceiverProfile::ConfigurationPolicy::Configure;
    result.driverType = result.configured ? static_cast<int>(profile.driverType) : -1;
    result.initialBaud = profile.endpoint.baud;
    using E = GPSReceiverProfile::Endpoint::Kind;
    switch (profile.endpoint.kind) {
        case E::Disabled:
            result.transport = T::Unknown;
            break;
        case E::Serial:
            result.transport = T::Serial;
            break;
        case E::Tcp:
            result.transport = T::Tcp;
            result.fixedBaud = 115200;
            break;
        case E::UdpListener:
            result.transport = T::UdpListener;
            result.fixedBaud = 115200;
            break;
        case E::UdpPeer:
            result.transport = T::UdpPeer;
            result.fixedBaud = 115200;
            break;
    }
    return result;
}

QByteArray GPSRecordingDocument::encode(QString* error) const
{
    if (events.size() > MAX_EVENTS) {
        if (error) {
            *error = QStringLiteral("Recording has too many events");
        }
        return {};
    }
    qsizetype estimatedBytes = 256;
    for (const auto& event : events) {
        const qsizetype overhead = event.kind == K::Session ? 1536 : 384;
        if (estimatedBytes > MAX_BYTES - overhead || event.bytes.size() > (MAX_BYTES - estimatedBytes - overhead) / 2) {
            if (error) {
                *error = QStringLiteral("Recording exceeds 4 MiB");
            }
            return {};
        }
        estimatedBytes += overhead + event.bytes.size() * 2;
    }
    QJsonArray output;
    for (const auto& event : events) {
        QJsonObject item{{"at_us", static_cast<qint64>(event.atUs)},
                         {"stream", static_cast<qint64>(event.stream)},
                         {"kind", name(event.kind, kinds)}};
        if (!event.bytes.isEmpty()) {
            item.insert("hex", QString::fromLatin1(event.bytes.toHex()));
        }
        if (event.startedAtUs) {
            item.insert("started_us", static_cast<qint64>(event.startedAtUs));
        }
        if (event.resumed) {
            item.insert("resumed", true);
        }
        if (event.kind == K::Session) {
            item.insert("profile", metadataJson(event.metadata));
        } else if (event.kind == K::ConfigurationFinished) {
            item.insert("status", name(event.value, configurationStatuses));
        } else if (event.value || event.kind == K::WriteError) {
            item.insert("value", event.value);
        }
        if (event.writeResult) {
            const auto& w = *event.writeResult;
            item.insert("write", QJsonObject{{"status", name(w.status, writes)},
                                             {"accepted", w.acceptedBytes},
                                             {"written", w.writtenBytes},
                                             {"uncertain", w.uncertainBytes},
                                             {"fatal", event.fatal}});
        }
        output.append(item);
    }
    const auto bytes = QJsonDocument(QJsonObject{{"fileType", "GPSRecording"},
                                                 {"version", CURRENT_VERSION},
                                                 {"description", "QGC receiver recording"},
                                                 {"limit_reached", limitReached},
                                                 {"events", output}})
                           .toJson(QJsonDocument::Compact);
    GPSRecordingDocument validated;
    QString validationError;
    if (!decode(bytes, validated, validationError)) {
        if (error) {
            *error = validationError;
        }
        return {};
    }
    if (error) {
        error->clear();
    }
    return bytes;
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
    if ((version == 2 || object.contains("fileType")) &&
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
    quint64 previous = 0;
    QSet<quint64> sessions;
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
                                              {"write", QJsonValue::Object, false}},
                                             error)) {
            return false;
        }
        GPSRecordingEvent event;
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
        if (event.atUs < previous || (item.contains("started_us") && !integer(item["started_us"], 0, event.atUs))) {
            error = QStringLiteral("Out-of-order event or invalid operation timing");
            return false;
        }
        event.startedAtUs = item["started_us"].toInteger();
        previous = event.atUs;
        const auto hex = item["hex"].toString().toLatin1();
        event.bytes = QByteArray::fromHex(hex);
        if (event.bytes.toHex() != hex.toLower() ||
            ((event.kind == K::Rx || event.kind == K::Tx) && event.bytes.isEmpty())) {
            error = QStringLiteral("Invalid recording payload hex");
            return false;
        }
        if (event.kind == K::Session) {
            if (!item.contains("profile") || sessions.contains(event.stream) ||
                !readMetadata(item["profile"].toObject(), version, event.metadata, error)) {
                if (error.isEmpty()) {
                    error = QStringLiteral("Missing or duplicate stream profile");
                }
                return false;
            }
            sessions.insert(event.stream);
        } else if (item.contains("profile")) {
            error = QStringLiteral("Profile outside session event");
            return false;
        }
        if (event.resumed && event.kind != K::Open) {
            error = QStringLiteral("Only open can be resumed");
            return false;
        }
        if (event.kind == K::ConfigurationFinished) {
            if (version == 2 ? !parseName(item["status"], event.value, configurationStatuses)
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
            GPSTransport::WriteResult w;
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
            if (w.writtenBytes + w.uncertainBytes > w.acceptedBytes ||
                ((w.status == W::Unsupported || w.status == W::InvalidData) && w.acceptedBytes != 0) ||
                (w.status == W::Completed && (w.writtenBytes != event.bytes.size() || w.uncertainBytes))) {
                error = QStringLiteral("Inconsistent bounded write counts");
                return false;
            }
            event.writeResult = w;
            event.fatal = write["fatal"].toBool();
        } else if (item.contains("write")) {
            error = QStringLiteral("Unexpected write evidence");
            return false;
        }
        parsed.events.append(std::move(event));
    }
    result = std::move(parsed);
    error.clear();
    return true;
}

bool GPSRecordingDocument::selectStream(quint64 requestedStream, QVector<GPSRecordingEvent>& selected,
                                        std::optional<GPSRecordingMetadata>& metadata, quint64& selectedStream,
                                        QString& error) const
{
    const quint64 id = requestedStream ? requestedStream : events.isEmpty() ? 0 : events.first().stream;
    QVector<GPSRecordingEvent> output;
    std::optional<GPSRecordingMetadata> profile;
    for (const auto& event : events) {
        if (event.stream != id) {
            continue;
        }
        output.append(event);
        if (event.kind == K::Session) {
            profile = event.metadata;
        }
    }
    if (requestedStream && output.isEmpty()) {
        error = QStringLiteral("Requested recording stream was not found");
        return false;
    }
    selected = std::move(output);
    metadata = profile;
    selectedStream = id;
    error.clear();
    return true;
}
