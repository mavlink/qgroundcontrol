#include "GPSRecordingReader.h"

#include <algorithm>
#include <chrono>

#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QIODevice>
#include <QtCore/QMetaEnum>

#include "GPSRecordingTransport.h"

namespace GPSTest {

namespace {
/// Whole bytes as the recorder writes them, in lower-case hexadecimal.
bool hexadecimal(const QByteArray& value)
{
    return !value.isEmpty() && value.size() % 2 == 0 && std::ranges::all_of(value, [](char digit) {
        return (digit >= '0' && digit <= '9') || (digit >= 'a' && digit <= 'f');
    });
}

/// @a line without its line ending, or nothing when it has none.
std::optional<QByteArray> terminated(QByteArray line)
{
    if (!line.endsWith('\n')) {
        return std::nullopt;
    }
    line.chop(line.endsWith("\r\n") ? 2 : 1);
    return line;
}

/// Longer than any settings record.
constexpr qint64 MAX_SETTINGS_LINE_BYTES = 1024;

/// The receiver type a recording names @a name, in lower case.
std::optional<GPSType> recordedType(const QByteArray& name)
{
    const QMetaEnum types = QMetaEnum::fromType<GPSType>();
    for (int index = 0; index < types.keyCount(); ++index) {
        if (QByteArray(types.key(index)).toLower() == name) {
            return static_cast<GPSType>(types.value(index));
        }
    }
    return std::nullopt;
}

/// The "key=value" pairs of a settings record, each read at most once: what is left unread, or was not there to
/// read, makes the record invalid.
class SettingsFields
{
public:
    explicit SettingsFields(const QByteArray& value)
    {
        for (const QByteArray& field : value.split(',')) {
            const qsizetype separator = field.indexOf('=');
            if (separator <= 0 || _fields.contains(field.first(separator))) {
                _valid = false;
                return;
            }
            _fields.insert(field.first(separator), field.sliced(separator + 1));
        }
    }

    QByteArray text(const char* key)
    {
        _valid = _valid && _fields.contains(key);
        return _fields.take(key);
    }

    double number(const char* key)
    {
        bool read = false;
        const double value = text(key).toDouble(&read);
        _valid = _valid && read;
        return value;
    }

    template <typename Integer>
    Integer integer(const char* key, Integer minimum, Integer maximum)
    {
        bool read = false;
        const qlonglong value = text(key).toLongLong(&read);
        _valid = _valid && read && value >= qlonglong(minimum) && value <= qlonglong(maximum);
        return static_cast<Integer>(value);
    }

    bool flag(const char* key) { return integer<int>(key, 0, 1) == 1; }

    bool allRead() const { return _valid && _fields.isEmpty(); }

private:
    QHash<QByteArray, QByteArray> _fields;
    bool _valid = true;
};

/// The settings a SETTINGS record with @a value holds (GPSRecordingTransport::settingsRecord()), or nothing when it
/// is not one.
std::optional<GPSRecordingSettings> recordedSettings(const QByteArray& value)
{
    SettingsFields fields(value);
    GPSRecordingSettings settings;
    const std::optional<GPSType> type = recordedType(fields.text("type"));
    bool known = type.has_value();
    settings.type = type.value_or(settings.type);
    settings.config.baudRate = fields.integer<uint32_t>("baud", 0, GPS_MAX_BAUD_RATE);
    settings.fixedBaudRate = fields.integer<unsigned>("fixed-baud", 0, GPS_MAX_BAUD_RATE);
    settings.config.allowPersistentChanges = fields.flag("save");
    settings.config.base.compactObservations = fields.flag("compact");
    const QByteArray base = fields.text("base");
    constexpr qlonglong MAX_SECONDS = std::chrono::seconds::max().count();
    if (base == "fixed") {
        GPSBaseStationConfig::Fixed fixed;
        fixed.position = {.latitudeDegrees = fields.number("latitude"),
                          .longitudeDegrees = fields.number("longitude"),
                          .altitudeMeters = fields.number("altitude")};
        fixed.accuracyMeters = static_cast<float>(fields.number("accuracy"));
        settings.config.base.mode = fixed;
    } else if (base == "averaging") {
        settings.config.base.mode = GPSBaseStationConfig::ReceiverAveraging{
            std::chrono::seconds(fields.integer<qlonglong>("duration", 0, MAX_SECONDS))};
    } else if (base == "survey") {
        const std::chrono::seconds duration(fields.integer<qlonglong>("duration", 0, MAX_SECONDS));
        settings.config.base.mode =
            GPSBaseStationConfig::SurveyIn{.accuracyMeters = fields.number("accuracy"), .duration = duration};
    } else {
        known = false;
    }
    return known && fields.allRead() ? std::optional(settings) : std::nullopt;
}
}  // namespace

GPSRecordingReader::GPSRecordingReader(QIODevice& device)
    : _device(device)
{}

bool GPSRecordingReader::readHeader()
{
    // The header, a CR and the LF.
    const QByteArray start = _device.peek(sizeof(GPSRecordingTransport::FILE_HEADER) + 1);
    const std::optional<QByteArray> header = terminated(start.first(start.indexOf('\n') + 1));
    if (header != QByteArray(GPSRecordingTransport::FILE_HEADER)) {
        return false;
    }
    (void) _device.skip(start.indexOf('\n') + 1);
    _readSettings();
    return true;
}

void GPSRecordingReader::_readSettings()
{
    const QByteArray start = _device.peek(MAX_SETTINGS_LINE_BYTES);
    const std::optional<QByteArray> line = terminated(start.first(start.indexOf('\n') + 1));
    const QByteArrayList fields = line ? line->split(' ') : QByteArrayList();
    // Anything else is a record or damage, which next() reads.
    if (fields.size() != 3 || fields[1] != GPSRecordingTransport::SETTINGS) {
        return;
    }
    (void) _device.skip(start.indexOf('\n') + 1);
    ++_line;
    bool timed = false;
    _latest = std::chrono::microseconds(fields[0].toLongLong(&timed));
    _settings = recordedSettings(fields[2]);
    if (!timed || _latest < std::chrono::microseconds::zero() || !_settings) {
        (void) _fail(QStringLiteral("not the settings of a session"));
    }
}

std::optional<GPSRecordingRecord> GPSRecordingReader::next()
{
    if (!_error.isEmpty() || _device.atEnd()) {
        return std::nullopt;
    }
    ++_line;
    const std::optional<QByteArray> line = terminated(_device.readLine());
    if (!line) {
        return _fail(QStringLiteral("the recording ends within a record"));
    }
    const QByteArrayList fields = line->split(' ');
    if (fields.size() != 3) {
        return _fail(QStringLiteral("not a time, a kind and a value"));
    }
    bool timed = false;
    GPSRecordingRecord record;
    record.time = std::chrono::microseconds(fields[0].toLongLong(&timed));
    if (!timed || record.time < _latest) {
        return _fail(QStringLiteral("not a time at or after the time of the record before it"));
    }
    const QByteArray& value = fields[2];
    if (fields[1] == GPSRecordingTransport::BAUD) {
        bool valid = false;
        record.kind = GPSRecordingRecord::Kind::BaudRate;
        record.baudRate = value.toUInt(&valid);
        if (!valid) {
            return _fail(QStringLiteral("not a baud rate"));
        }
    } else if (fields[1] == GPSRecordingTransport::RECEIVED || fields[1] == GPSRecordingTransport::SENT) {
        if (!hexadecimal(value)) {
            return _fail(QStringLiteral("not whole bytes in lower-case hexadecimal"));
        }
        record.kind = fields[1] == GPSRecordingTransport::SENT ? GPSRecordingRecord::Kind::Sent
                                                               : GPSRecordingRecord::Kind::Received;
        record.bytes = QByteArray::fromHex(value);
    } else if (fields[1] == GPSRecordingTransport::SETTINGS) {
        return _fail(QStringLiteral("the settings are not the first record"));
    } else {
        return _fail(QStringLiteral("unknown record kind"));
    }
    _latest = record.time;
    return record;
}

QList<GPSRecordingRecord> GPSRecordingReader::readFile(const QString& path, QString* error,
                                                       std::optional<GPSRecordingSettings>* settings)
{
    QList<GPSRecordingRecord> records;
    QFile file(path);
    GPSRecordingReader reader(file);
    QString failure;
    if (!file.open(QIODevice::ReadOnly)) {
        failure = file.errorString();
    } else if (!reader.readHeader()) {
        failure = QStringLiteral("not a receiver recording");
    } else {
        while (auto record = reader.next()) {
            records.append(std::move(*record));
        }
        failure = file.error() == QFileDevice::NoError ? reader.error() : file.errorString();
    }
    if (error) {
        *error = failure;
    }
    if (settings) {
        *settings = reader.settings();
    }
    return records;
}

std::optional<GPSRecordingRecord> GPSRecordingReader::_fail(const QString& reason)
{
    _error = QStringLiteral("line %1: %2").arg(_line).arg(reason);
    return std::nullopt;
}

}  // namespace GPSTest
