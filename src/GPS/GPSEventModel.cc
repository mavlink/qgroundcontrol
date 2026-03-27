#include "GPSEventModel.h"

GPSEventModel::GPSEventModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int GPSEventModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : _events.size();
}

QVariant GPSEventModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= _events.size()) {
        return {};
    }
    const auto &e = _events.at(index.row());
    switch (role) {
    case TimestampRole:  return e.timestamp.toString(Qt::ISODate);
    case SeverityRole:   return severityString(e.severity);
    case SourceRole:     return sourceString(e.source);
    case MessageRole:    return e.message;
    default:             return {};
    }
}

QHash<int, QByteArray> GPSEventModel::roleNames() const
{
    return {
        {TimestampRole, "timestamp"},
        {SeverityRole,  "severity"},
        {SourceRole,    "source"},
        {MessageRole,   "message"},
    };
}

void GPSEventModel::append(const GPSEvent &event, int maxSize)
{
    if (maxSize <= 0) {
        maxSize = 100;
    }
    if (_events.size() >= maxSize) {
        beginRemoveRows(QModelIndex(), 0, 0);
        _events.removeFirst();
        endRemoveRows();
    }
    const int row = _events.size();
    beginInsertRows(QModelIndex(), row, row);
    _events.append(event);
    endInsertRows();
    emit countChanged();
}

void GPSEventModel::clear()
{
    if (_events.isEmpty()) {
        return;
    }
    beginResetModel();
    _events.clear();
    endResetModel();
    emit countChanged();
}

QString GPSEventModel::severityString(GPSEvent::Severity s)
{
    switch (s) {
    case GPSEvent::Severity::Info:    return QStringLiteral("Info");
    case GPSEvent::Severity::Warning: return QStringLiteral("Warning");
    case GPSEvent::Severity::Error:   return QStringLiteral("Error");
    }
    return QStringLiteral("Unknown");
}

QString GPSEventModel::sourceString(GPSEvent::Source s)
{
    switch (s) {
    case GPSEvent::Source::RTKBase:   return QStringLiteral("RTK Base");
    case GPSEvent::Source::NTRIP:     return QStringLiteral("NTRIP");
    case GPSEvent::Source::Transport: return QStringLiteral("Transport");
    case GPSEvent::Source::GPS:       return QStringLiteral("GPS");
    case GPSEvent::Source::GCS:       return QStringLiteral("GCS");
    }
    return QStringLiteral("Unknown");
}
