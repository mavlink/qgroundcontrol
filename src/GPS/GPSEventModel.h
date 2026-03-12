#pragma once

#include "GPSEvent.h"

#include <QtCore/QAbstractListModel>
#include <QtCore/QList>

class GPSManager;

class GPSEventModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        TimestampRole = Qt::UserRole + 1,
        SeverityRole,
        SourceRole,
        MessageRole
    };

    explicit GPSEventModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return rowCount(); }

    void append(const GPSEvent &event, int maxSize);
    void clear();

signals:
    void countChanged();

private:
    static QString severityString(GPSEvent::Severity s);
    static QString sourceString(GPSEvent::Source s);

    QList<GPSEvent> _events;
};
