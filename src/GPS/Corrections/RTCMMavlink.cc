#include "RTCMMavlink.h"

#include <QtCore/QPointer>
#include <QtCore/QScopeGuard>
#include <QtCore/QSet>

#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(RTCMMavlinkLog, "GPS.Corrections.RTCMMavlink")

RTCMMavlink::RTCMMavlink(QObject* parent) : QObject(parent)
{
    qCDebug(RTCMMavlinkLog) << this;
}

RTCMMavlink::~RTCMMavlink()
{
    qCDebug(RTCMMavlinkLog) << this;
}

void RTCMMavlink::setOutputProvider(OutputProvider provider)
{
    ++_outputRevision;
    _outputProvider = std::move(provider);
}

quint64 RTCMMavlink::submit(QByteArrayView data)
{
    quint64 bytes = 0;
    for (const auto& admission : submitToOutputs(data)) {
        bytes += admission.queuedBytes;
    }
    return bytes;
}

QList<RTCMMavlink::Admission> RTCMMavlink::submitToOutputs(QByteArrayView data)
{
    QList<Admission> admissions;
    if (data.isEmpty() || _submitting) {
        return admissions;
    }
    const QPointer<RTCMMavlink> guard(this);
    const quint64 revision = _outputRevision;
    _submitting = true;
    const auto clearSubmitting = qScopeGuard([guard]() {
        if (guard) {
            guard->_submitting = false;
        }
    });
    const auto current = [this, guard, revision]() { return guard && revision == _outputRevision; };
    const auto provider = _outputProvider;
    _rateTracker.recordBytes(data.size());
    if (_rateTracker.rateUpdated()) {
        qCDebug(RTCMMavlinkLog) << QStringLiteral("RTCM bandwidth: %1 kB/s").arg(_rateTracker.kBps(), 0, 'f', 3);
        emit bandwidthChanged();
        if (!current()) {
            return admissions;
        }
    }
    const auto packed = pack(data, _sequenceId);
    _sequenceId = packed.nextSequenceId;
    const auto outputs = provider ? provider() : QList<Output>();
    if (!current()) {
        return admissions;
    }
    QSet<QString> seen;
    for (const auto& output : outputs) {
        if (output.id.isEmpty() || !output.submit || seen.contains(output.id)) {
            continue;
        }
        seen.insert(output.id);
        Admission admission{output.id, output.session, 0, true};
        for (const auto& packet : packed.packets) {
            const bool accepted = output.submit(packet);
            if (accepted) {
                admission.queuedBytes += packet.data.size();
            }
            if (!accepted || !current()) {
                admission.complete = false;
                break;
            }
        }
        admissions.append(admission);
        if (!current()) {
            break;
        }
    }
    if (!guard) {
        return admissions;
    }
    quint64 submitted = 0;
    for (const auto& admission : admissions) {
        submitted += admission.queuedBytes;
    }
    if (submitted > 0) {
        _submittedBytes += submitted;
        emit deliveryStatsChanged();
    }
    return admissions;
}
