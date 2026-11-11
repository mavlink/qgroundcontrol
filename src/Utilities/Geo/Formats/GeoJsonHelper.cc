#include "GeoJsonHelper.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QVariantMap>
#include <QtPositioning/QGeoCoordinate>

#include "JsonParsing.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GeoJsonHelperLog, "Utilities.GeoJsonHelper")

namespace GeoJsonHelper {

struct Shape
{
    enum class Kind
    {
        Point,
        Polyline,
        Polygon,
    };

    Kind kind;
    QVariantList positions;
};

QJsonDocument _loadFile(const QString& filePath, QString& errorString);
QList<Shape> _shapes(const QJsonDocument& jsonDoc);
void _appendShapes(const QVariantMap& geometry, int nestingDepth, QList<Shape>& shapes);
QList<QGeoCoordinate> _coordinates(const QVariantList& positions);

// Shapes inside more collections (FeatureCollection, GeometryCollection, Multi* geometry) are ignored.
constexpr int _maxNestingDepth = 15;
constexpr const char* _errorPrefix = QT_TRANSLATE_NOOP("GeoJsonHelper", "GeoJson file load failed. %1");

}  // namespace GeoJsonHelper

void GeoJsonHelper::_appendShapes(const QVariantMap& geometry, int nestingDepth, QList<Shape>& shapes)
{
    if (nestingDepth > _maxNestingDepth) {
        return;
    }

    const QString type = geometry.value(QStringLiteral("type")).toString();
    const QVariantList coordinates = geometry.value(QStringLiteral("coordinates")).toList();

    if (type == QLatin1String("Point")) {
        shapes.append(Shape{Shape::Kind::Point, {}});
    } else if (type == QLatin1String("LineString")) {
        shapes.append(Shape{Shape::Kind::Polyline, coordinates});
    } else if (type == QLatin1String("Polygon")) {
        // The first ring is the perimeter, the others are holes.
        shapes.append(Shape{Shape::Kind::Polygon, coordinates.value(0).toList()});
    } else if ((type == QLatin1String("MultiPoint")) || (type == QLatin1String("MultiLineString")) ||
               (type == QLatin1String("MultiPolygon"))) {
        // A Multi* geometry lists the coordinates of geometries of the type named after "Multi".
        const QString memberType = type.sliced(QLatin1String("Multi").size());
        for (const QVariant& memberCoordinates : coordinates) {
            _appendShapes({{QStringLiteral("type"), memberType}, {QStringLiteral("coordinates"), memberCoordinates}},
                          nestingDepth + 1, shapes);
        }
    } else if (type == QLatin1String("GeometryCollection")) {
        const QVariantList geometries = geometry.value(QStringLiteral("geometries")).toList();
        for (const QVariant& member : geometries) {
            _appendShapes(member.toMap(), nestingDepth + 1, shapes);
        }
    }
}

// Returns the shapes of all geometries in document order: callers use the first of the kind they want.
QList<GeoJsonHelper::Shape> GeoJsonHelper::_shapes(const QJsonDocument& jsonDoc)
{
    // Values are converted by QVariant, as in Qt's GeoJSON importer, so that malformed files load as they did
    // with it: a number written as a string still counts as a number, for example.
    const QVariantMap root = jsonDoc.object().toVariantMap();
    const QString type = root.value(QStringLiteral("type")).toString();

    QList<Shape> shapes;
    if (type == QLatin1String("FeatureCollection")) {
        const QVariantList features = root.value(QStringLiteral("features")).toList();
        for (const QVariant& feature : features) {
            _appendShapes(feature.toMap().value(QStringLiteral("geometry")).toMap(), 1, shapes);
        }
    } else if (type == QLatin1String("Feature")) {
        _appendShapes(root.value(QStringLiteral("geometry")).toMap(), 0, shapes);
    } else {
        _appendShapes(root, 0, shapes);
    }

    return shapes;
}

QList<QGeoCoordinate> GeoJsonHelper::_coordinates(const QVariantList& positions)
{
    QList<QGeoCoordinate> coordinates;
    coordinates.reserve(positions.size());

    for (const QVariant& positionValue : positions) {
        // GeoJSON ordering is [lon, lat, alt] (RFC 7946).
        const QVariantList position = positionValue.toList();
        QGeoCoordinate coordinate;
        if (position.size() >= 2) {
            coordinate.setLongitude(position.at(0).toDouble());
            coordinate.setLatitude(position.at(1).toDouble());
        }
        if (position.size() >= 3) {
            coordinate.setAltitude(position.at(2).toDouble());
        }
        if (!coordinate.isValid()) {
            // One invalid position discards the whole line or ring, as QGeoPath and QGeoPolygon did.
            return {};
        }
        coordinates.append(coordinate);
    }

    return coordinates;
}

QJsonDocument GeoJsonHelper::_loadFile(const QString &filePath, QString &errorString)
{
    errorString.clear();

    QFile file(filePath);
    if (!file.exists()) {
        errorString = QCoreApplication::translate("GeoJsonHelper", _errorPrefix).arg(
            QCoreApplication::translate("GeoJson", "File not found: %1").arg(filePath));
        return QJsonDocument();
    }

    if (!file.open(QIODevice::ReadOnly)) {
        errorString = QCoreApplication::translate("GeoJsonHelper", _errorPrefix).arg(
            QCoreApplication::translate("GeoJson", "Unable to open file: %1 error: %2")
                .arg(filePath, file.errorString()));
        return QJsonDocument();
    }

    QJsonDocument jsonDoc;
    const QByteArray bytes = file.readAll();
    if (!JsonParsing::isJsonFile(bytes, jsonDoc, errorString)) {
        errorString = QCoreApplication::translate("GeoJsonHelper", _errorPrefix).arg(errorString);
    }

    return jsonDoc;
}

ShapeFileHelper::ShapeType GeoJsonHelper::determineShapeType(const QString &filePath, QString &errorString)
{
    using ShapeType = ShapeFileHelper::ShapeType;

    const QJsonDocument jsonDoc = GeoJsonHelper::_loadFile(filePath, errorString);
    if (!errorString.isEmpty()) {
        return ShapeType::Error;
    }

    const QList<Shape> shapes = _shapes(jsonDoc);
    if (shapes.isEmpty()) {
        errorString = QCoreApplication::translate("GeoJsonHelper", _errorPrefix).arg(
            QCoreApplication::translate("GeoJson", "No shapes found in GeoJson file."));
        return ShapeType::Error;
    }

    for (const Shape& shape : shapes) {
        if (shape.kind == Shape::Kind::Polygon) {
            return ShapeType::Polygon;
        }
        if (shape.kind == Shape::Kind::Polyline) {
            return ShapeType::Polyline;
        }
    }

    errorString = QCoreApplication::translate("GeoJsonHelper", _errorPrefix).arg(
        QCoreApplication::translate("GeoJson", "No supported type found in GeoJson file."));
    return ShapeType::Error;
}

bool GeoJsonHelper::loadPolygonFromFile(const QString &filePath, QList<QGeoCoordinate> &vertices, QString &errorString)
{
    errorString.clear();
    vertices.clear();

    const QJsonDocument jsonDoc = GeoJsonHelper::_loadFile(filePath, errorString);
    if (!errorString.isEmpty()) {
        return false;
    }

    const QList<Shape> shapes = _shapes(jsonDoc);
    if (shapes.isEmpty()) {
        errorString = QCoreApplication::translate("GeoJsonHelper", _errorPrefix).arg(
            QCoreApplication::translate("GeoJson", "No polygon data found in GeoJson file."));
        return false;
    }

    for (const Shape& shape : shapes) {
        if (shape.kind == Shape::Kind::Polygon) {
            vertices = _coordinates(shape.positions);
            return true;
        }
    }

    errorString = QCoreApplication::translate("GeoJsonHelper", _errorPrefix).arg(
        QCoreApplication::translate("GeoJson", "No polygon found in GeoJson file."));
    return false;
}

bool GeoJsonHelper::loadPolylineFromFile(const QString &filePath, QList<QGeoCoordinate> &coords, QString &errorString)
{
    errorString.clear();
    coords.clear();

    const QJsonDocument jsonDoc = GeoJsonHelper::_loadFile(filePath, errorString);
    if (!errorString.isEmpty()) {
        return false;
    }

    const QList<Shape> shapes = _shapes(jsonDoc);
    if (shapes.isEmpty()) {
        errorString = QCoreApplication::translate("GeoJsonHelper", _errorPrefix).arg(
            QCoreApplication::translate("GeoJson", "No polyline data found in GeoJson file."));
        return false;
    }

    for (const Shape& shape : shapes) {
        if (shape.kind == Shape::Kind::Polyline) {
            coords = _coordinates(shape.positions);
            return true;
        }
    }

    errorString = QCoreApplication::translate("GeoJsonHelper", _errorPrefix).arg(
        QCoreApplication::translate("GeoJson", "No polyline found in GeoJson file."));
    return false;
}

bool GeoJsonHelper::loadGeoJsonCoordinate(const QJsonValue &jsonValue, bool altitudeRequired, QGeoCoordinate &coordinate, QString &errorString)
{
    if (!jsonValue.isArray()) {
        errorString = QCoreApplication::translate("GeoJsonHelper", "value for coordinate is not array");
        return false;
    }

    const QJsonArray coordinateArray = jsonValue.toArray();
    const int requiredCount = altitudeRequired ? 3 : 2;
    if (coordinateArray.count() != requiredCount) {
        errorString = QCoreApplication::translate("GeoJsonHelper", "Coordinate array must contain %1 values").arg(requiredCount);
        return false;
    }

    for (const QJsonValue &coordinateValue : coordinateArray) {
        if ((coordinateValue.type() != QJsonValue::Double) && (coordinateValue.type() != QJsonValue::Null)) {
            errorString =
                QCoreApplication::translate("GeoJsonHelper", "Coordinate array may only contain double values, found: %1").arg(coordinateValue.type());
            return false;
        }
    }

    // GeoJSON ordering is [lon, lat, alt] (RFC 7946).
    coordinate = QGeoCoordinate(coordinateArray[1].toDouble(), coordinateArray[0].toDouble());
    if (altitudeRequired) {
        coordinate.setAltitude(JsonParsing::possibleNaNJsonValue(coordinateArray[2]));
    }

    return true;
}

void GeoJsonHelper::saveGeoJsonCoordinate(const QGeoCoordinate &coordinate, bool writeAltitude, QJsonValue &jsonValue)
{
    QJsonArray coordinateArray;
    coordinateArray << coordinate.longitude() << coordinate.latitude();
    if (writeAltitude) {
        coordinateArray << coordinate.altitude();
    }
    jsonValue = QJsonValue(coordinateArray);
}

bool GeoJsonHelper::loadGeoCoordinate(const QJsonValue &jsonValue, bool altitudeRequired, QGeoCoordinate &coordinate,
                                      QString &errorString)
{
    if (!jsonValue.isArray()) {
        errorString = QCoreApplication::translate("GeoJsonHelper", "value for coordinate is not array");
        return false;
    }

    const QJsonArray coordinateArray = jsonValue.toArray();
    const int requiredCount = altitudeRequired ? 3 : 2;
    if (coordinateArray.count() != requiredCount) {
        errorString = QCoreApplication::translate("GeoJsonHelper", "Coordinate array must contain %1 values").arg(requiredCount);
        return false;
    }

    for (const QJsonValue &coordinateValue : coordinateArray) {
        if ((coordinateValue.type() != QJsonValue::Double) && (coordinateValue.type() != QJsonValue::Null)) {
            errorString =
                QCoreApplication::translate("GeoJsonHelper", "Coordinate array may only contain double values, found: %1").arg(coordinateValue.type());
            return false;
        }
    }

    coordinate = QGeoCoordinate(
        JsonParsing::possibleNaNJsonValue(coordinateArray[0]),
        JsonParsing::possibleNaNJsonValue(coordinateArray[1]));

    if (altitudeRequired) {
        coordinate.setAltitude(JsonParsing::possibleNaNJsonValue(coordinateArray[2]));
    }

    return true;
}

void GeoJsonHelper::saveGeoCoordinate(const QGeoCoordinate &coordinate, bool writeAltitude, QJsonValue &jsonValue)
{
    QJsonArray coordinateArray;
    coordinateArray << coordinate.latitude() << coordinate.longitude();

    if (writeAltitude) {
        coordinateArray << coordinate.altitude();
    }

    jsonValue = QJsonValue(coordinateArray);
}

bool GeoJsonHelper::loadGeoCoordinateArray(const QJsonValue &jsonValue, bool altitudeRequired,
                                           QVariantList &rgVarPoints, QString &errorString)
{
    if (!jsonValue.isArray()) {
        errorString = QCoreApplication::translate("GeoJsonHelper", "value for coordinate array is not array");
        return false;
    }

    const QJsonArray rgJsonPoints = jsonValue.toArray();

    rgVarPoints.clear();
    for (const QJsonValue &point : rgJsonPoints) {
        QGeoCoordinate coordinate;
        if (!loadGeoCoordinate(point, altitudeRequired, coordinate, errorString)) {
            return false;
        }
        rgVarPoints.append(QVariant::fromValue(coordinate));
    }

    return true;
}

bool GeoJsonHelper::loadGeoCoordinateArray(const QJsonValue &jsonValue, bool altitudeRequired,
                                           QList<QGeoCoordinate> &rgPoints, QString &errorString)
{
    QVariantList rgVarPoints;

    if (!loadGeoCoordinateArray(jsonValue, altitudeRequired, rgVarPoints, errorString)) {
        return false;
    }

    rgPoints.clear();
    for (const QVariant &point : rgVarPoints) {
        rgPoints.append(point.value<QGeoCoordinate>());
    }

    return true;
}

void GeoJsonHelper::saveGeoCoordinateArray(const QVariantList &rgVarPoints, bool writeAltitude, QJsonValue &jsonValue)
{
    QJsonArray rgJsonPoints;
    for (const QVariant &point : rgVarPoints) {
        QJsonValue jsonPoint;
        saveGeoCoordinate(point.value<QGeoCoordinate>(), writeAltitude, jsonPoint);
        rgJsonPoints.append(jsonPoint);
    }

    jsonValue = rgJsonPoints;
}

void GeoJsonHelper::saveGeoCoordinateArray(const QList<QGeoCoordinate> &rgPoints, bool writeAltitude,
                                           QJsonValue &jsonValue)
{
    QVariantList rgVarPoints;
    for (const QGeoCoordinate &coord : rgPoints) {
        rgVarPoints.append(QVariant::fromValue(coord));
    }

    saveGeoCoordinateArray(rgVarPoints, writeAltitude, jsonValue);
}
