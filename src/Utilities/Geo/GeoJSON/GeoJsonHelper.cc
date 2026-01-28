#include "GeoJsonHelper.h"
#include "GeoFileIO.h"
#include "GeoUtilities.h"
#include "JsonHelper.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonValue>
#include <QtLocation/private/qgeojson_p.h>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPath>
#include <QtPositioning/QGeoPolygon>
#include <QtPositioning/QGeoCircle>

QGC_LOGGING_CATEGORY(GeoJsonHelperLog, "Utilities.Geo.GeoJsonHelper")

namespace {
    constexpr const char *kFormatName = "GeoJSON";
}

GeoFormatRegistry::ShapeType GeoJsonHelper::determineShapeType(const QString &filePath, QString &errorString)
{
    using ShapeType = GeoFormatRegistry::ShapeType;

    const auto jsonResult = GeoFileIO::loadJson(filePath, QString::fromLatin1(kFormatName));
    if (!jsonResult.success) {
        errorString = jsonResult.error;
        return ShapeType::Error;
    }

    const QVariantList shapes = QGeoJson::importGeoJson(jsonResult.document);
    if (shapes.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),
            QT_TRANSLATE_NOOP("GeoJson", "No shapes found in GeoJson file."));
        qCWarning(GeoJsonHelperLog) << errorString;
        return ShapeType::Error;
    }

    // QGeoJson::importGeoJson returns QVariantList of QVariantMaps with "type" and "data" keys
    for (const QVariant &item : shapes) {
        if (!item.canConvert<QVariantMap>()) {
            continue;
        }
        const QVariantMap map = item.toMap();
        const QVariant data = map.value(QStringLiteral("data"));
        if (data.canConvert<QGeoShape>()) {
            const QGeoShape shape = data.value<QGeoShape>();
            if (shape.type() == QGeoShape::PathType) {
                return ShapeType::Polyline;
            } else if (shape.type() == QGeoShape::PolygonType) {
                return ShapeType::Polygon;
            } else if (shape.type() == QGeoShape::CircleType) {
                return ShapeType::Point;
            }
        }
    }

    errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),
        QT_TRANSLATE_NOOP("GeoJson", "No supported type found in GeoJson file."));
    qCWarning(GeoJsonHelperLog) << errorString;
    return ShapeType::Error;
}

int GeoJsonHelper::getEntityCount(const QString &filePath, QString &errorString)
{
    const auto jsonResult = GeoFileIO::loadJson(filePath, QString::fromLatin1(kFormatName));
    if (!jsonResult.success) {
        errorString = jsonResult.error;
        return 0;
    }

    const QVariantList shapes = QGeoJson::importGeoJson(jsonResult.document);
    int count = 0;
    // QGeoJson::importGeoJson returns QVariantList of QVariantMaps with "type" and "data" keys
    for (const QVariant &item : shapes) {
        if (!item.canConvert<QVariantMap>()) {
            continue;
        }
        const QVariantMap map = item.toMap();
        const QVariant data = map.value(QStringLiteral("data"));
        if (data.canConvert<QGeoShape>()) {
            count++;
        }
    }
    return count;
}

// ============================================================================
// Load functions
// ============================================================================

bool GeoJsonHelper::loadPolygonFromFile(const QString &filePath, QList<QGeoCoordinate> &vertices, QString &errorString, double filterMeters)
{
    QList<QList<QGeoCoordinate>> polygons;
    if (!loadPolygonsFromFile(filePath, polygons, errorString, filterMeters)) {
        return false;
    }
    vertices = polygons.first();
    return true;
}

namespace {
    // Logging callback for coordinate validation warnings
    void logValidationWarning(int index, const QString &type, double value)
    {
        qCWarning(GeoJsonHelperLog) << "Coordinate" << index << type << "out of range:" << value;
    }

    // Helper to extract polygons from a QVariantList of QGeoJson items (handles nested FeatureCollections)
    // Also converts circles to polygon approximations
    void extractPolygons(const QVariantList &items, QList<QList<QGeoCoordinate>> &polygons, double filterMeters)
    {
        for (const QVariant &item : items) {
            if (!item.canConvert<QVariantMap>()) {
                continue;
            }
            const QVariantMap map = item.toMap();
            const QString type = map.value(QStringLiteral("type")).toString();
            const QVariant data = map.value(QStringLiteral("data"));

            if (type == QStringLiteral("FeatureCollection") && data.canConvert<QVariantList>()) {
                // Recursively process nested features
                extractPolygons(data.toList(), polygons, filterMeters);
            } else if (data.canConvert<QGeoShape>()) {
                const QGeoShape shape = data.value<QGeoShape>();

                if (shape.type() == QGeoShape::PolygonType) {
                    const QGeoPolygon poly = data.value<QGeoPolygon>();
                    QList<QGeoCoordinate> vertices = poly.perimeter();
                    GeoUtilities::removeClosingVertex(vertices);
                    GeoUtilities::validateAndNormalizeCoordinates(vertices, logValidationWarning);
                    GeoFormatRegistry::filterVertices(vertices, filterMeters, GeoUtilities::kMinPolygonVertices);
                    polygons.append(vertices);
                } else if (shape.type() == QGeoShape::CircleType) {
                    // Convert circle to polygon approximation
                    const QGeoCircle circle = data.value<QGeoCircle>();
                    if (circle.isValid() && circle.radius() > 0) {
                        QList<QGeoCoordinate> vertices = GeoUtilities::circleToPolygon(
                            circle.center(), circle.radius());
                        if (vertices.size() >= GeoUtilities::kMinPolygonVertices) {
                            GeoUtilities::validateAndNormalizeCoordinates(vertices, logValidationWarning);
                            GeoFormatRegistry::filterVertices(vertices, filterMeters, GeoUtilities::kMinPolygonVertices);
                            polygons.append(vertices);
                            qCDebug(GeoJsonHelperLog) << "Converted circle (r=" << circle.radius() << "m) to polygon with"
                                                      << vertices.size() << "vertices";
                        }
                    }
                }
            }
        }
    }
}

bool GeoJsonHelper::loadPolygonsFromFile(const QString &filePath, QList<QList<QGeoCoordinate>> &polygons, QString &errorString, double filterMeters)
{
    errorString.clear();
    polygons.clear();

    const auto jsonResult = GeoFileIO::loadJson(filePath, QString::fromLatin1(kFormatName));
    if (!jsonResult.success) {
        errorString = jsonResult.error;
        return false;
    }

    const QVariantList shapes = QGeoJson::importGeoJson(jsonResult.document);
    if (shapes.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),
            QT_TRANSLATE_NOOP("GeoJson", "No polygon data found in GeoJson file."));
        qCWarning(GeoJsonHelperLog) << errorString;
        return false;
    }

    extractPolygons(shapes, polygons, filterMeters);

    if (polygons.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),
            QT_TRANSLATE_NOOP("GeoJson", "No polygon found in GeoJson file."));
        qCWarning(GeoJsonHelperLog) << errorString;
        return false;
    }

    qCDebug(GeoJsonHelperLog) << "Loaded" << polygons.size() << "polygon(s) from" << filePath;
    return true;
}

bool GeoJsonHelper::loadPolygonWithHolesFromFile(const QString &filePath, QGeoPolygon &polygon, QString &errorString)
{
    QList<QGeoPolygon> polygons;
    if (!loadPolygonsWithHolesFromFile(filePath, polygons, errorString)) {
        return false;
    }
    polygon = polygons.first();
    return true;
}

namespace {
    // Helper to extract polygons with holes from QGeoJson items (handles nested FeatureCollections)
    void extractPolygonsWithHoles(const QVariantList &items, QList<QGeoPolygon> &polygons)
    {
        for (const QVariant &item : items) {
            if (!item.canConvert<QVariantMap>()) {
                continue;
            }
            const QVariantMap map = item.toMap();
            const QString type = map.value(QStringLiteral("type")).toString();
            const QVariant data = map.value(QStringLiteral("data"));

            if (type == QStringLiteral("FeatureCollection") && data.canConvert<QVariantList>()) {
                extractPolygonsWithHoles(data.toList(), polygons);
            } else if (data.canConvert<QGeoPolygon>()) {
                polygons.append(data.value<QGeoPolygon>());
            }
        }
    }
}

bool GeoJsonHelper::loadPolygonsWithHolesFromFile(const QString &filePath, QList<QGeoPolygon> &polygons, QString &errorString)
{
    errorString.clear();
    polygons.clear();

    const auto jsonResult = GeoFileIO::loadJson(filePath, QString::fromLatin1(kFormatName));
    if (!jsonResult.success) {
        errorString = jsonResult.error;
        return false;
    }

    const QVariantList shapes = QGeoJson::importGeoJson(jsonResult.document);
    if (shapes.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),
            QT_TRANSLATE_NOOP("GeoJson", "No polygon data found in GeoJson file."));
        qCWarning(GeoJsonHelperLog) << errorString;
        return false;
    }

    extractPolygonsWithHoles(shapes, polygons);

    if (polygons.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),
            QT_TRANSLATE_NOOP("GeoJson", "No polygon found in GeoJson file."));
        qCWarning(GeoJsonHelperLog) << errorString;
        return false;
    }

    qCDebug(GeoJsonHelperLog) << "Loaded" << polygons.size() << "polygon(s) with holes from" << filePath;
    return true;
}

bool GeoJsonHelper::loadPolylineFromFile(const QString &filePath, QList<QGeoCoordinate> &coords, QString &errorString, double filterMeters)
{
    QList<QList<QGeoCoordinate>> polylines;
    if (!loadPolylinesFromFile(filePath, polylines, errorString, filterMeters)) {
        return false;
    }
    coords = polylines.first();
    return true;
}

namespace {
    // Helper to extract polylines from QGeoJson items (handles nested FeatureCollections)
    void extractPolylines(const QVariantList &items, QList<QList<QGeoCoordinate>> &polylines, double filterMeters)
    {
        for (const QVariant &item : items) {
            if (!item.canConvert<QVariantMap>()) {
                continue;
            }
            const QVariantMap map = item.toMap();
            const QString type = map.value(QStringLiteral("type")).toString();
            const QVariant data = map.value(QStringLiteral("data"));

            if (type == QStringLiteral("FeatureCollection") && data.canConvert<QVariantList>()) {
                extractPolylines(data.toList(), polylines, filterMeters);
            } else if (data.canConvert<QGeoPath>()) {
                const QGeoPath path = data.value<QGeoPath>();
                QList<QGeoCoordinate> coords = path.path();
                GeoUtilities::validateAndNormalizeCoordinates(coords, logValidationWarning);
                GeoFormatRegistry::filterVertices(coords, filterMeters, GeoUtilities::kMinPolylineVertices);
                polylines.append(coords);
            }
        }
    }
}

bool GeoJsonHelper::loadPolylinesFromFile(const QString &filePath, QList<QList<QGeoCoordinate>> &polylines, QString &errorString, double filterMeters)
{
    errorString.clear();
    polylines.clear();

    const auto jsonResult = GeoFileIO::loadJson(filePath, QString::fromLatin1(kFormatName));
    if (!jsonResult.success) {
        errorString = jsonResult.error;
        return false;
    }

    const QVariantList shapes = QGeoJson::importGeoJson(jsonResult.document);
    if (shapes.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),
            QT_TRANSLATE_NOOP("GeoJson", "No polyline data found in GeoJson file."));
        qCWarning(GeoJsonHelperLog) << errorString;
        return false;
    }

    extractPolylines(shapes, polylines, filterMeters);

    if (polylines.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),
            QT_TRANSLATE_NOOP("GeoJson", "No polyline found in GeoJson file."));
        qCWarning(GeoJsonHelperLog) << errorString;
        return false;
    }

    qCDebug(GeoJsonHelperLog) << "Loaded" << polylines.size() << "polyline(s) from" << filePath;
    return true;
}

namespace {
    // Helper to extract points from QGeoJson items (handles nested FeatureCollections)
    void extractPoints(const QVariantList &items, QList<QGeoCoordinate> &points)
    {
        for (const QVariant &item : items) {
            if (!item.canConvert<QVariantMap>()) {
                continue;
            }
            const QVariantMap map = item.toMap();
            const QString type = map.value(QStringLiteral("type")).toString();
            const QVariant data = map.value(QStringLiteral("data"));

            if (type == QStringLiteral("FeatureCollection") && data.canConvert<QVariantList>()) {
                extractPoints(data.toList(), points);
            } else if (data.canConvert<QGeoShape>()) {
                const QGeoShape shape = data.value<QGeoShape>();
                if (shape.type() == QGeoShape::CircleType) {
                    // QGeoCircle is used for Point features - get the center
                    QGeoCoordinate coord = shape.center();
                    // Validate and normalize the point
                    if (!GeoUtilities::isValidCoordinate(coord)) {
                        qCWarning(GeoJsonHelperLog) << "Point coordinate out of range, normalizing";
                        coord = GeoUtilities::normalizeCoordinate(coord);
                    }
                    if (!std::isnan(coord.altitude()) && !GeoUtilities::isValidAltitude(coord.altitude())) {
                        qCWarning(GeoJsonHelperLog) << "Point altitude out of expected range:" << coord.altitude();
                    }
                    points.append(coord);
                }
            }
        }
    }
}

bool GeoJsonHelper::loadPointFromFile(const QString &filePath, QGeoCoordinate &point, QString &errorString)
{
    QList<QGeoCoordinate> points;
    if (!loadPointsFromFile(filePath, points, errorString)) {
        return false;
    }
    if (points.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),
            QT_TRANSLATE_NOOP("GeoJson", "No points found in GeoJson file."));
        return false;
    }
    point = points.first();
    return true;
}

bool GeoJsonHelper::loadPointsFromFile(const QString &filePath, QList<QGeoCoordinate> &points, QString &errorString)
{
    errorString.clear();
    points.clear();

    const auto jsonResult = GeoFileIO::loadJson(filePath, QString::fromLatin1(kFormatName));
    if (!jsonResult.success) {
        errorString = jsonResult.error;
        return false;
    }

    const QVariantList shapes = QGeoJson::importGeoJson(jsonResult.document);
    if (shapes.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),
            QT_TRANSLATE_NOOP("GeoJson", "No point data found in GeoJson file."));
        qCWarning(GeoJsonHelperLog) << errorString;
        return false;
    }

    extractPoints(shapes, points);

    if (points.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),
            QT_TRANSLATE_NOOP("GeoJson", "No point found in GeoJson file."));
        qCWarning(GeoJsonHelperLog) << errorString;
        return false;
    }

    qCDebug(GeoJsonHelperLog) << "Loaded" << points.size() << "point(s) from" << filePath;
    return true;
}

// ============================================================================
// Save functions
// ============================================================================

bool GeoJsonHelper::savePolygonToFile(const QString &filePath, const QList<QGeoCoordinate> &vertices, QString &errorString)
{
    // Validate coordinates before saving
    QString validationError;
    if (!GeoUtilities::validateCoordinates(vertices, validationError)) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),validationError);
        qCWarning(GeoJsonHelperLog) << errorString;
        return false;
    }

    // Build coordinates array: [[[lon, lat, alt?], ...]]
    QJsonArray ring;
    for (const QGeoCoordinate &coord : vertices) {
        QJsonArray point;
        point.append(coord.longitude());
        point.append(coord.latitude());
        if (!qIsNaN(coord.altitude())) {
            point.append(coord.altitude());
        }
        ring.append(point);
    }

    // Close the ring if not already closed
    if (!vertices.isEmpty() && vertices.first() != vertices.last()) {
        QJsonArray closePoint;
        closePoint.append(vertices.first().longitude());
        closePoint.append(vertices.first().latitude());
        if (!qIsNaN(vertices.first().altitude())) {
            closePoint.append(vertices.first().altitude());
        }
        ring.append(closePoint);
    }

    QJsonArray coordinates;
    coordinates.append(ring);

    // Build Feature structure
    QJsonObject geometry;
    geometry.insert(QStringLiteral("type"), QStringLiteral("Polygon"));
    geometry.insert(QStringLiteral("coordinates"), coordinates);

    QJsonObject feature;
    feature.insert(QStringLiteral("type"), QStringLiteral("Feature"));
    feature.insert(QStringLiteral("geometry"), geometry);
    feature.insert(QStringLiteral("properties"), QJsonObject());

    const QJsonDocument doc(feature);
    return GeoFileIO::saveJson(filePath, doc, QString::fromLatin1(kFormatName), errorString);
}

bool GeoJsonHelper::savePolygonsToFile(const QString &filePath, const QList<QList<QGeoCoordinate>> &polygons, QString &errorString)
{
    if (polygons.isEmpty()) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),
            QT_TRANSLATE_NOOP("GeoJson", "No polygons to save."));
        qCWarning(GeoJsonHelperLog) << errorString;
        return false;
    }

    // Validate all coordinates before saving
    QString validationError;
    if (!GeoUtilities::validatePolygonListCoordinates(polygons, validationError)) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName), validationError);
        qCWarning(GeoJsonHelperLog) << errorString;
        return false;
    }

    QJsonArray features;
    for (const QList<QGeoCoordinate> &vertices : polygons) {
        // Build coordinates array
        QJsonArray ring;
        for (const QGeoCoordinate &coord : vertices) {
            QJsonArray point;
            point.append(coord.longitude());
            point.append(coord.latitude());
            if (!qIsNaN(coord.altitude())) {
                point.append(coord.altitude());
            }
            ring.append(point);
        }

        // Close the ring if not already closed
        if (!vertices.isEmpty() && vertices.first() != vertices.last()) {
            QJsonArray closePoint;
            closePoint.append(vertices.first().longitude());
            closePoint.append(vertices.first().latitude());
            if (!qIsNaN(vertices.first().altitude())) {
                closePoint.append(vertices.first().altitude());
            }
            ring.append(closePoint);
        }

        QJsonArray coordinates;
        coordinates.append(ring);

        QJsonObject geometry;
        geometry.insert(QStringLiteral("type"), QStringLiteral("Polygon"));
        geometry.insert(QStringLiteral("coordinates"), coordinates);

        QJsonObject feature;
        feature.insert(QStringLiteral("type"), QStringLiteral("Feature"));
        feature.insert(QStringLiteral("geometry"), geometry);
        feature.insert(QStringLiteral("properties"), QJsonObject());

        features.append(feature);
    }

    QJsonObject featureCollection;
    featureCollection.insert(QStringLiteral("type"), QStringLiteral("FeatureCollection"));
    featureCollection.insert(QStringLiteral("features"), features);

    const QJsonDocument doc(featureCollection);
    return GeoFileIO::saveJson(filePath, doc, QString::fromLatin1(kFormatName), errorString);
}

bool GeoJsonHelper::savePolygonWithHolesToFile(const QString &filePath, const QGeoPolygon &polygon, QString &errorString)
{
    // Validate all coordinates
    QString validationError;
    if (!GeoUtilities::validateGeoPolygonCoordinates(polygon, validationError)) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName), validationError);
        qCWarning(GeoJsonHelperLog) << errorString;
        return false;
    }

    QJsonArray coordinates;

    // Outer ring (perimeter)
    coordinates.append(coordinatesToJsonRing(polygon.perimeter()));

    // Inner rings (holes)
    for (int i = 0; i < polygon.holesCount(); ++i) {
        coordinates.append(coordinatesToJsonRing(polygon.holePath(i)));
    }

    QJsonObject geometry;
    geometry.insert(QStringLiteral("type"), QStringLiteral("Polygon"));
    geometry.insert(QStringLiteral("coordinates"), coordinates);

    QJsonObject feature;
    feature.insert(QStringLiteral("type"), QStringLiteral("Feature"));
    feature.insert(QStringLiteral("geometry"), geometry);
    feature.insert(QStringLiteral("properties"), QJsonObject());

    const QJsonDocument doc(feature);
    return GeoFileIO::saveJson(filePath, doc, QString::fromLatin1(kFormatName), errorString);
}

bool GeoJsonHelper::savePolygonsWithHolesToFile(const QString &filePath, const QList<QGeoPolygon> &polygons, QString &errorString)
{
    if (polygons.isEmpty()) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),
            QT_TRANSLATE_NOOP("GeoJson", "No polygons to save."));
        qCWarning(GeoJsonHelperLog) << errorString;
        return false;
    }

    // Validate all coordinates before saving
    QString validationError;
    if (!GeoUtilities::validateGeoPolygonListCoordinates(polygons, validationError)) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName), validationError);
        qCWarning(GeoJsonHelperLog) << errorString;
        return false;
    }

    QJsonArray features;
    for (const QGeoPolygon &polygon : polygons) {
        QJsonArray coordinates;

        // Outer ring (perimeter)
        coordinates.append(coordinatesToJsonRing(polygon.perimeter()));

        // Inner rings (holes)
        for (int i = 0; i < polygon.holesCount(); ++i) {
            coordinates.append(coordinatesToJsonRing(polygon.holePath(i)));
        }

        QJsonObject geometry;
        geometry.insert(QStringLiteral("type"), QStringLiteral("Polygon"));
        geometry.insert(QStringLiteral("coordinates"), coordinates);

        QJsonObject feature;
        feature.insert(QStringLiteral("type"), QStringLiteral("Feature"));
        feature.insert(QStringLiteral("geometry"), geometry);
        feature.insert(QStringLiteral("properties"), QJsonObject());

        features.append(feature);
    }

    QJsonObject featureCollection;
    featureCollection.insert(QStringLiteral("type"), QStringLiteral("FeatureCollection"));
    featureCollection.insert(QStringLiteral("features"), features);

    const QJsonDocument doc(featureCollection);
    return GeoFileIO::saveJson(filePath, doc, QString::fromLatin1(kFormatName), errorString);
}

bool GeoJsonHelper::savePolylineToFile(const QString &filePath, const QList<QGeoCoordinate> &coords, QString &errorString)
{
    // Validate coordinates before saving
    QString validationError;
    if (!GeoUtilities::validateCoordinates(coords, validationError)) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),validationError);
        qCWarning(GeoJsonHelperLog) << errorString;
        return false;
    }

    QJsonArray coordinates;
    for (const QGeoCoordinate &coord : coords) {
        QJsonArray point;
        point.append(coord.longitude());
        point.append(coord.latitude());
        if (!qIsNaN(coord.altitude())) {
            point.append(coord.altitude());
        }
        coordinates.append(point);
    }

    QJsonObject geometry;
    geometry.insert(QStringLiteral("type"), QStringLiteral("LineString"));
    geometry.insert(QStringLiteral("coordinates"), coordinates);

    QJsonObject feature;
    feature.insert(QStringLiteral("type"), QStringLiteral("Feature"));
    feature.insert(QStringLiteral("geometry"), geometry);
    feature.insert(QStringLiteral("properties"), QJsonObject());

    const QJsonDocument doc(feature);
    return GeoFileIO::saveJson(filePath, doc, QString::fromLatin1(kFormatName), errorString);
}

bool GeoJsonHelper::savePolylinesToFile(const QString &filePath, const QList<QList<QGeoCoordinate>> &polylines, QString &errorString)
{
    if (polylines.isEmpty()) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),
            QT_TRANSLATE_NOOP("GeoJson", "No polylines to save."));
        qCWarning(GeoJsonHelperLog) << errorString;
        return false;
    }

    // Validate all coordinates before saving
    for (int i = 0; i < polylines.size(); i++) {
        QString validationError;
        if (!GeoUtilities::validateCoordinates(polylines[i], validationError)) {
            errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),
                QObject::tr("Polyline %1: %2").arg(i + 1).arg(validationError));
            qCWarning(GeoJsonHelperLog) << errorString;
            return false;
        }
    }

    QJsonArray features;
    for (const QList<QGeoCoordinate> &coords : polylines) {
        QJsonArray coordinates;
        for (const QGeoCoordinate &coord : coords) {
            QJsonArray point;
            point.append(coord.longitude());
            point.append(coord.latitude());
            if (!qIsNaN(coord.altitude())) {
                point.append(coord.altitude());
            }
            coordinates.append(point);
        }

        QJsonObject geometry;
        geometry.insert(QStringLiteral("type"), QStringLiteral("LineString"));
        geometry.insert(QStringLiteral("coordinates"), coordinates);

        QJsonObject feature;
        feature.insert(QStringLiteral("type"), QStringLiteral("Feature"));
        feature.insert(QStringLiteral("geometry"), geometry);
        feature.insert(QStringLiteral("properties"), QJsonObject());

        features.append(feature);
    }

    QJsonObject featureCollection;
    featureCollection.insert(QStringLiteral("type"), QStringLiteral("FeatureCollection"));
    featureCollection.insert(QStringLiteral("features"), features);

    const QJsonDocument doc(featureCollection);
    return GeoFileIO::saveJson(filePath, doc, QString::fromLatin1(kFormatName), errorString);
}

bool GeoJsonHelper::savePointToFile(const QString &filePath, const QGeoCoordinate &point, QString &errorString)
{
    return savePointsToFile(filePath, QList<QGeoCoordinate>() << point, errorString);
}

bool GeoJsonHelper::savePointsToFile(const QString &filePath, const QList<QGeoCoordinate> &points, QString &errorString)
{
    if (points.isEmpty()) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),
            QT_TRANSLATE_NOOP("GeoJson", "No points to save."));
        qCWarning(GeoJsonHelperLog) << errorString;
        return false;
    }

    // Validate all coordinates before saving
    QString validationError;
    if (!GeoUtilities::validateCoordinates(points, validationError)) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),validationError);
        qCWarning(GeoJsonHelperLog) << errorString;
        return false;
    }

    if (points.size() == 1) {
        // Single point - save as Feature with Point geometry
        const QGeoCoordinate &coord = points.first();
        QJsonArray coordinates;
        coordinates.append(coord.longitude());
        coordinates.append(coord.latitude());
        if (!qIsNaN(coord.altitude())) {
            coordinates.append(coord.altitude());
        }

        QJsonObject geometry;
        geometry.insert(QStringLiteral("type"), QStringLiteral("Point"));
        geometry.insert(QStringLiteral("coordinates"), coordinates);

        QJsonObject feature;
        feature.insert(QStringLiteral("type"), QStringLiteral("Feature"));
        feature.insert(QStringLiteral("geometry"), geometry);
        feature.insert(QStringLiteral("properties"), QJsonObject());

        const QJsonDocument doc(feature);
        return GeoFileIO::saveJson(filePath, doc, QString::fromLatin1(kFormatName), errorString);
    }

    // Multiple points - save as FeatureCollection with Point features
    QJsonArray features;
    for (const QGeoCoordinate &coord : points) {
        QJsonArray coordinates;
        coordinates.append(coord.longitude());
        coordinates.append(coord.latitude());
        if (!qIsNaN(coord.altitude())) {
            coordinates.append(coord.altitude());
        }

        QJsonObject geometry;
        geometry.insert(QStringLiteral("type"), QStringLiteral("Point"));
        geometry.insert(QStringLiteral("coordinates"), coordinates);

        QJsonObject feature;
        feature.insert(QStringLiteral("type"), QStringLiteral("Feature"));
        feature.insert(QStringLiteral("geometry"), geometry);
        feature.insert(QStringLiteral("properties"), QJsonObject());

        features.append(feature);
    }

    QJsonObject featureCollection;
    featureCollection.insert(QStringLiteral("type"), QStringLiteral("FeatureCollection"));
    featureCollection.insert(QStringLiteral("features"), features);

    const QJsonDocument doc(featureCollection);
    return GeoFileIO::saveJson(filePath, doc, QString::fromLatin1(kFormatName), errorString);
}

// ============================================================================
// Coordinate helpers
// ============================================================================

bool GeoJsonHelper::loadGeoJsonCoordinate(const QJsonValue &jsonValue, bool altitudeRequired, QGeoCoordinate &coordinate, QString &errorString)
{
    return JsonHelper::loadGeoCoordinate(jsonValue, altitudeRequired, coordinate, errorString, true /* geoJsonFormat */);
}

void GeoJsonHelper::saveGeoJsonCoordinate(const QGeoCoordinate &coordinate, bool writeAltitude, QJsonValue &jsonValue)
{
    JsonHelper::saveGeoCoordinate(coordinate, writeAltitude, jsonValue, true /* geoJsonFormat */);
}

bool GeoJsonHelper::loadGeoJsonCoordinateArray(const QJsonValue &jsonValue, bool altitudeRequired, QList<QGeoCoordinate> &coordinates, QString &errorString)
{
    return JsonHelper::loadGeoCoordinateArray(jsonValue, altitudeRequired, coordinates, errorString, true /* geoJsonFormat */);
}

void GeoJsonHelper::saveGeoJsonCoordinateArray(const QList<QGeoCoordinate> &coordinates, bool writeAltitude, QJsonValue &jsonValue)
{
    JsonHelper::saveGeoCoordinateArray(coordinates, writeAltitude, jsonValue, true /* geoJsonFormat */);
}

QJsonArray GeoJsonHelper::coordinatesToJsonRing(const QList<QGeoCoordinate> &coords)
{
    QJsonArray ring;
    for (const QGeoCoordinate &coord : coords) {
        QJsonArray point;
        point.append(coord.longitude());
        point.append(coord.latitude());
        if (!qIsNaN(coord.altitude())) {
            point.append(coord.altitude());
        }
        ring.append(point);
    }

    // Close the ring (GeoJSON requires first == last for polygon rings)
    if (!coords.isEmpty() && coords.first() != coords.last()) {
        QJsonArray closePoint;
        closePoint.append(coords.first().longitude());
        closePoint.append(coords.first().latitude());
        if (!qIsNaN(coords.first().altitude())) {
            closePoint.append(coords.first().altitude());
        }
        ring.append(closePoint);
    }

    return ring;
}
