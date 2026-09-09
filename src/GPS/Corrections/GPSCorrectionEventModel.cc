#include "GPSCorrectionEventModel.h"

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
    const auto snapshot = events.last((std::min) (events.size(), GPSCorrectionRouter::MAX_EVENTS));
    if ((_events.isEmpty() && snapshot.isEmpty()) ||
        (!_events.isEmpty() && _events.size() == snapshot.size() &&
         _events.first().sequence == snapshot.first().sequence && _events.last().sequence == snapshot.last().sequence)) {
        return;
    }
    // One bounded snapshot per refresh avoids one GUI event and model mutation per packet stage.
    beginResetModel();
    _events = snapshot;
    endResetModel();
}
