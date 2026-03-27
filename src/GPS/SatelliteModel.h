#pragma once

#include "satellite_info.h"

#include <QtCore/QAbstractListModel>
#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtPositioning/QGeoSatelliteInfo>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(SatelliteModelLog)

/// Unified satellite list model for both RTK driver data and Qt Positioning data.
///
/// Thread safety: not thread-safe; must be used from the GUI thread only.
class SatelliteModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int usedCount READ usedCount NOTIFY modelDataChanged)
    Q_PROPERTY(QString constellationSummary READ constellationSummary NOTIFY modelDataChanged)

public:
    enum Roles {
        ConstellationRole = Qt::UserRole + 1,
        SvidRole,
        SnrRole,
        ElevationRole,
        AzimuthRole,
        UsedRole
    };
    Q_ENUM(Roles)

    explicit SatelliteModel(QObject *parent = nullptr);

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Properties
    int count() const { return rowCount(); }
    int usedCount() const { return _usedCount; }
    QString constellationSummary() const { return _constellationSummary; }

    // Update from GPS driver (satellite_info_s). Uses incremental insert/remove for
    // smooth model updates when count changes between calls.
    void updateFromDriverInfo(const satellite_info_s &info);

    // Update from Qt Positioning. Uses beginResetModel/endResetModel because Qt
    // Positioning delivers full snapshots rather than diffs.
    void updateFromQtPositioning(const QList<QGeoSatelliteInfo> &inView,
                                 const QList<QGeoSatelliteInfo> &inUse);

    void clear();

signals:
    void countChanged();
    // Custom property-notification signal. Named distinctly from
    // QAbstractListModel::dataChanged(QModelIndex,QModelIndex,QList<int>) so
    // MOC does not produce an overloaded signal set (which breaks QML
    // Connections resolution).
    void modelDataChanged();

private:
    struct SatEntry {
        int svid        = 0;
        bool used       = false;
        int elevation   = 0;
        int azimuth     = 0;
        int snr         = 0;
        QString constellation;
    };

    // Rebuild _constellationSummary from used-satellite counts.
    // counts maps full constellation name → used satellite count.
    void _buildSummary(const QMap<QString, int> &counts);

    // Map QGeoSatelliteInfo::SatelliteSystem to the canonical constellation name
    // used throughout this class ("GPS", "GLONASS", "Galileo", "BeiDou", "QZSS", …).
    static QString _qtSystemName(QGeoSatelliteInfo::SatelliteSystem system);

    QList<SatEntry> _satellites;
    int _usedCount              = 0;
    QString _constellationSummary;
};
