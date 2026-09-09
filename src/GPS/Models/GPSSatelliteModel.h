#pragma once

#include <QtCore/QAbstractListModel>

#include "GPSObservation.h"

/// One receiver session's satellite snapshot. Signal strength retains the source's native units.
class GPSSatelliteModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString sourceId READ sourceId NOTIFY stateChanged)
    Q_PROPERTY(qulonglong sessionId READ sessionId NOTIFY stateChanged)
    Q_PROPERTY(bool fresh READ fresh NOTIFY stateChanged)
    Q_PROPERTY(int count READ count NOTIFY stateChanged)

public:
    enum Role
    {
        SatelliteIdRole = Qt::UserRole + 1,
        PrnRole,
        ConstellationRole,
        UsedRole,
        ElevationRole,
        SignalStrengthRole,
        AzimuthRole,
        SourceIdRole
    };
    Q_ENUM(Role)

    explicit GPSSatelliteModel(QObject* parent = nullptr);
    ~GPSSatelliteModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString sourceId() const { return _current.sourceId; }

    qulonglong sessionId() const { return _current.sessionId; }

    bool fresh() const { return _current.fresh; }

    int count() const { return static_cast<int>(_current.satellites.size()); }

    void beginSession(const QString& sourceId, quint64 sessionId);
    /// Project an accepted GPSSatelliteStore snapshot; acceptance and expiry belong to the store.
    void updateObservation(const GPSSatelliteObservation& observation);
    void reset();

signals:
    void stateChanged();

private:
    struct Snapshot
    {
        QString sourceId;
        quint64 sessionId = 0;
        quint64 revision = 0;
        bool fresh = false;
        QList<GPSSatellite> satellites;
    };

    void _publish();
    static QString _constellationName(GPSSatellite::Constellation constellation);

    Snapshot _current;
    Snapshot _pending;
    bool _publishing = false;
    bool _publicationQueued = false;
};
