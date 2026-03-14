#pragma once

#include "satellite_info.h"

#include <QtCore/QAbstractListModel>

class RTKSatelliteModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY satelliteDataChanged)
    Q_PROPERTY(int usedCount READ usedCount NOTIFY satelliteDataChanged)
    Q_PROPERTY(QString constellationSummary READ constellationSummary NOTIFY satelliteDataChanged)

public:
    enum Roles {
        SvidRole = Qt::UserRole + 1,
        UsedRole,
        ElevationRole,
        AzimuthRole,
        SnrRole,
        ConstellationRole
    };

    explicit RTKSatelliteModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void update(const satellite_info_s &info);
    void clear();

    int count() const { return rowCount(); }
    int usedCount() const { return _usedCount; }
    QString constellationSummary() const { return _constellationSummary; }

signals:
    void satelliteDataChanged();

private:
    static QString _constellation(uint8_t svid, uint8_t prn);

    struct SatEntry {
        uint8_t svid;
        uint8_t used;
        uint8_t elevation;
        uint16_t azimuth;
        uint8_t snr;
        QString constellation;
    };

    QList<SatEntry> _satellites;
    int _usedCount = 0;
    QString _constellationSummary;
};
