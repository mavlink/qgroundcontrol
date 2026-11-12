#pragma once

#include <QtCore/QList>
#include <QtCore/QVariantList>
#include <QtPositioning/QGeoCoordinate>

#include "ShapeFileHelper.h"

class QJsonValue;

namespace GeoJsonHelper
{
    ShapeFileHelper::ShapeType determineShapeType(const QString &filePath, QString &errorString);

    /// Get the number of geometry entities in the GeoJSON file
    int getEntityCount(const QString& filePath, QString& errorString);

    /// Load all polygon entities: the outer ring of each, without its closing position and wound clockwise
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolygonsFromFile(const QString& filePath, QList<QList<QGeoCoordinate>>& polygons, QString& errorString,
                              double filterMeters = ShapeFileHelper::kDefaultVertexFilterMeters);

    /// Load all polyline entities
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolylinesFromFile(const QString& filePath, QList<QList<QGeoCoordinate>>& polylines, QString& errorString,
                               double filterMeters = ShapeFileHelper::kDefaultVertexFilterMeters);

    /// Load all point entities
    bool loadPointsFromFile(const QString& filePath, QList<QGeoCoordinate>& points, QString& errorString);

    /// Loads a QGeoCoordinate
    ///     Stored as array [ lon, lat, alt ]
    /// @return false: validation failed
    bool loadGeoJsonCoordinate(const QJsonValue &jsonValue, ///< json value to load from
                               bool altitudeRequired,       ///< true: altitude must be specified
                               QGeoCoordinate &coordinate,  ///< returned QGeoCordinate
                               QString &errorString);       ///< returned error string if load failure

    /// Saves a QGeoCoordinate
    ///     Stored as array [ lon, lat, alt ]
    void saveGeoJsonCoordinate(const QGeoCoordinate &coordinate,    ///< QGeoCoordinate to save
                               bool writeAltitude,                  ///< true: write altitude to json
                               QJsonValue &jsonValue);              ///< json value to save to

    /// Loads a QGeoCoordinate stored as `[lat, lon, alt]` array (QGC plan format).
    /// For GeoJson `[lon, lat, alt]` ordering use `loadGeoJsonCoordinate`.
    bool loadGeoCoordinate(const QJsonValue &jsonValue,
                           bool altitudeRequired,
                           QGeoCoordinate &coordinate,
                           QString &errorString);

    /// Saves a QGeoCoordinate as `[lat, lon, alt]` array (QGC plan format).
    void saveGeoCoordinate(const QGeoCoordinate &coordinate,
                           bool writeAltitude,
                           QJsonValue &jsonValue);

    /// Loads a list of QGeoCoordinates (QGC plan format) from a json array.
    bool loadGeoCoordinateArray(const QJsonValue &jsonValue,
                                bool altitudeRequired,
                                QVariantList &rgVarPoints,
                                QString &errorString);
    bool loadGeoCoordinateArray(const QJsonValue &jsonValue,
                                bool altitudeRequired,
                                QList<QGeoCoordinate> &rgPoints,
                                QString &errorString);

    /// Saves a list of QGeoCoordinates (QGC plan format) to a json array.
    void saveGeoCoordinateArray(const QVariantList &rgVarPoints,
                                bool writeAltitude,
                                QJsonValue &jsonValue);
    void saveGeoCoordinateArray(const QList<QGeoCoordinate> &rgPoints,
                                bool writeAltitude,
                                QJsonValue &jsonValue);
};
