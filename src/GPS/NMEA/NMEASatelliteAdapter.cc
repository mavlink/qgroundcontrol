#include "NMEASatelliteAdapter.h"

#include <algorithm>

#include "NMEAUtils.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NMEASatelliteAdapterLog, "GPS.NMEA.NMEASatelliteAdapter")

namespace {
QByteArray canonicalTalker(QByteArray talker)
{
    if (talker == "BD") {
        return "GB";
    }
    if (talker == "PQ" || talker == "QZ") {
        return "GQ";
    }
    return talker;
}

QByteArray gsaTalker(const QList<QByteArray>& fields)
{
    const QByteArray talker = canonicalTalker(fields[0].mid(1, 2));
    if (talker != "GN") {
        return talker;
    }
    if (fields.size() > 18 && !fields[18].isEmpty()) {
        switch (fields[18].toInt()) {
            case 1:
                return "GP";
            case 2:
                return "GL";
            case 3:
                return "GA";
            case 4:
                return "GB";
            case 5:
                return "GQ";
            default:
                return {};
        }
    }
    // Older GNGSA reports encode the constellation in globally assigned satellite IDs.
    for (int i = 3; i < 15; ++i) {
        const int id = fields[i].toInt();
        if ((id >= 1 && id <= 64) || (id >= 152 && id <= 158)) {
            return "GP";
        }
        if (id >= 65 && id <= 96) {
            return "GL";
        }
        if (id >= 193 && id <= 202) {
            return "GQ";
        }
        if ((id >= 201 && id <= 235) || (id >= 401 && id <= 463)) {
            return "GB";
        }
        if (id >= 301 && id <= 336) {
            return "GA";
        }
    }
    return {};
}

bool supportedTalker(const QByteArray& talker)
{
    return talker == "GP" || talker == "GL" || talker == "GA" || talker == "GB" || talker == "GQ";
}

QByteArray encode(const QList<QByteArray>& fields)
{
    return NMEAUtils::repairChecksum(fields.join(','));
}
}  // namespace

NMEASatelliteAdapter::NMEASatelliteAdapter(QIODevice* source, QObject* parent)
    : QIODevice(parent)
    , _source(source)
    , _idleTimer(this)
    , _batchTimer(this)
{
    qCDebug(NMEASatelliteAdapterLog) << this;
    open(ReadOnly | Unbuffered);
    _idleTimer.setSingleShot(true);
    _idleTimer.setInterval(150);
    _batchTimer.setSingleShot(true);
    _batchTimer.setInterval(1000);
    connect(&_idleTimer, &QTimer::timeout, this, &NMEASatelliteAdapter::_flush);
    connect(&_batchTimer, &QTimer::timeout, this, &NMEASatelliteAdapter::_flush);
    if (source) {
        connect(source, &QIODevice::readyRead, this, &NMEASatelliteAdapter::_readAvailable);
        connect(source, &QIODevice::aboutToClose, this, &NMEASatelliteAdapter::close);
        connect(source, &QObject::destroyed, this, &NMEASatelliteAdapter::close);
    }
}

NMEASatelliteAdapter::~NMEASatelliteAdapter()
{
    qCDebug(NMEASatelliteAdapterLog) << this;
}

qint64 NMEASatelliteAdapter::bytesAvailable() const
{
    return QIODevice::bytesAvailable() + _buffer.size();
}

bool NMEASatelliteAdapter::canReadLine() const
{
    return QIODevice::canReadLine() || _buffer.contains('\n');
}

qint64 NMEASatelliteAdapter::readData(char* data, qint64 maxSize)
{
    const qint64 size = std::min<qint64>(maxSize, _buffer.size());
    std::copy_n(_buffer.constData(), size, data);
    _buffer.remove(0, size);
    return size;
}

qint64 NMEASatelliteAdapter::readLineData(char* data, qint64 maxSize)
{
    const qsizetype newline = _buffer.indexOf('\n');
    return readData(data, newline < 0 ? maxSize : std::min<qint64>(maxSize, newline + 1));
}

void NMEASatelliteAdapter::close()
{
    _idleTimer.stop();
    _batchTimer.stop();
    _reports.clear();
    _inUse.clear();
    _epochTime.clear();
    _buffer.clear();
    QIODevice::close();
}

void NMEASatelliteAdapter::_readAvailable()
{
    while (isOpen() && _source && _source->canReadLine()) {
        _parseSentence(_source->readLine());
    }
}

void NMEASatelliteAdapter::_parseSentence(const QByteArray& sentence)
{
    if (!NMEAUtils::verifyChecksum(sentence)) {
        return;
    }
    QList<QByteArray> fields = sentence.first(sentence.indexOf('*')).split(',');
    if (fields[0].size() != 6) {
        return;
    }
    const QByteArray type = fields[0].right(3);
    if ((type == "RMC" || type == "GGA") && fields.size() > 1 && !fields[1].isEmpty()) {
        if (fields[1] != _epochTime) {
            _flush();
            _epochTime = fields[1];
        }
        return;
    }
    if (type == "GSA" && fields.size() >= 18) {
        const QByteArray talker = gsaTalker(fields);
        if (!supportedTalker(talker)) {
            return;
        }
        if (_inUse.contains(talker) && !_reports.isEmpty()) {
            _flush();
        }
        fields[0] = "$" + talker + "GSA";
        _inUse[talker] = fields;
    } else if (type == "GSV" && fields.size() >= 4) {
        const QByteArray talker = canonicalTalker(fields[0].mid(1, 2));
        bool totalOk = false;
        bool messageOk = false;
        bool countOk = false;
        const int total = fields[1].toInt(&totalOk);
        const int message = fields[2].toInt(&messageOk);
        const int count = fields[3].toInt(&countOk);
        if (!totalOk || !messageOk || !countOk || !supportedTalker(talker) || total < 1 || total > 64 || message < 1 ||
            message > total || count < 0 || count > 256 || total != std::max(1, (count + 3) / 4)) {
            return;
        }
        const int entries = std::min(4, count - (message - 1) * 4);
        const int end = 4 + entries * 4;
        if (fields.size() != end && fields.size() != end + 1) {
            return;
        }
        bool signalOk = true;
        const int signal = fields.size() == end || fields[end].isEmpty() ? -1 : fields[end].toInt(&signalOk, 16);
        if (!signalOk || signal < -1 || signal > 15) {
            return;
        }
        if (message == 1 && _reports.value(talker).value(signal).complete()) {
            _flush();
        }
        auto& report = _reports[talker][signal];
        if (message == 1) {
            report = SignalReport{.messageCount = total, .satelliteCount = count, .satellites = {}};
        }
        if (report.messageCount != total || report.satelliteCount != count || report.nextMessage != message) {
            _reports[talker].remove(signal);
            return;
        }
        for (int i = 4; i < end; i += 4) {
            bool idOk = false;
            const int id = fields[i].toInt(&idOk);
            if (!idOk || id < 1 || id > 999) {
                _reports[talker].remove(signal);
                return;
            }
            report.satellites.append(fields.mid(i, 4));
        }
        ++report.nextMessage;
    } else {
        return;
    }
    _idleTimer.start();
    if (!_batchTimer.isActive()) {
        _batchTimer.start();
    }
}

void NMEASatelliteAdapter::_flush()
{
    _idleTimer.stop();
    _batchTimer.stop();
    QByteArray output;
    for (auto system = _reports.cbegin(); system != _reports.cend(); ++system) {
        QMap<int, QList<QByteArray>> satellites;
        bool complete = false;
        for (const auto& report : system.value()) {
            if (!report.complete()) {
                continue;
            }
            complete = true;
            for (const auto& satellite : report.satellites) {
                const int id = satellite[0].toInt();
                const auto existing = satellites.constFind(id);
                // Qt exposes one signal strength per satellite; retain the strongest reported signal.
                if (existing == satellites.cend() || satellite[3].toInt() > existing.value()[3].toInt()) {
                    satellites[id] = satellite;
                }
            }
        }
        if (!complete) {
            continue;
        }
        const auto entries = satellites.values();
        const int count = static_cast<int>(entries.size());
        const int messages = std::max(1, (count + 3) / 4);
        for (int message = 0; message < messages; ++message) {
            QList<QByteArray> fields{"$" + system.key() + "GSV", QByteArray::number(messages),
                                     QByteArray::number(message + 1), QByteArray::number(count)};
            for (int i = message * 4; i < std::min(count, (message + 1) * 4); ++i) {
                fields.append(entries[i]);
            }
            output += encode(fields);
        }
    }
    for (const auto& fields : std::as_const(_inUse)) {
        output += encode(fields);
    }
    _reports.clear();
    _inUse.clear();
    if (!output.isEmpty()) {
        // Drop whole stale batches if Qt is not consuming requests.
        if (_buffer.size() + output.size() > 64 * 1024) {
            _buffer.clear();
        }
        _buffer += output;
        // Leave the parsing stack before consumers can destroy the connection.
        QMetaObject::invokeMethod(
            this,
            [this]() {
                if (isOpen()) {
                    emit readyRead();
                }
            },
            Qt::QueuedConnection);
    }
}
