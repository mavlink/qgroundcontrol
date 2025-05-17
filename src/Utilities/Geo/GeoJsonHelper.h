/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtPositioning/QGeoCoordinate>

#include "ShapeFileHelper.h"

class QJsonValue;

Q_DECLARE_LOGGING_CATEGORY(GeoJsonHelperLog)

namespace GeoJsonHelper
{
    ShapeFileHelper::ShapeType determineShapeType(const QString &filePath, QString &errorString);
    bool loadPolygonFromFile(const QString &filePath, QList<QGeoCoordinate> &vertices, QString &errorString);
    bool loadPolylineFromFile(const QString &filePath, QList<QGeoCoordinate> &coords, QString &errorString);

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
};
