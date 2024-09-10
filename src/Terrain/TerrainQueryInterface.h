#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QList>
#include <QtCore/QObject>

class QGeoCoordinate;

Q_DECLARE_LOGGING_CATEGORY(TerrainQueryInterfaceLog)

namespace TerrainQuery {
    enum QueryMode {
        QueryModeNone,
        QueryModeCoordinates,
        QueryModePath,
        QueryModeCarpet
    };

    enum class State {
        Idle,
        Downloading,
    };
} // namespace TerrainQuery

/// Base class for offline/online terrain queries
class TerrainQueryInterface : public QObject
{
    Q_OBJECT

public:
    explicit TerrainQueryInterface(QObject *parent = nullptr) : QObject(parent) { }
    virtual ~TerrainQueryInterface() {}

    /// Request terrain heights for specified coodinates.
    /// Signals: coordinateHeights when data is available
    ///     @param coordinates to query
    virtual void requestCoordinateHeights(const QList<QGeoCoordinate> &coordinates);

    /// Requests terrain heights along the path specified by the two coordinates.
    /// Signals: pathHeights
    ///     @param coordinates to query
    virtual void requestPathHeights(const QGeoCoordinate &fromCoord, const QGeoCoordinate &toCoord);

    /// Request terrain heights for the rectangular area specified.
    /// Signals: carpetHeights when data is available
    ///     @param swCoord South-West bound of rectangular area to query
    ///     @param neCoord North-East bound of rectangular area to query
    ///     @param statsOnly true: Return only stats, no carpet data
    virtual void requestCarpetHeights(const QGeoCoordinate &swCoord, const QGeoCoordinate &neCoord, bool statsOnly);

    void signalCoordinateHeights(bool success, const QList<double> &heights);
    void signalPathHeights(bool success, double distanceBetween, double finalDistanceBetween, const QList<double> &heights);
    void signalCarpetHeights(bool success, double minHeight, double maxHeight, const QList<QList<double>> &carpet);

signals:
    void coordinateHeightsReceived(bool success, const QList<double> &heights);
    void pathHeightsReceived(bool success, double distanceBetween, double finalDistanceBetween, const QList<double> &heights);
    void carpetHeightsReceived(bool success, double minHeight, double maxHeight, const QList<QList<double>> &carpet);

protected:
    TerrainQuery::QueryMode _queryMode = TerrainQuery::QueryMode::QueryModeNone;
};
