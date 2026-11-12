#pragma once

#include <span>

#include <QtCore/QAbstractListModel>
#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSReceiverReports.h"

/// The satellites the receiver has in view, one row each, in the order of its report: by constellation and then by
/// number. Rows change in place: a satellite that rises or sets inserts or removes its row and the others change
/// only the values that differ, so a view keeps its delegates from one report to the next.
class GPSSatelliteModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by GPSReceiver")
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)

public:
    /// The values of the constellation role: GPSConstellation, which QGCGPSCore declares without QML.
    enum class Constellation
    {
        Unknown = static_cast<int>(GPSConstellation::Unknown),
        GPS = static_cast<int>(GPSConstellation::GPS),
        GLONASS = static_cast<int>(GPSConstellation::GLONASS),
        Galileo = static_cast<int>(GPSConstellation::Galileo),
        BeiDou = static_cast<int>(GPSConstellation::BeiDou),
        QZSS = static_cast<int>(GPSConstellation::QZSS),
        SBAS = static_cast<int>(GPSConstellation::SBAS),
        NavIC = static_cast<int>(GPSConstellation::NavIC),
    };
    Q_ENUM(Constellation)

    enum Roles
    {
        ConstellationRole = Qt::UserRole,
        /// The constellation's translated name; "Other" for one the receiver does not identify.
        ConstellationNameRole,
        /// The row is the first of its constellation, where a view names the constellation.
        StartsConstellationRole,
        /// The satellite's number in its constellation: a PRN or slot, or an SBAS PRN from 120.
        SatelliteIdRole,
        /// Degrees above the horizon and clockwise from north, meaningful while DirectionKnownRole is true.
        ElevationRole,
        AzimuthRole,
        DirectionKnownRole,
        /// Carrier-to-noise density in dB-Hz; zero while the receiver tracks no signal of the satellite.
        SignalStrengthRole,
        /// The receiver uses the satellite in its solution; false too when it does not report which it uses.
        UsedRole,
    };

    explicit GPSSatelliteModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return static_cast<int>(_rows.size()); }

    /// Replaces the rows with @a satellites, keeping the row of each satellite that stays in view.
    void setSatellites(std::span<const GPSSatellite> satellites);

signals:
    void countChanged();

private:
    struct Row
    {
        GPSSatellite satellite;
        bool startsConstellation = false;
    };

    /// Whether @a satellite at @a row follows a row of another constellation, or none.
    bool _startsConstellation(qsizetype row, const GPSSatellite& satellite) const;

    QList<Row> _rows;
};
