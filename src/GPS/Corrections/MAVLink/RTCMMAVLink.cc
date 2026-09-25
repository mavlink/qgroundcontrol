#include "RTCMMAVLink.h"

#include <algorithm>
#include <utility>

#include <QtCore/QPointer>
#include <QtCore/QScopeGuard>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(RTCMMAVLinkLog, "GPS.Corrections.RTCMMAVLink")

RTCMMAVLink::RTCMMAVLink(QObject* parent)
    : QObject(parent)
{
    qCDebug(RTCMMAVLinkLog) << this;
}

RTCMMAVLink::~RTCMMAVLink()
{
    qCDebug(RTCMMAVLinkLog) << this;
}

void RTCMMAVLink::setOutputProvider(OutputProvider provider)
{
    ++_outputRevision;
    _outputProvider = std::move(provider);
}

QList<RTCMMAVLink::Admission> RTCMMAVLink::submitToOutputs(QByteArrayView data)
{
    QList<Admission> admissions;
    if (data.isEmpty() || _submitting) {
        return admissions;
    }
    const QPointer<RTCMMAVLink> guard(this);
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
        qCDebug(RTCMMAVLinkLog) << QStringLiteral("RTCM bandwidth: %1 kB/s").arg(_rateTracker.kBps(), 0, 'f', 3);
        emit bandwidthChanged();
        if (!current()) {
            return admissions;
        }
    }
    const auto packed = RTCMMAVLinkPacket::pack(data, _sequenceId);
    _sequenceId = packed.nextSequenceId;
    const auto outputs = provider ? provider() : QList<Output>();
    if (!current()) {
        return admissions;
    }
    admissions.reserve(outputs.size());
    for (const auto& output : outputs) {
        if (output.id.isEmpty() || !output.submit ||
            std::any_of(admissions.cbegin(), admissions.cend(),
                        [&output](const Admission& admitted) { return admitted.id == output.id; })) {
            continue;
        }
        Admission admission{output.id, output.session, 0, true};
        for (const auto& packet : packed.packets) {
            if (!current()) {
                admission.complete = false;
                break;
            }
            const bool accepted = output.submit(packet);
            if (accepted) {
                admission.queuedBytes += packet.data.size();
            }
            if (!accepted) {
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
