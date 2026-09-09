#pragma once

#include <QtCore/QAbstractListModel>
#include <QtCore/QSet>
#include <QtCore/QTimer>
#include <QtPositioning/QGeoSatelliteInfo>

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

    explicit GPSSatelliteModel(QObject* parent = nullptr, int freshnessTimeoutMs = 5000);
    ~GPSSatelliteModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString sourceId() const { return _current.sourceId; }

    qulonglong sessionId() const { return _current.sessionId; }

    bool fresh() const { return _current.fresh; }

    int count() const { return static_cast<int>(_current.satellites.size()); }

    void beginSession(const QString& sourceId, quint64 sessionId);
    void updateObservation(const GPSSatelliteObservation& observation);
    void updateNmeaSatellites(const QList<QGeoSatelliteInfo>& view, const QList<QGeoSatelliteInfo>& used,
                              bool usedKnown, quint64 receivedAtUs, quint64 sessionId,
                              const std::optional<QSet<int>>& usedSystems = std::nullopt);
    void reset();

signals:
    void stateChanged();

private:
    struct Snapshot
    {
        QString sourceId;
        quint64 sessionId = 0;
        quint64 timestampUs = 0;
        bool fresh = false;
        QList<GPSSatellite> satellites;
    };

    void _publish();
    void _expire();
    void _armTimer();
    static QString _constellationName(GPSSatellite::Constellation constellation);

    Snapshot _current;
    Snapshot _pending;
    QTimer _expiryTimer;
    int _freshnessTimeoutMs;
    bool _publishing = false;
    bool _publicationQueued = false;
};
