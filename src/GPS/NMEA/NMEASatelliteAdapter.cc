#include "NMEASatelliteAdapter.h"

#include <algorithm>
#include <cmath>

#include "GPSQtRuntimeScheduler.h"
#include "GPSReadTimestamp.h"
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

}  // namespace

NMEASatelliteAdapter::NMEASatelliteAdapter(QIODevice* source, QObject* parent, GPSRuntimeScheduler* scheduler)
    : QObject(parent)
    , _source(source)
    , _scheduler(scheduler ? scheduler : new GPSQtRuntimeScheduler(this))
{
    qCDebug(NMEASatelliteAdapterLog) << this;
    if (source) {
        connect(source, &QIODevice::readyRead, this, &NMEASatelliteAdapter::_readAvailable);
        connect(source, &QIODevice::aboutToClose, this, &NMEASatelliteAdapter::close);
        connect(source, &QObject::destroyed, this, &NMEASatelliteAdapter::close);
    }
}

NMEASatelliteAdapter::~NMEASatelliteAdapter()
{
    qCDebug(NMEASatelliteAdapterLog) << this;
    close();
}

void NMEASatelliteAdapter::close()
{
    _open = false;
    for (auto* task : {&_idleTask, &_batchTask, &_deliveryTask, &_readTask}) {
        _scheduler->cancel(*task);
        *task = 0;
    }
    _reports.clear();
    _inUse.clear();
    _epochTime.clear();
    _pending.clear();
}

void NMEASatelliteAdapter::_readAvailable()
{
    qsizetype remaining = 64 * 1024;
    while (_open && _source && _source->canReadLine() && remaining > 0) {
        const QByteArray sentence = _source->readLine(4096);
        if (sentence.isEmpty()) {
            break;
        }
        remaining -= sentence.size();
        _parseSentence(sentence, GPSReadTimestamp::from(_source));
    }
    if (_open && _source && _source->canReadLine() && !_readTask) {
        _readTask = _scheduler->schedule(this, std::chrono::microseconds::zero(), [this]() {
            _readTask = 0;
            _readAvailable();
        });
    }
}

void NMEASatelliteAdapter::_parseSentence(const QByteArray& sentence, quint64 receivedAtUs)
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
        bool validFix = false;
        const int fix = fields[2].toInt(&validFix);
        if (!validFix || fix < 1 || fix > 3) {
            return;
        }
        for (int index = 3; index < 15; ++index) {
            if (!fields[index].isEmpty()) {
                bool validId = false;
                const int id = fields[index].toInt(&validId);
                if (!validId || id <= 0) {
                    return;
                }
            }
        }
        const QByteArray talker = gsaTalker(fields);
        if (!supportedTalker(talker)) {
            return;
        }
        if (_inUse.contains(talker) && !_reports.isEmpty()) {
            _flush();
        }
        UsedReport report;
        report.receivedAtUs = receivedAtUs;
        for (int index = 3; fix != 1 && index < 15; ++index) {
            if (!fields[index].isEmpty()) {
                report.ids.insert(fields[index].toInt());
            }
        }
        _inUse[talker] = report;
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
            report = SignalReport{
                .messageCount = total, .satelliteCount = count, .receivedAtUs = receivedAtUs, .satellites = {}};
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
            GPSSatellite satellite;
            satellite.id = id;
            satellite.constellation = NMEAUtils::satelliteConstellation(talker);
            bool valid = false;
            const double elevation = fields[i + 1].toDouble(&valid);
            if (valid && std::isfinite(elevation) && elevation >= 0 && elevation <= 90) {
                satellite.elevationDegrees = elevation;
            }
            const double azimuth = fields[i + 2].toDouble(&valid);
            if (valid && std::isfinite(azimuth) && azimuth >= 0 && azimuth <= 360) {
                satellite.normalizedAzimuthDegrees = azimuth;
            }
            const int strength = fields[i + 3].toInt(&valid);
            if (valid && strength >= 0 && strength <= 99) {
                satellite.signalStrength = strength;
            }
            report.satellites.append(satellite);
        }
        report.receivedAtUs = std::min(report.receivedAtUs, receivedAtUs);
        ++report.nextMessage;
    } else {
        return;
    }
    _scheduler->cancel(_idleTask);
    _idleTask = _scheduler->schedule(this, std::chrono::milliseconds(150), [this]() {
        _idleTask = 0;
        _flush();
    });
    if (!_batchTask) {
        _batchTask = _scheduler->schedule(this, std::chrono::seconds(1), [this]() {
            _batchTask = 0;
            _flush();
        });
    }
}

void NMEASatelliteAdapter::_flush()
{
    _scheduler->cancel(_idleTask);
    _scheduler->cancel(_batchTask);
    _idleTask = _batchTask = 0;
    GPSSatelliteObservation observation;
    observation.updateMode = GPSSatelliteObservation::UpdateMode::ConstellationDelta;
    QMap<QByteArray, GPSSatelliteProvenance> provenance;
    for (auto system = _reports.cbegin(); system != _reports.cend(); ++system) {
        QMap<int, GPSSatellite> satellites;
        bool complete = false;
        quint64 receivedAtUs = 0;
        for (const auto& report : system.value()) {
            if (!report.complete()) {
                continue;
            }
            complete = true;
            receivedAtUs = receivedAtUs == 0 ? report.receivedAtUs : std::min(receivedAtUs, report.receivedAtUs);
            for (const auto& satellite : report.satellites) {
                const auto existing = satellites.constFind(satellite.id);
                // The public model exposes one signal per satellite, retaining its strongest measurement.
                if (existing == satellites.cend() ||
                    satellite.signalStrength.value_or(-1) > existing->signalStrength.value_or(-1)) {
                    satellites[satellite.id] = satellite;
                }
            }
        }
        if (complete) {
            auto& report = provenance[system.key()];
            report.constellation = NMEAUtils::satelliteConstellation(system.key());
            report.inViewTimestampUs = receivedAtUs;
            observation.satellites.append(satellites.values());
        }
    }
    for (auto it = _inUse.cbegin(); it != _inUse.cend(); ++it) {
        auto& report = provenance[it.key()];
        report.constellation = NMEAUtils::satelliteConstellation(it.key());
        report.inUseTimestampUs = it->receivedAtUs;
        report.usedSatelliteIds = it->ids.values();
        std::sort(report.usedSatelliteIds->begin(), report.usedSatelliteIds->end());
        report.satellitesUsed = static_cast<int>(it->ids.size());
    }
    observation.provenance = provenance.values();
    for (const auto& report : observation.provenance) {
        observation.monotonicTimestampUs =
            std::max({observation.monotonicTimestampUs, report.inViewTimestampUs, report.inUseTimestampUs});
    }
    _reports.clear();
    _inUse.clear();
    if (!observation.provenance.isEmpty()) {
        // Bound queued epochs if a receiver outpaces the application event loop.
        if (_pending.size() >= 64) {
            _pending.removeFirst();
        }
        _pending.append(observation);
        if (!_deliveryTask) {
            _deliveryTask = _scheduler->schedule(this, std::chrono::microseconds::zero(), [this]() {
                _deliveryTask = 0;
                _deliver();
            });
        }
    }
}

void NMEASatelliteAdapter::_deliver()
{
    if (!_open || _pending.isEmpty()) {
        return;
    }
    const auto observation = _pending.takeFirst();
    // Schedule before notification; callbacks may close or destroy this assembler.
    if (!_pending.isEmpty()) {
        _deliveryTask = _scheduler->schedule(this, std::chrono::microseconds::zero(), [this]() {
            _deliveryTask = 0;
            _deliver();
        });
    }
    emit observationReceived(observation);
}
