#include "GPSCorrectionEventModel.h"

#include <QtCore/QPointer>

#include <algorithm>

#include "GPSCorrectionRouter.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSCorrectionEventModelLog, "GPS.Corrections.GPSCorrectionEventModel")

GPSCorrectionEventModel::GPSCorrectionEventModel(QObject* parent)
    : QAbstractListModel(parent)
{
    qCDebug(GPSCorrectionEventModelLog) << this;
}

GPSCorrectionEventModel::~GPSCorrectionEventModel()
{
    qCDebug(GPSCorrectionEventModelLog) << this;
}

int GPSCorrectionEventModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(_events.size());
}

QVariant GPSCorrectionEventModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.model() != this || index.column() != 0 || index.row() < 0 ||
        index.row() >= _events.size()) {
        return {};
    }
    const auto& event = _events.at(index.row());
    switch (role) {
        case EventSequenceRole:
            return QVariant::fromValue(event.sequence);
        case TimestampMsRole:
            return event.timestampMs;
        case DeliveryIdRole:
            return QVariant::fromValue(event.deliveryId);
        case SourceRole:
            return static_cast<int>(event.source);
        case SourceInstanceRole:
            return event.sourceInstance;
        case SourceSessionRole:
            return QVariant::fromValue(event.sourceSession);
        case DestinationIdRole:
            return event.destinationId;
        case DestinationSessionRole:
            return QVariant::fromValue(event.destinationSession);
        case StageRole:
            return static_cast<int>(event.stage);
        case ReasonRole:
            return static_cast<int>(event.reason);
        case BytesRole:
            return QVariant::fromValue(event.bytes);
    }
    return {};
}

QHash<int, QByteArray> GPSCorrectionEventModel::roleNames() const
{
    static const QHash<int, QByteArray> roles = {
        {EventSequenceRole, "eventSequence"},
        {TimestampMsRole, "timestampMs"},
        {DeliveryIdRole, "deliveryId"},
        {SourceRole, "source"},
        {SourceInstanceRole, "sourceInstance"},
        {SourceSessionRole, "sourceSession"},
        {DestinationIdRole, "destinationId"},
        {DestinationSessionRole, "destinationSession"},
        {StageRole, "stage"},
        {ReasonRole, "reason"},
        {BytesRole, "bytes"},
    };
    return roles;
}

void GPSCorrectionEventModel::setEvents(const QList<GPSCorrectionEvent>& events)
{
    _pendingEvents = events.last((std::min) (events.size(), GPSCorrectionRouter::MAX_EVENTS));
    _pending = true;
    if (_updating) {
        return;
    }
    _updating = true;
    const QPointer<GPSCorrectionEventModel> guard(this);
    while (_pending) {
        _pending = false;
        const auto snapshot = _pendingEvents;
        qsizetype remove = 0;
        while (remove < _events.size() &&
               (snapshot.isEmpty() || _events[remove].sequence != snapshot.first().sequence)) {
            ++remove;
        }
        qsizetype retained = _events.size() - remove;
        if (retained > snapshot.size()) {
            remove = _events.size();
            retained = 0;
        }
        for (qsizetype index = 0; index < retained; ++index) {
            if (_events[remove + index].sequence != snapshot[index].sequence) {
                remove = _events.size();
                retained = 0;
                break;
            }
        }
        if (remove > 0) {
            beginRemoveRows({}, 0, static_cast<int>(remove - 1));
            if (!guard) {
                return;
            }
            _events.remove(0, remove);
            endRemoveRows();
            if (!guard) {
                return;
            }
        }
        if (retained < snapshot.size()) {
            beginInsertRows({}, static_cast<int>(retained), static_cast<int>(snapshot.size() - 1));
            if (!guard) {
                return;
            }
            _events.append(snapshot.sliced(retained));
            endInsertRows();
            if (!guard) {
                return;
            }
        }
    }
    _updating = false;
}
